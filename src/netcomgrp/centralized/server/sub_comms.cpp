#include <ace/ACE.h>

#include <exception>
#include <algorithm>

#include "../../exception.h"
#include "../util.h"
#include "../protocol/data_list.h"
#include "../protocol/address_tuple.h"
#include "../protocol/pdu.h"

#include "sub_comms.h"

namespace netcomgrp {
namespace centralized {
namespace server {

sub_comms::sub_comms(group *owner) 
	: _owner(owner) // : sub_system(owner, "dgram_signal")
{
	// Reserve enough for one UDP packet, taking into
	// account IP+UDP+ReUDP etc. headers take some space
	_msg      = new reudp::msg_block_type(65000);
	_msg_busy = false;
	_sock.packet_done_cb(_packet_done, this);
	_next_timeout = time_value_type::max_time;
}

sub_comms::~sub_comms() {
	_close();
	// TODO deregister handler?
	delete _msg;
}

void
sub_comms::join(const char *grp_id) {	
	// TODO configurable port number(s)
	u_short port = atoi(grp_id);
	if (port == 0) {
		throw call_errorf(
		      "group::server::join group id must be a port number to bind, " \
		      "got %s", grp_id
		);
	}
	_open(port);
}

void
sub_comms::leave() {
	// _send_leave();
	// this->notify(state::signal_left);
	_close();
}

void 
sub_comms::send(const char *msg, size_t msglen, const node *recp) {
	
}

void 
sub_comms::send(const char *msg, size_t msglen) {
	
}

void
sub_comms::send_timeout(const node *n) {
	// At least for now, just send a leave
	_send_left(n->addr());
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

	// _dheader.node_sender(this->addr_announce());
	// _dheader.group(this->group_id());
}

void
sub_comms::_close() {
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

void 
sub_comms::_send_msg_all() {
/*	std::list<addr_inet_type> plist;
	plist.resize(_owner->_peer_participants_size());
	std::transform(_owner->_peer_participants_begin(),
	               _owner->_peer_participants_end(),
	               plist.begin(),
	               util::addr_node_pair_to_addr);
	*/
	ACE_DEBUG((LM_DEBUG, "sub_comms: sending all participants data of " \
	          "length %d\n", _msg->length()));
	// _sock.send_multi(_msg->rd_ptr(), _msg->length(), n->addr());	
	_sock.send_multi(_msg->rd_ptr(), _msg->length(), 
	                _owner->_peer_participants_begin(),
	                _owner->_peer_participants_end(),
	                util::addr_node_pair_to_addr());	
	_msg->rd_ptr(_msg->length());
}

void 
sub_comms::_send_msg(const addr_inet_type &to) {
	ACE_DEBUG((LM_DEBUG, "sub_comms: sending data of length %d to %s:%d\n",
	          _msg->length(),
	          to.get_host_addr(), to.get_port_number()));
	
	ACE_DEBUG((LM_DEBUG, "sub_comms: sending participant data of " \
	          "length %d\n", _msg->length()));
	_sock.send(_msg->rd_ptr(), _msg->length(), to);
	_msg->rd_ptr(_msg->length());
}

void
sub_comms::_msg_prepare_resend(const addr_inet_type &addr_sender, 
                               const void *data, 
                               size_t datalen) 
{
	// No _msg_start_use here because it is a resend, _msg_start_use has
	// been done when the msg was received, this is just continuing the
	// use.
	_msg->reset();
	_dheader.type(protocol::msg_send);
	_dheader.addr(addr_sender);

	protocol::pdu::serialize_header(_msg, _dheader);
	// Ugly hack, but since when relaying a send from a node
	// to other nodes, the data section should be in the same position
	// in the message block so, instead of copying
	// let it be there.
	// Two first bytes are the length of the data block
	_msg->wr_ptr(2);
	if (data    == _msg->wr_ptr() &&
	    datalen  < _msg->space())
	{
		_msg->wr_ptr(datalen);
	} else {
		throw protocol_errorf(
			"centralized::_msg_prepare_resend: resending data " \
			"failed because data positions differ or not enough " \
			"space!? Pointers: %d/%d, length of data/available: " \
			"%d/%d\n",
			data,    _msg->wr_ptr(),
			datalen, _msg->space()
		);
	}
}

void
sub_comms::_resend(const addr_inet_type &from, const void *data, size_t datalen) {
	_msg_prepare_resend(from, data, datalen);
	_send_msg_all();
}

void
sub_comms::_resend(const addr_inet_type &from, const void *data, size_t datalen,
                   std::list<protocol::address_tuple> &nlist) 
{
	_msg_prepare_resend(from, data, datalen);
    
	std::list<protocol::address_tuple>::iterator i = nlist.begin();
	std::list<protocol::address_tuple>::iterator rm_i;
	while (i != nlist.end()) {
		rm_i = i++;
		if (!_owner->_peer_find(rm_i->addr())) {
			// The user was not in the participants container, so send 
			// back "left" to the sender without sending the data
			_to_addr_timeouted.push(rm_i->addr());
			if (_to_addr_timeouted.size() == 1) _to_addr = from;
			nlist.erase(rm_i);
		}
	}

	_sock.send_multi(_msg->rd_ptr(), _msg->length(),
	                 nlist.begin(),
	                 nlist.end(),
	                 util::addr_tuple_to_addr());
}

void
sub_comms::_send_join_ack(const addr_inet_type &a) {
	ACE_DEBUG((LM_DEBUG, "sub_comms: sending join_ack to node %s:%d\n",
	          a.get_host_addr(), a.get_port_number()));
	          
	_dheader.type(protocol::msg_join_ack);
	// Client's (external) address is reported back in ack
	_dheader.addr(a);
	
	_msg_start_use();
	protocol::util::serialize(_msg, _dheader);
	protocol::util::serialize(_msg, protocol::address_tuple(a));
	_sock.send(_msg->base(), _msg->length(), a);
	_msg_stop_use();
	_send_watch();
}

void
sub_comms::_send_nodes_list(const addr_inet_type &a) {
	ACE_DEBUG((LM_DEBUG, "sub_comms: sending nodes_list to node %s:%d\n",
	          a.get_host_addr(), a.get_port_number()));
	          
	_dheader.type(protocol::msg_nodes_list);
	_dheader.addr(addr_inet_type());
	protocol::data_list<protocol::address_tuple> nlist;
	nlist.resize(_owner->_peer_participants_size());
	std::transform(_owner->_peer_participants_begin(),
	               _owner->_peer_participants_end(),
	               nlist.begin(),
	               util::addr_node_pair_to_addr());
	// nlist.push_back(this->addr_announce());
	
	_msg_start_use();
	protocol::pdu::serialize_header(_msg, _dheader);
	protocol::pdu::serialize_nodes_list(_msg, nlist); // , this->peers_complete());
	
	_sock.send(_msg->base(), _msg->length(), a);
	_msg_stop_use();
	
	_send_watch();
}

void 
sub_comms::_send_left(const addr_inet_type &a) {
	ACE_DEBUG((LM_DEBUG, "sub_comms: sending left about node %s:%d to all\n",
	          a.get_host_addr(), a.get_port_number()));

	_msg_start_use();
	_dheader.type(protocol::msg_node_left);
	_dheader.addr(a);

	protocol::pdu::serialize_header(_msg, _dheader);

	_send_msg_all();
	_msg_stop_use();
	_send_watch();
}	

void 
sub_comms::_send_left(const addr_inet_type &who, const addr_inet_type &to) {
	ACE_DEBUG((LM_DEBUG, "sub_comms: sending left about node %s:%d to %s:%d\n",
	          who.get_host_addr(), who.get_port_number(),
	          to.get_host_addr(), to.get_port_number()));

	_msg_start_use();
	_dheader.type(protocol::msg_node_left);
	_dheader.addr(who);

	protocol::pdu::serialize_header(_msg, _dheader);

	_send_msg(to);
	_msg_stop_use();
	_send_watch();
}	

void
sub_comms::_send_joined(const addr_inet_type &a) {
	ACE_DEBUG((LM_DEBUG, "sub_comms: sending joined about node %s:%d to all\n",
	          a.get_host_addr(), a.get_port_number()));

	_msg_start_use();
	_dheader.type(protocol::msg_node_joined);
	_dheader.addr(a);
	
	protocol::pdu::serialize_header(_msg, _dheader);

	_send_msg_all();
	_msg_stop_use();
	_send_watch();
}

int
sub_comms::_send_watch() {
	ACE_DEBUG((LM_DEBUG, "sub_comms: _send_watch\n"));
				
	// TODO register/unregister only if not so already
	
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

		int ret = this->owner()->reactor()->register_handler(
		            _sock.get_handle(),
	                this,
	                ACE_Event_Handler::WRITE_MASK);
	    if (ret == -1) 
	    	throw unexpected_error("failed registering a write handler");
	} else {
		ACE_DEBUG((LM_DEBUG, "sub_comms: removing wait for socket " \
	                         "writable\n"));
		int ret = this->owner()->reactor()->remove_handler(
		            _sock.get_handle(),
	                ACE_Event_Handler::WRITE_MASK |
	                ACE_Event_Handler::DONT_CALL);
	    if (ret == -1) 
	    	throw unexpected_error("failed removing a write handler");

		_send_timer_watch();	                
	}
	ACE_DEBUG((LM_DEBUG, "sub_comms: stopping waiting for socket " \
	          "writable\n"));
	          
	// Peer timeout notification can be dispatched from _sock.send() or
	// _sock.needs_to_send, so check if that happened
	ACE_DEBUG((LM_DEBUG, "timeouted size: %d\n", _timeouted.size()));
	while (!_timeouted.empty()) {
		addr_inet_type a = _timeouted.front(); _timeouted.pop();
		_owner->_peer_send_timeout(a);
	}

	ACE_DEBUG((LM_DEBUG, "to addr timeouted size: %d\n", _to_addr_timeouted.size()));
	while (!_to_addr_timeouted.empty()) {
		addr_inet_type a = _to_addr_timeouted.front(); _to_addr_timeouted.pop();
		_send_left(a, _to_addr);
	}
	          
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
	ACE_DEBUG((LM_DEBUG, "sub_sgram_signal: timer expired, trying to send\n"));
	// _sock.send(NULL, 0, reudp::addr_inet_type());
	_next_timeout = time_value_type::max_time;
	_send_watch();
	return 0;
}

void 
sub_comms::_recv() {
	protocol::header      rhdr;
	reudp::addr_inet_type from;
	
	// TODO I guess...
	_msg_start_use();
	ssize_t nread = _sock.recv(_msg->base(), _msg->size(), from);
	ACE_DEBUG((LM_DEBUG, "sub_comms: nread %d\n", nread));
	if (nread < 0) {
		_msg_stop_use();
		ACE_DEBUG((LM_DEBUG, "sub_comms: recv failure\n"));
		return;
	}
	if (nread == 0) {
		_msg_stop_use();
		ACE_DEBUG((LM_DEBUG, "sub_comms: received 0 bytes, do nothing\n"));
		return;
	}
	
	_msg->wr_ptr(nread);

	try {	
		protocol::pdu::deserialize_header(_msg, &rhdr);
		if (!_dheader.compatible_with(rhdr)) {
			throw protocol_errorf(
				"sub_comms: read header not compatible " \
				"with this version. Read version: %x, our: %x\n" \
				"magic id: %x, our: %x\n",
				rhdr.version(),  _dheader.version(),
				rhdr.magic_id(), _dheader.magic_id()
			);
		}

		switch (rhdr.type()) {
		case protocol::msg_join:
			_recv_join(from, rhdr); break;
		case protocol::msg_leave:
			_recv_leave(from, rhdr); break;
		case protocol::msg_nodes_req:
			_recv_nodes_req(from, rhdr); break;
		case protocol::msg_send:
			_recv_send(from, rhdr); break;
		case protocol::msg_keepalive:
			ACE_DEBUG((LM_DEBUG, "sub_comms: keepalive received\n")); 
			_msg_stop_use();
			break;
		default:
			throw protocol_errorf("sub_comms: read type %d unrecognized",
			                     rhdr.type());
		}
	} catch (std::exception &e) {
		// For now at least, any exception raised will result in
		// discarding the packet
		ACE_DEBUG((LM_DEBUG, "sub_comms: recv() error reading the packet " \
		           "from %s:%u discarding the result. Error is: '%s'\n", 
		           from.get_host_addr(), from.get_port_number(),
		           e.what()));
		_msg_stop_use();
	}
}

void 
sub_comms::_recv_join(const addr_inet_type &f, 
                      const protocol::header &hdr) 
{
	ACE_DEBUG((LM_DEBUG, "netcomgrp::centralized join received\n"));
	bool is_new = false;
	_owner->_peer_confirm(f, &is_new);

	_msg_stop_use();
	_send_join_ack(f);
	if (is_new) _send_joined(f);
}

void 
sub_comms::_recv_leave(const addr_inet_type &f, 
                       const protocol::header &hdr) 
{
	ACE_DEBUG((LM_DEBUG, "netcomgrp::centralized leave received\n"));
	_msg_stop_use();
	if (_owner->_peer_left(f)) {
		_send_left(f);
	}
}

void 
sub_comms::_recv_nodes_req(const addr_inet_type &f, 
                           const protocol::header &hdr) 
{
	ACE_DEBUG((LM_DEBUG, "netcomgrp::centralized nodes_req received\n"));
	_msg_stop_use();
	_send_nodes_list(f);
}

void 
sub_comms::_recv_send(const addr_inet_type &f, 
                      const protocol::header &hdr) 
{
	ACE_DEBUG((LM_DEBUG, "netcomgrp::centralized send received\n"));

	bool is_new = false;
	_owner->_peer_confirm(f, &is_new);
	
	// After the header there is the message payload data to be sent, 
	// and after that a list of addresses where the message
	// payload should be sent to. 0 addresses means send to everyone on
	// the group.
	const char *data;
	size_t      datalen;
	protocol::data_list<protocol::address_tuple> nlist;	
	protocol::pdu::deserialize_send(_msg, &data, &datalen, &nlist);

	ACE_DEBUG((LM_DEBUG, "netcomgrp::centralized send received %d nodes, " \
	          "with payload of size %d\n",
	          nlist.size(), datalen));

	// Warning, _msg is reset and overwritten after _resend calls.	
	if (nlist.size() == 0) {
		_resend(f, data, datalen);
	} else {
		_resend(f, data, datalen, nlist);
	}
	_msg_stop_use();

	if (is_new) _send_joined(f);
	else
		_send_watch();	
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
	    // reudp send is not re-entrant so unfortunately can't call
	    // _peer_send_timeout directly, but put to a list and later
	    // call it.
	    // self->_timeouted.push(to);
		ACE_DEBUG((LM_DEBUG, "timeouted size: %d\n", self->_timeouted.size()));
		self->_timeouted.push(to);
		break;
	}

	ACE_DEBUG((LM_DEBUG, "netcomgrp::centralized: packet_done exiting\n"));	
	return 0;
}

} // ns server
} // ns centralized
} // ns netcomgrp
