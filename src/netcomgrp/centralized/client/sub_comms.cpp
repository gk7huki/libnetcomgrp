#include <ace/ACE.h>

#include <exception>
#include <algorithm>

#include <strings.h>
#include "../../exception.h"
#include "../util.h"
#include "../protocol/data_list.h"
#include "../protocol/address_tuple.h"
#include "../protocol/pdu.h"

#include "sub_comms.h"

// 45sec keepalive interval if seems like connection
// is behind NAT
#define KEEPALIVE_INTERVAL 45

namespace netcomgrp {
namespace centralized {
namespace client {

sub_comms::sub_comms(group *owner) 
  : _owner(owner), 
	_exception_command(ex_none),
    _opened_port(0), 
    _public_port(0),
    _keepalive_mode(false),
    _keepalive_timer(-1)	
{
	// Reserve enough for one UDP packet, taking into
	// account IP+UDP+ReUDP etc. headers take some space
	_msg      = new reudp::msg_block_type(65000);
	_msg_recv = new reudp::msg_block_type(65000);
	
	_sock.packet_done_cb(_packet_done, this);
	_next_timeout = time_value_type::max_time;
}

sub_comms::~sub_comms() {
	_close();
	// TODO deregister handler?
	delete _msg;
	delete _msg_recv;
}

int
sub_comms::join(const char *grp_id) {	
	// TODO configurable port number(s)
	ACE_DEBUG((LM_DEBUG, "centralized::client: setting server address to %s\n",
	           grp_id));
	
	_addr_server.set(grp_id);

	ACE_DEBUG((LM_DEBUG, "centralized::client: that's it\n"));
	
	if (_addr_server.get_ip_address() == INADDR_ANY ||
	    _addr_server.get_ip_address() == INADDR_NONE) 
	{
		return 1;
// For some reason this throw causes major problems in Win32
// when attempting joining repeatedly. Instead of a throw,
// an error return is used.
/*		throw address_resolution_errorf(
			"group::centralized::client invalid group id %s, " \
			"should be server_ip:port", grp_id
		);*/
	}
	ACE_DEBUG((LM_DEBUG, "centralized::client: server is %s:%u\n",
	          _addr_server.get_host_addr(), _addr_server.get_port_number()));
	          
	_open();
	
	_send_join();
	return 0;
}

void
sub_comms::leave() {
	_send_leave();
	_close();
}

void sub_comms::send(const char *msg, size_t msglen,
                     const protocol::data_list<protocol::address_tuple> &nlist)
{
	ACE_DEBUG((LM_DEBUG, "centralized::sub_comms: sending to %d nodes msg of " \
	          "size %d\n", nlist.size(), msglen));
	          
	_msg->reset();
	_dheader.type(protocol::msg_send);
	_dheader.addr(addr_inet_type());
	
	protocol::pdu::serialize_header(_msg, _dheader);
	protocol::pdu::serialize_send(_msg, msg, msglen, nlist);
	
	// NULL terminate for debugging help
	// *(_msg->wr_ptr()) = 0; _msg->wr_ptr(1);
	
	_send_server();
	_send_watch();
}

void sub_comms::send(const char *msg, size_t msglen,
                     std::list<const netcomgrp::node *>::const_iterator start,
                     std::list<const netcomgrp::node *>::const_iterator stop)
{
	// Generate the list of addresses to send to
	protocol::data_list<protocol::address_tuple> nlist;
	std::insert_iterator<protocol::data_list<protocol::address_tuple> > 
	  nlist_insert(nlist, nlist.begin());
	std::transform(start, stop, nlist_insert, util::node_to_addr());

	send(msg, msglen, nlist);		
}

void 
sub_comms::send(const char *msg, size_t msglen, const node *recp) {
	ACE_DEBUG((LM_DEBUG, "sub_comms: sending data to node %s:%d\n",
	          recp->addr().get_host_addr(), recp->addr().get_port_number()));

	protocol::data_list<protocol::address_tuple> nlist;
	// nlist.insert(nlist.end(), recp->addr());
	nlist.push_back(recp->addr());
	
	send(msg, msglen, nlist);
	ACE_DEBUG((LM_DEBUG, "sub_comms::send returning\n"));
}

void 
sub_comms::send(const char *msg, size_t msglen) {
	// When sending to all, empty list is enough to signal that
	protocol::data_list<protocol::address_tuple> nlist;
	send(msg, msglen, nlist);
}

void
sub_comms::_open() {
	ACE_DEBUG((LM_DEBUG, "centralized::sub_comms: opening udp socket\n"));
	reudp::addr_inet_type local_addr;
	_sock.open(reudp::addr_inet_type());
	_sock.get_local_addr(local_addr);
	_opened_port = local_addr.get_port_number();
	ACE_DEBUG((LM_DEBUG, "centralized::sub_comms: "
	          "sock handle/port_number: %d/%d\n", 
	          _sock.get_handle(),
	          _opened_port));
	int blocking_mode;
	ACE::record_and_set_non_blocking_mode(_sock.get_handle(), blocking_mode);	
	_recv_watch();

	// _dheader.node_sender(this->addr_announce());
	// _dheader.group(this->group_id());
}

void
sub_comms::_close() {
	if (_keepalive_timer != -1) {
		owner()->reactor()->cancel_timer(_keepalive_timer);
		_keepalive_timer = -1;
	}
	
	// unregister all event handlers from reactor
	this->owner()->reactor()->
	      remove_handler(_sock.get_handle(), 
	                     ACE_Event_Handler::ALL_EVENTS_MASK |
	                     ACE_Event_Handler::DONT_CALL);
	this->owner()->reactor()->
	      cancel_timer(this);

	// ACE_DEBUG((LM_DEBUG, "sub_comms: closing socket\n"));
	_sock.close();
	_next_timeout = time_value_type::max_time;
}

inline void
sub_comms::_send_server() {
	ACE_DEBUG((LM_DEBUG, "sub_comms: sending to server %s:%u\n",
	          _addr_server.get_host_addr(), _addr_server.get_port_number()));
	
	_sock.send(_msg->base(), _msg->length(), _addr_server);
#if 0
	// TODO, right now ace works such that resetting timer inteval
	// does not do what is expected. See for example here description:
	// http://tinyurl.com/h2bvn
	if (_keepalive_timer != -1) {
		ACE_DEBUG((LM_DEBUG, "sub_comms: rescheduling keepalive timer\n"));
		int ret = owner()->reactor()->reset_timer_interval(
			_keepalive_timer, ACE_Time_Value(KEEPALIVE_INTERVAL)
		);
		ACE_DEBUG((LM_DEBUG, "sub_comms: keepalive result %d\n", ret));
		
	}
#endif

}

void
sub_comms::_send_join() {
	ACE_DEBUG((LM_DEBUG, "sub_comms: sending join to server %s:%d\n",
	          _addr_server.get_host_addr(), _addr_server.get_port_number()));

	_msg->reset();	
	_dheader.type(protocol::msg_join);
	_dheader.addr(addr_inet_type());
	
	protocol::pdu::serialize_header(_msg, _dheader);
	_send_server();
	_send_watch();
}

void
sub_comms::_send_leave() {
	ACE_DEBUG((LM_DEBUG, "sub_comms: sending leave to server %s:%d\n",
	          _addr_server.get_host_addr(), _addr_server.get_port_number()));

	_msg->reset();	
	_dheader.type(protocol::msg_leave);
	_dheader.addr(addr_inet_type());
	
	protocol::pdu::serialize_header(_msg, _dheader);
	_send_server();
	_send_watch();
}

void
sub_comms::_send_nodes_req() {
	ACE_DEBUG((LM_DEBUG, "sub_comms: sending nodes_req\n"));
	         
	
	_dheader.type(protocol::msg_nodes_req);
	_dheader.addr(addr_inet_type());
	
	_msg->reset();
	protocol::pdu::serialize_header(_msg, _dheader);
	_send_server();
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
	    if (ret == -1) 
	    	throw unexpected_error("failed registering a write handler");
		return 0;
	} else {
		ACE_DEBUG((LM_DEBUG, "sub_comms: removing wait for socket " \
	                         "writable, owner: %d\n", this->owner()));
		ACE_DEBUG((LM_DEBUG, "sub_comms: owner's reactor: %d\n",
	                         this->owner()->reactor()));
		int ret = this->owner()->reactor()->cancel_wakeup(
		            _sock.get_handle(),
	                ACE_Event_Handler::WRITE_MASK);

	    if (ret == -1) 
	    	throw unexpected_error("failed removing a write handler");

		_send_timer_watch();	                
	    return 0;		
	}
	
	// Should not come this far ever
	return 0;
	// TODO for now nothing DUH!!!!!!!!	
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
			throw unexpected_error("sub_comms: failed scheduling timer");
		}
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
		_msg->reset();	
		_dheader.type(protocol::msg_keepalive);
		_dheader.addr(addr_inet_type());
		
		protocol::pdu::serialize_header(_msg, _dheader);
		_send_server();
		_send_watch();		
	} else {	
		ACE_DEBUG((LM_DEBUG, "sub_comms: timer expired, trying to send\n"));
		// _sock.send(NULL, 0, reudp::addr_inet_type());
		_next_timeout = time_value_type::max_time;
		_send_watch();
	}
	return 0;
}

int
sub_comms::handle_exception(ACE_HANDLE handle) {	
	ACE_DEBUG((LM_DEBUG, "sub_comms: received exception command %d\n", 
	          _exception_command));
	switch (_exception_command) {
	case ex_close_socket:
		ACE_DEBUG((LM_DEBUG, "sub_comms: closing socket\n"));
		_close();
		break;
	default:
		break;
	}
	
	_exception_command = ex_none;
	return 0;
}

int
sub_comms::handle_close(ACE_HANDLE handle, ACE_Reactor_Mask m) {	
	ACE_DEBUG((LM_DEBUG, "sub_comms::handle_close(%d, %d) ???\n",
	          handle, m));
	return ACE_Event_Handler::handle_close(handle, m);
}

void 
sub_comms::_recv() {
	protocol::header      rhdr;
	reudp::addr_inet_type sender;
	reudp::addr_inet_type from;
	
	// TODO I guess...
	_msg_recv->reset();
	ssize_t nread = _sock.recv(_msg_recv->base(), _msg_recv->size(), sender);
	ACE_DEBUG((LM_DEBUG, "sub_comms::_recv nread %d\n", nread));
	if (nread < 0) {
		ACE_DEBUG((LM_DEBUG, "sub_comms: recv failure\n"));
		return;
	}
	if (nread == 0) {
		ACE_DEBUG((LM_DEBUG, "sub_comms: received 0 bytes, do nothing\n"));
		return;
	}
	
	_msg_recv->wr_ptr(nread);
	// To ease debugging, ensure NULL termination
	if (_msg_recv->length() < _msg_recv->size()) {
		*(_msg_recv->wr_ptr()) = 0;
	}
	try {	
		protocol::pdu::deserialize_header(_msg_recv, &rhdr);
		if (!_dheader.compatible_with(rhdr)) {
			throw protocol_errorf(
				"sub_comms: read header not compatible " \
				"with this version. Read version: %x, our: %x\n" \
				"magic id: %x, our: %x\n",
				rhdr.version(),  _dheader.version(),
				rhdr.magic_id(), _dheader.magic_id()
			);
		}
		
		from = rhdr.addr().addr();

		if (rhdr.type() != protocol::msg_join_ack) {
			// Discard all received packets until join has been completed
			if (_owner->in_state() != group::joined) {
				ACE_DEBUG((LM_DEBUG, "group::centralized::client: discarding " \
				          "received packet since not joined yet\n"));
				return;
			}
			// Also discard all packets that are from ourself
/*			if (!_owner->_self || *_owner->_self == from) {
				ACE_DEBUG((LM_DEBUG, "group::centralized::client: discarding " \
				          "received packet since it is from ourself\n"));
				return;
			} */
		}
				
		switch (rhdr.type()) {
		case protocol::msg_join_ack:
			_recv_join_ack(from, rhdr); break;
		case protocol::msg_node_joined:
			_recv_node_joined(from, rhdr); break;
		case protocol::msg_node_left:
			_recv_node_left(from, rhdr); break;
		case protocol::msg_nodes_list:
			_recv_nodes_list(from, rhdr); break;
		case protocol::msg_send:
			_recv_send(from, rhdr); break;
		default:
			throw protocol_errorf("sub_comms: read type %d unrecognized\n",
			                      rhdr.type());
		}
	} catch (std::exception &e) {
		// For now at least, any exception raised will result in
		// discarding the packet
		ACE_DEBUG((LM_DEBUG, "sub_comms: recv() error reading the packet " \
		           "from %s:%u discarding the result. Error is: '%s'\n", 
		           sender.get_host_addr(), sender.get_port_number(),
		           e.what()));
	}
	ACE_DEBUG((LM_DEBUG, "sub_comms::_recv returning\n"));	
}

void 
sub_comms::_recv_join_ack(const addr_inet_type &from, 
                          const protocol::header &hdr) 
{
	ACE_DEBUG((LM_DEBUG, "netcomgrp::centralized join_ack received\n"));
	_owner->_peer_confirm(from, true);

	// The server sends our address back, check that if we're natted.
	protocol::address_tuple at;
	protocol::util::deserialize(_msg_recv, &at); // , &is_complete);

	ACE_DEBUG((LM_DEBUG, "netcomgrp::centralized::_recv_my_ip_resp: %s:%d (%d)\n",
	           at.addr().get_host_addr(), at.addr().get_port_number(),
	           _opened_port));
	
	if (_public_port && _public_port != at.addr().get_port_number()) {
		ACE_DEBUG((LM_WARNING, "netcomgrp::centralized::_recv_my_ip_resp: "
		            "%s:%d (%d), old public port %d differs\n",
		            at.addr().get_host_addr(), at.addr().get_port_number(),
		            _opened_port, _public_port));
	}
	
	_public_port = at.addr().get_port_number();
	
	if (_opened_port && _public_port != _opened_port) {
		ACE_DEBUG((LM_INFO, "netcomgrp::centralized::sub_comms: "
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
	
	
	// When ack received, request node list from the central server
	_send_nodes_req();
}

void 
sub_comms::_recv_node_joined(const addr_inet_type &from, 
                              const protocol::header &hdr) 
{
	ACE_DEBUG((LM_DEBUG, "netcomgrp::centralized node joined received\n"));
	_owner->_peer_confirm(from, false);
	ACE_DEBUG((LM_DEBUG, "netcomgrp::centralized node joined returning\n"));
}

void 
sub_comms::_recv_node_left(const addr_inet_type &from, 
                           const protocol::header &hdr) 
{
	ACE_DEBUG((LM_DEBUG, "netcomgrp::centralized leave received\n"));
	_owner->_peer_remove(from);
}

void 
sub_comms::_recv_nodes_list(const addr_inet_type &from, 
                            const protocol::header &hdr) 
{
	ACE_DEBUG((LM_DEBUG, "netcomgrp::centralized nodes_list received\n"));

	protocol::data_list<protocol::address_tuple> nlist;
	protocol::data_list<protocol::address_tuple>::iterator i;
	protocol::pdu::deserialize_nodes_list(_msg_recv, &nlist); // , &is_complete);

	_owner->_peer_participants(nlist.begin(), nlist.end(),
	                           util::addr_tuple_to_addr());	                           	
}

void 
sub_comms::_recv_send(const addr_inet_type &from, 
                      const protocol::header &hdr) 
{
	ACE_DEBUG((LM_DEBUG, "netcomgrp::centralized send received\n"));

	node *n = _owner->_peer_confirm(from, false);
	
	// Directly after the header comes the payload.
	const char *data;
	size_t      datalen;

	protocol::pdu::deserialize_data(_msg_recv, &data, &datalen);

	ACE_DEBUG((LM_DEBUG, "netcomgrp::centralized received %d bytes\n",
	          datalen));
	
	_owner->_peer_user_data_read(data, datalen, n);	
}

int
sub_comms::_packet_done(int type, void *obj,
                               const void *buf,
                               size_t n,
                               const reudp::addr_type &to_r)
{
	ACE_DEBUG((LM_DEBUG, "netcomgrp::centralized: packet_done called\n"));
	sub_comms *self = reinterpret_cast<sub_comms *>(obj);
	
	const reudp::addr_inet_type &to = 
	  dynamic_cast<const reudp::addr_inet_type &>(to_r);
	  
	switch (type) {
	case reudp::packet_done::success:
		ACE_DEBUG((LM_DEBUG, "netcomgrp::centralized: " \
		          "packet sent successfully: " \
	              "%s:%d\n", 
	              to.get_host_addr(), 
	              to.get_port_number()));
		break;
	case reudp::packet_done::failure:
		ACE_DEBUG((LM_WARNING, "netcomgrp::centralized: " \
		          "failed to send to: " \
	              "%s:%d\n", 
	              to.get_host_addr(), 
	              to.get_port_number()));
		break;
	case reudp::packet_done::timeout:
		ACE_DEBUG((LM_DEBUG, "netcomgrp::centralized: " \
		          "packet send timeout: " \
	              "%s:%d\n", 
	              to.get_host_addr(), 
	              to.get_port_number()));
		self->_owner->_peer_send_timeout(to);
		self->_exception_command = ex_close_socket;
		self->_owner->reactor()->notify(self);
		break;
	}
	
	return 0;
}

} // ns server
} // ns centralized
} // ns netcomgrp
