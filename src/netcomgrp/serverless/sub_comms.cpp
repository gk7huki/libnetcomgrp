#include <ace/ACE.h>

#include <exception>
#include <algorithm>

#include "../exception.h"
#include "sub_comms.h"
#include "protocol/data_list.h"
#include "protocol/address_tuple.h"
#include "protocol/pdu.h"

// 45sec keepalive interval if seems like connection
// is behind NAT
#define KEEPALIVE_INTERVAL 45

namespace netcomgrp {
namespace serverless {

sub_comms::sub_comms(group *owner) 
  : sub_system(owner, "comms"), 
    _opened_port(0), 
    _public_port(0),
    _keepalive_mode(false),
    _keepalive_timer(-1)
{
	// Reserve enough for one UDP packet, taking into
	// account IP+UDP+ReUDP etc. headers take some space
	_msg_send = new reudp::msg_block_type(65000);
	_msg_recv = new reudp::msg_block_type(65000);
	_sock.packet_done_cb(_packet_done, this);
	_next_timeout = time_value_type::max_time;
}

sub_comms::~sub_comms() {
	_close();
	
	// TODO deregister handler?
	delete _msg_send;
	delete _msg_recv;
}

void
sub_comms::join(const char *grp_id) {	
	// TODO configurable port number(s)
	_open(this->addr_announce().get_port_number());
	
}

void
sub_comms::leave() {
	_send_leave();
	// TODO Should really before announcing successfull leaving let 
	// sending finish to all the peers but... this will have to
	// do for now.
	this->notify(state::signal_left);
	
	_close();
}

void 
sub_comms::send(const char *msg, size_t msglen, const node *recp_orig) {
	node *recp = this->peer_confirmed_find(recp_orig->addr());
	if (!recp) {
		ACE_DEBUG((LM_DEBUG, "netcomgrp::serverless::send recipient peer "
		          "%s:%d not found from confirmed nodes list, not sent",
		          recp_orig->addr().get_host_addr(),
		          recp_orig->addr().get_port_number()));
		return;
	}
		
	// TODO
	// _sock->send(msg, msglen, recp->addr_user());
	_msg_send->reset();	
	_dheader.type(protocol::msg_send);
	// _dheader.node_recver(n->addr_direct());
	
	protocol::pdu::serialize_header(_msg_send, _dheader);
	// TODO would be better to use sendv interface instead of copying
	// the data around
	protocol::pdu::serialize_data  (_msg_send, msg, msglen);
	_send_msg(recp);
	
	_send_watch();
}

void 
sub_comms::send(const char *msg, size_t msglen) {
	// Obtain a list of all confirmed peers, creating a list
	// of their user dgram addresses

	_msg_send->reset();	
	_dheader.type(protocol::msg_send);
	
	protocol::pdu::serialize_header(_msg_send, _dheader);
	// TODO would be better to use sendv interface instead of copying
	// the data around
	protocol::pdu::serialize_data  (_msg_send, msg, msglen);
	
	// TODO
	_send_msg(this->peer_confirmed_begin(), this->peer_confirmed_end()); // plist.begin(), plist.end());

	_send_watch();
}

void
sub_comms::peer_confirmed_new(node *n) {
	ACE_DEBUG((LM_DEBUG, "serverless::sub_comms::peer_confirmed_new\n"));
	
	if (!_public_port) {
		// Request to know what our public port is
		_send_my_ip_req(n);
	}
	// Request list of known peers from 3 peers... should be enough?
	// if (_peers_nodes_requested < 3) {
	//	_send_nodes_req(n);
	//	_peers_nodes_requested++;
	// }
}

void 
sub_comms::_check_possible_peers() {
	node *n;
	while ((n = this->peer_possible_to_check())) {
		_send_join(n);
	}
}

void
sub_comms::notice_receive(int notice_id) {
	switch (notice_id) {
	case state::peer_new_possible:
		_check_possible_peers(); break;
	}
}

void
sub_comms::_open(u_short port) {
	ACE_DEBUG((LM_DEBUG, "sub_comms: opening udp port %d\n", port));
	reudp::addr_inet_type addr(port);
	_sock.open(addr);
	ACE_DEBUG((LM_DEBUG, "sub_comms: sock handle: %d\n", _sock.get_handle()));
	int blocking_mode;
	ACE::record_and_set_non_blocking_mode(_sock.get_handle(), blocking_mode);	
	_recv_watch();

	_opened_port = port;
	
	// _dheader.node_sender(this->addr_announce());
	_dheader.group(this->group_id());
}

void
sub_comms::_close() {
	if (_keepalive_timer != -1) {
		owner()->reactor()->cancel_timer(_keepalive_timer);
		_keepalive_timer = -1;		
	}
	
	if (_sock.get_handle() != ACE_INVALID_HANDLE) {
		// unregister all event handlers from reactor
		this->owner()->reactor()->
		      remove_handler(_sock.get_handle(), 
		                     ACE_Event_Handler::ALL_EVENTS_MASK |
		                     ACE_Event_Handler::DONT_CALL);
		this->owner()->reactor()->
		      cancel_timer(this);
	
		ACE_DEBUG((LM_DEBUG, "sub_comms: closing socket\n"));
		_sock.close();
	}
	_next_timeout = time_value_type::max_time;	

	_opened_port = 0;
	_public_port = 0;
}
		
void
sub_comms::_send_join(node *n) {
	ACE_DEBUG((LM_DEBUG, "sub_comms: sending join to node %s:%d\n",
	          n->addr_direct().get_host_addr(), n->addr_direct().get_port_number()));

	_msg_send->reset();	
	_dheader.type(protocol::msg_join);
	// _dheader.node_recver(n->addr_direct());
	
	protocol::pdu::serialize_header(_msg_send, _dheader);
	_send_msg(n);

	_send_watch();
}

void
sub_comms::_send_join_ack(node *n) {
	ACE_DEBUG((LM_DEBUG, "sub_comms: sending join_ack to node %s:%d\n",
	          n->addr_direct().get_host_addr(), n->addr_direct().get_port_number()));
	          
	_dheader.type(protocol::msg_join_ack);
	// _dheader.node_recver(n->addr_direct());
	
	_msg_send->reset();
	protocol::pdu::serialize_header(_msg_send, _dheader);
	_send_msg(n);
	_send_watch();
}

void
sub_comms::_send_leave() {
	ACE_DEBUG((LM_DEBUG, "sub_comms: sending leave to %d nodes\n",
	          this->peer_confirmed_size()));

	_msg_send->reset();	
	_dheader.type(protocol::msg_leave);

	protocol::pdu::serialize_header(_msg_send, _dheader);

	_send_msg(this->peer_confirmed_begin(), this->peer_confirmed_end());
	_send_watch();
}

void
sub_comms::_send_nodes_req(node *n) {
	ACE_DEBUG((LM_DEBUG, "sub_comms: sending nodes_req to node %s:%d\n",
	          n->addr_direct().get_host_addr(), n->addr_direct().get_port_number()));
	          
	_dheader.type(protocol::msg_nodes_req);
	// _dheader.node_recver(n->addr_direct());
	
	_msg_send->reset();
	protocol::pdu::serialize_header(_msg_send, _dheader);
	_send_msg(n);
	
	_send_watch();
}

void
sub_comms::_send_nodes_list(node *n) {
	ACE_DEBUG((LM_DEBUG, "sub_comms: sending nodes_list to node %s:%d\n",
	          n->addr_direct().get_host_addr(), n->addr_direct().get_port_number()));
	          
	_dheader.type(protocol::msg_nodes_list);
	// _dheader.node_recver(n->addr_direct());

	protocol::data_list<protocol::address_tuple> nlist;
	nlist.resize(this->peer_confirmed_size());
	std::transform(this->peer_confirmed_begin(),
	               this->peer_confirmed_end(),
	               nlist.begin(),
	               util::addr_node_pair_to_addr);
	// nlist.push_back(this->addr_announce());

	_msg_send->reset();
	protocol::pdu::serialize_header(_msg_send, _dheader);
	protocol::pdu::serialize_nodes_list(_msg_send, nlist); // , this->peers_complete());

	_send_msg(n);
	// _sock.send(_msg_send->base(), _msg_send->length(), n->addr_direct());
	
	_send_watch();
}

void
sub_comms::_send_my_ip_req(node *n) {
	ACE_DEBUG((LM_DEBUG, "serverless::sub_comms::_send_my_ip_req\n"));

	_dheader.type(protocol::msg_my_ip_req);
	_msg_send->reset();
	protocol::util::serialize(_msg_send, _dheader);

	_send_msg(n);
	// _sock.send(_msg_send->base(), _msg_send->length(), n->addr_direct());
	
	_send_watch();
}

void
sub_comms::_send_my_ip_resp(node *n) {
	ACE_DEBUG((LM_DEBUG, "serverless::sub_comms::_send_my_ip_resp\n"));

	// TODO away, on purpose return a faulty answer
#if 0
	addr_inet_type rigged(n->addr_direct());
	rigged.set_port_number(n->addr_direct().get_port_number() + 100);
	
	protocol::address_tuple at(rigged);
#endif
	protocol::address_tuple at(n->addr_direct());

	_dheader.type(protocol::msg_my_ip_resp);	
	_msg_send->reset();
	protocol::util::serialize(_msg_send, _dheader);
	protocol::util::serialize(_msg_send, at);

	_send_msg(n);
	// _sock.send(_msg_send->base(), _msg_send->length(), n->addr_direct());
	
	_send_watch();
}

int
sub_comms::_send_watch() {
	ACE_DEBUG((LM_DEBUG, "sub_comms: _send_watch\n"));
	// First send as much as possible, can't simply wait for
	// when the handle is writable because on Windows the event
	// is edge-triggered. This approach works for both edge and level
	// triggered styles.
	bool nts;
	while ((nts = _sock.needs_to_send())) {
		ACE_DEBUG((LM_DEBUG, "sub_comms: sending queued data\n"));
		ssize_t n =_sock.send(NULL, 0, reudp::addr_inet_type());
		if (n == -1) break;
	}
	
	if (nts) {
		ACE_DEBUG((LM_DEBUG, "sub_comms: registering waiting for socket " \
	                         "writable\n"));

		int ret = this->owner()->reactor()->schedule_wakeup(
		            _sock.get_handle(),
	                ACE_Event_Handler::WRITE_MASK);		
		if (ret == -1) throw internal_error(
	                                   "failed registering a write handler");
	} else {
		ACE_DEBUG((LM_DEBUG, "sub_comms: removing wait for socket " \
	                         "writable\n"));
		int ret = this->owner()->reactor()->cancel_wakeup(
		            _sock.get_handle(),
		            ACE_Event_Handler::WRITE_MASK);
		if (ret == -1) throw internal_error(
	                                   "failed removing a write handler");

		_send_timer_watch();	                
	}

	ACE_DEBUG((LM_DEBUG, "sub_comms: stopping waiting for socket " \
	          "writable\n"));
	// Peer timeout notification can be dispatched from _sock.send() or
	// _sock.needs_to_send, so check if that happened
	ACE_DEBUG((LM_DEBUG, "timeouted size: %d\n", _timeouted.size()));
	while (!_timeouted.empty()) {
		addr_inet_type a = _timeouted.front(); _timeouted.pop();
		this->peer_send_timeout(a);
	}

	return 0;
}

void
sub_comms::_send_timer_watch() {
	ACE_DEBUG((LM_DEBUG, "sub_comms: _send_timer_watch\n"));
	reudp::time_value_type nt = _sock.needs_to_send_when();

	if (nt != time_value_type::max_time) {
		if (nt >= _next_timeout) {
			ACE_DEBUG((LM_DEBUG, "sub_comms: not setting timer yet as " \
			                     "another one is expiring before this one: " \
			                     "timer/next: %d/%d\n", 
			                     nt.sec(), 
			                     _next_timeout.sec()));
			return;
		}
		_next_timeout = nt;
		
		// Unfortunately, reactor only allows timers relative to
		// current time.
		nt -= ACE_OS::gettimeofday();
		ACE_DEBUG((LM_DEBUG, "sub_comms::set_timer schedule " \
		           "timer at %ds%dus\n", nt.sec(), nt.usec()));
		int ret = owner()->reactor()->schedule_timer(this, NULL, nt);
		if (ret == -1) {
			throw internal_error("sub_comms: failed scheduling timer");
		}
	} else {
		_next_timeout = time_value_type::max_time;
	}
}

void
sub_comms::_recv_watch() {
	ACE_DEBUG((LM_DEBUG, "sub_comms: registering input handler\n"));

	int ret = this->owner()->reactor()->register_handler(_sock.get_handle(),
	                                           this,
	                                           ACE_Event_Handler::READ_MASK);
	ACE_DEBUG((LM_DEBUG, "sub_comms: register returned %d\n", ret));
}

int
sub_comms::handle_input(ACE_HANDLE handle) {
	// Well what now... I guess just call recv
	ACE_DEBUG((LM_DEBUG, "sub_comms: handle_input received for %d\n",
	          handle));
	// TODO recv until EWOULDBLOCK (to take into account
	// underlying buffering)?	          
	_recv();
	_send_watch();
	return 0;
}

int
sub_comms::handle_output(ACE_HANDLE handle) {
	// Well what now... I guess just call send without content.
	ACE_DEBUG((LM_DEBUG, "sub_comms: handle_output received for %d\n",
	          handle));
	// _sock.send(NULL, 0, reudp::addr_inet_type());
	return _send_watch();
}

int
sub_comms::handle_timeout(const ACE_Time_Value &now, const void *arg) {	
	if (arg == reinterpret_cast<void *>(&_keepalive_timer)) {
		ACE_DEBUG((LM_DEBUG, "sub_comms: keepalive timer expired\n"));
		// Collect all nodes that that have not been sent to during
		// the last keepalive interval
		ACE_Time_Value time_cutoff = now - ACE_Time_Value(25);
		
		std::list<node *> send_to;
		std::map<addr_inet_type, node *>::iterator i   = this->peer_confirmed_begin();
		std::map<addr_inet_type, node *>::iterator end = this->peer_confirmed_end();
		for (; i != end; i++) {
			node *n = i->second;
			if (n->last_send() <= time_cutoff)  {
				ACE_DEBUG((LM_DEBUG, "sub_comms: keepalive to %s:%d\n",
				           n->addr_direct().get_host_addr(),
				           n->addr_direct().get_port_number()));
				send_to.push_back(n);
			}
		}
		ACE_DEBUG((LM_DEBUG, "sub_comms: sending keepalive to %d nodes\n",
		           send_to.size()));
		if (send_to.size() > 0) {
			_msg_send->reset();
			_dheader.type(protocol::msg_keepalive);
			protocol::pdu::serialize_header(_msg_send, _dheader);
			_send_msg(send_to.begin(), send_to.end());
			_send_watch();
		}		
	} else {
		ACE_DEBUG((LM_DEBUG, "sub_comms: timer expired, trying to send\n"));
		// _sock.send(NULL, 0, reudp::addr_inet_type());
		_next_timeout = time_value_type::max_time;
		_send_watch();
	}
	return 0;
}

void 
sub_comms::_recv() {
	protocol::header      rhdr;
	reudp::addr_inet_type from;
	
	// TODO I guess...
	_msg_recv->reset();
	ssize_t nread = _sock.recv(_msg_recv->base(), _msg_recv->size(), from);
	ACE_DEBUG((LM_DEBUG, "sub_comms: nread %d\n", nread));
	if (nread < 0) {
		ACE_DEBUG((LM_DEBUG, "sub_comms: recv failure\n"));
		return;
	}
	if (nread == 0) {
		ACE_DEBUG((LM_DEBUG, "sub_comms: received 0 bytes, do nothing\n"));
		return;
	}
	
	_msg_recv->wr_ptr(nread);

	try {	
		protocol::pdu::deserialize_header(_msg_recv, &rhdr);
		if (!_dheader.compatible_with(rhdr)) {
			throw protocol_errorf(
				"sub_comms: read header not compatible " \
				"with this version. Read version: %x, our: %x, " \
				"group: '%s', our: '%s', magic: %d, our: %d\n",
				rhdr.version(),       _dheader.version(),
				rhdr.group().c_str(), _dheader.group().c_str(),
				rhdr.magic_id(),      _dheader.magic_id()
			);
		}

		node *n = this->peer_confirm(from, _dheader.time());
		if (!n) {
			throw internal_errorf(
			                 "sub_comms::recv could not get node " \
			                 "for IP (is it self?)\n");
		}
		switch (rhdr.type()) {
		case protocol::msg_join:
			_recv_join(n, rhdr); break;
		case protocol::msg_join_ack:
			_recv_join_ack(n, rhdr); break;
		case protocol::msg_leave:
			_recv_leave(n, rhdr); break;
		case protocol::msg_nodes_req:
			_recv_nodes_req(n, rhdr); break;
		case protocol::msg_nodes_list:
			_recv_nodes_list(n, rhdr); break;
		case protocol::msg_send:
			_recv_send(n, rhdr); break;
		case protocol::msg_my_ip_req:
			_recv_my_ip_req(n, rhdr); break;
		case protocol::msg_my_ip_resp:
			_recv_my_ip_resp(n, rhdr); break;
		case protocol::msg_keepalive:
			ACE_DEBUG((LM_DEBUG, "recv::keepalive\n")); break;
		default:
			throw protocol_errorf(
			                 "sub_comms: read type %d unrecognized\n",
			                 rhdr.type());
		}
	} catch (std::exception &e) {
		// For now at least, any exception raised will result in
		// discarding the packet
		// TODO this is not good since for example msg_send and even others
		// call user defined observers, which if they raise exceptions
		// will get caught here and discarded also.
		ACE_DEBUG((LM_DEBUG, "sub_comms: recv() error reading the packet " \
		           "discarding the result. Error is: '%s'\n", e.what()));		
	}
}

void 
sub_comms::_recv_join(node *n, 
                             const protocol::header &hdr) 
{
	ACE_DEBUG((LM_DEBUG, "netcomgrp::serverless join received\n"));
	_send_join_ack(n);
}

void 
sub_comms::_recv_join_ack(node *n, 
                                 const protocol::header &hdr) 
{
	ACE_DEBUG((LM_DEBUG, "netcomgrp::serverless join_ack received\n"));
	// Doesn't require any extra actions to be taken
}

void 
sub_comms::_recv_leave(node *n, 
                              const protocol::header &hdr) 
{
	ACE_DEBUG((LM_DEBUG, "netcomgrp::serverless leave received\n"));
	this->peer_left(n->addr_direct());
}

void 
sub_comms::_recv_nodes_req(node *n, 
                                  const protocol::header &hdr) 
{
	ACE_DEBUG((LM_DEBUG, "netcomgrp::serverless nodes_req received\n"));
	_send_nodes_list(n);
}

void 
sub_comms::_recv_nodes_list(node *n, 
                                  const protocol::header &hdr) 
{
	ACE_DEBUG((LM_DEBUG, "netcomgrp::serverless nodes_list received\n"));
	// bool is_complete = false;

	protocol::data_list<protocol::address_tuple> nlist;
	protocol::data_list<protocol::address_tuple>::iterator i;
	protocol::pdu::deserialize_nodes_list(_msg_recv, &nlist); // , &is_complete);

	ACE_DEBUG((LM_DEBUG, "netcomgrp::serverless received %d nodes\n",
	          nlist.size()));
	
	for (i = nlist.begin(); i != nlist.end(); i++) {
		this->peer_possible(i->addr(), 0);
	}
	
//	if (is_complete) {
		// A complete list of nodes has been received
//		this->peers_complete(true);
//	}
}

void 
sub_comms::_recv_send(node *n, 
                          const protocol::header &hdr) 
{
	ACE_DEBUG((LM_DEBUG, "netcomgrp::serverless send received\n"));

	// Directly after the header comes the payload.
	const char *data;
	size_t      datalen;

	protocol::pdu::deserialize_data(_msg_recv, &data, &datalen);

	ACE_DEBUG((LM_DEBUG, "netcomgrp::serverless received %d bytes\n",
	          datalen));

	if (n) this->peer_user_data_read(data, datalen, n);
}

void 
sub_comms::_recv_my_ip_req(node *n, const protocol::header &hdr) 
{
	ACE_DEBUG((LM_DEBUG, "netcomgrp::serverless::_recv_my_ip_req\n"));
	// bool is_complete = false;

	_send_my_ip_resp(n);
}

void 
sub_comms::_recv_my_ip_resp(node *n, const protocol::header &hdr) 
{
	ACE_DEBUG((LM_DEBUG, "netcomgrp::serverless::_recv_my_ip_resp\n"));
	// bool is_complete = false;
	protocol::address_tuple at;
	protocol::util::deserialize(_msg_recv, &at);
	
	ACE_DEBUG((LM_DEBUG, "netcomgrp::serverless::_recv_my_ip_resp: %s:%d (%d)\n",
	           at.addr().get_host_addr(), at.addr().get_port_number(),
	           _opened_port));
	
	if (_public_port && _public_port != at.addr().get_port_number()) {
		ACE_DEBUG((LM_WARNING, "netcomgrp::serverless::_recv_my_ip_resp: "
		            "%s:%d (%d), old public port %d differs\n",
		            at.addr().get_host_addr(), at.addr().get_port_number(),
		            _opened_port, _public_port));
	}
	
	_public_port = at.addr().get_port_number();
	
	if (_opened_port && _public_port != _opened_port) {
		ACE_DEBUG((LM_INFO, "netcomgrp::serverless::sub_comms: "
		          "outside peers seem to see different port than what was "
		          "opened, connection might be NATted. Activating keep alive "
		          "mode\n"));
		_keepalive_mode  = true;
		_keepalive_timer = owner()->reactor()->schedule_timer(
			this, &_keepalive_timer, 
			ACE_Time_Value(KEEPALIVE_INTERVAL), 
			ACE_Time_Value(KEEPALIVE_INTERVAL)
		);		
	}
}

int
sub_comms::_packet_done(int type, void *obj,
                               const void *buf,
                               size_t n,
                               const reudp::addr_type &to_r)
{
	ACE_DEBUG((LM_DEBUG, "netcomgrp::serverless: packet_done called\n"));
	sub_comms *self = reinterpret_cast<sub_comms *>(obj);
	
	const reudp::addr_inet_type &to = 
	  dynamic_cast<const reudp::addr_inet_type &>(to_r);
	  
	switch (type) {
	case reudp::packet_done::success:
		ACE_DEBUG((LM_DEBUG, "netcomgrp::serverless: " \
		          "packet sent successfully: " \
	              "%s:%d\n", 
	              to.get_host_addr(), 
	              to.get_port_number()));
		break;
	case reudp::packet_done::failure:
		ACE_DEBUG((LM_WARNING, "netcomgrp::serverless: " \
		          "failed to send to: " \
	              "%s:%d\n", 
	              to.get_host_addr(), 
	              to.get_port_number()));
		break;
	case reudp::packet_done::timeout:
		ACE_DEBUG((LM_DEBUG, "netcomgrp::serverless: " \
		          "packet send timeout: " \
	              "%s:%d\n", 
	              to.get_host_addr(), 
	              to.get_port_number()));
		// Timeout is usually called for example when reudp::send
		// is called. Since reudp::send_multi receives iterators
		// to confrmed peers, and peer_send_timeout invalidates
		// those peers, it is dangerous to call it directly
		// here. Instead push to timeouted list which is purged
		// in safe places after sends.

	    // reudp send is not re-entrant so unfortunately can't call
	    // _peer_send_timeout directly. Instead put to a list and later
	    // call it.
	    // self->_timeouted.push(to);
		ACE_DEBUG((LM_DEBUG, "timeouted size: %d\n", self->_timeouted.size()));
		self->_timeouted.push(to);
		break;
	}
	
	return 0;
}

} // ns serverless	
} // ns netcomgrp
