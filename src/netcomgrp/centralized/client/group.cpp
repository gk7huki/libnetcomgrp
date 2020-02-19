#include <utility>

#include "../../exception.h"
#include "group.h"

#include "sub_comms.h"

namespace netcomgrp {
namespace centralized {
namespace client {

group::group() {
	ACE_DEBUG((LM_DEBUG, "netcomgrp::client::group constructor\n"));
	
	// Unless otherwise instructed, use ACE's system wide reactor
	_reactor     = reactor_type::instance();	
	_sub_comms   = new sub_comms(this);
	_self        = NULL;
}

group::~group() {
	delete _sub_comms;
	_peers_clear();
}

void 
group::join(const char *grp_id) {
	ACE_DEBUG((LM_DEBUG, "centralized::client: join\n"));
	observer_notifier()->state_changed(group::joining);
	int err = _sub_comms->join(grp_id);
	if (err != 0) 
	{
		// This  is mainly because group id address
		// resolution might fail
		observer_notifier()->state_changed(group::not_joined);
	}
	ACE_DEBUG((LM_DEBUG, "centralized::client: calling sub_comms join\n"));
}

void 
group::leave() {
	observer_notifier()->state_changed(group::leaving);
	_sub_comms->leave();
	_peers_clear();	
	observer_notifier()->state_changed(group::not_joined);
}

void
group::send(const char *msg, size_t msglen, const netcomgrp::node *recp) {	
	const node *n = dynamic_cast<const node *>(recp);
	if (!n) {
		ACE_DEBUG((LM_WARNING, "netcomgrp::client::group::send " \
		          "received a node ptr that was not from client group"));
		return;
	}
	// Don't send unless joined
	if (in_state() == group::joined) {
		_sub_comms->send(msg, msglen, n);
	} else {
		ACE_DEBUG((LM_WARNING, "netcomgrp::client::group::send " \
		          "tried to send but in joined state, ignored, "
		          "current state is '%s'\n", in_state_str()));
	}		
}

void
group::send(const char *msg, size_t msglen) {
	if (in_state() == group::joined) {
		_sub_comms->send(msg, msglen);
	} else {
		ACE_DEBUG((LM_WARNING, "netcomgrp::client::group::send " \
		          "tried to send but in joined state, ignored, "
		          "current state is '%s'\n", in_state_str()));
	}		
}

size_t
group::nodes_size() const {
	return _participants.size();
}

size_t
group::nodes(std::list<const netcomgrp::node *> *l) {
	_peer_container_type::const_iterator i = _participants.begin();
	for (; i != _participants.end(); i++) {
		l->push_back(i->second);	
	}
	return _participants.size();
}

const node *
group::self() const {
	return _self;
}

reactor_type *
group::reactor() {
	return _reactor;
}

void
group::reactor(reactor_type *r) {
	if (r == NULL) {
		throw call_error("netcomgrp::group::reactor NULL pointer not allowed");
	}
	_reactor = r;
}

int
group::process(time_value_type *max_wait) {
	// _state->process(this);	
	return _reactor->handle_events(max_wait);
}

int
group::process(time_value_type &max_wait) {
	// _state->process(this);	
	return _reactor->handle_events(max_wait);
}

//void
//group::_peers_complete(bool v) {
//	if (!_pcomplete && v) {
//		_notify(state::peers_complete);
//	}
//	_pcomplete = v;
//}
int
group::_peer_user_data_read(const void *buf, size_t nread, 
		                    const node *n)
{
	return observer_notifier()->data_received(buf, nread, n);		
}

/*
node *
group::_peer_find(const addr_inet_type &a) {
	ACE_DEBUG((LM_DEBUG, "group::server: finding peer: " \
	                     "%s:%d\n", 
	                     a.get_host_addr(), 
	                     a.get_port_number()));

	_peer_container_type::iterator i;
	_peer_container_type *c;
	
	return _peer_from_container(&c, &i, a);
}
*/
node *
group::_peer_confirm(const addr_inet_type &a, bool is_self) {
	ACE_DEBUG((LM_DEBUG, "group::centralized::client: confirming peer: " \
	                     "%s:%d\n", 
	                     a.get_host_addr(), 
	                     a.get_port_number()));

	if (a.get_ip_address() == INADDR_ANY ||
	    a.get_ip_address() == INADDR_NONE)
	{
		ACE_DEBUG((LM_DEBUG, "group::centralized::client: invalid peer address received\n"));
		return NULL;
	}
	
	_peer_container_type::iterator i;
	_peer_container_type *c;
	node *n = NULL;
	
	if ((n = _peer_from_container(&c, &i, a))) {
		if (!_self && is_self) _self = n;
		
		if (c == &_participants) {
			ACE_DEBUG((LM_DEBUG, "group::centralized::client: " \
			          "address was already known, returning it\n"));
		    return n;
		}
		throw unexpected_error("group::centralized::client::peer_confirm: " \
		                       "not possible");
	}

	n = new node(a);

	ACE_DEBUG((LM_DEBUG, "group::server: " \
	          "address inserted to participants container\n"));
	
	_participants.insert(std::make_pair(a, n));

	if (is_self) {
		_self = n;
		ACE_DEBUG((LM_DEBUG, "group::centralized::client: assuming received address is self\n"));
		observer_notifier()->state_changed(group::joined);		
	}
	// The node was added to confirmed nodes, so do notify.
	observer_notifier()->node_added(n);	
	return n;
}

void
group::_peer_remove(const addr_inet_type &a) {
	ACE_DEBUG((LM_DEBUG, "group::centralized::client: peer remove for: " \
	                     "%s:%d\n", 
	                     a.get_host_addr(), 
	                     a.get_port_number()));
	
	_peer_container_type::iterator i;
	_peer_container_type *c;
	node *n = NULL;
	
	if ((n = _peer_from_container(&c, &i, a))) {
		ACE_DEBUG((LM_DEBUG, "group::centralized::client: removing node\n"));
		c->erase(i);
		
		// Only notify if the node was confirmed (ie. its addition was
		// or would have been notified)
		if (c == &_participants)
			observer_notifier()->node_removed(n);
		
		delete n;
		if (_self == n) {
			ACE_DEBUG((LM_DEBUG, "group::centralized::client::_peer_remove "
			          "was self\n"));
			_self = NULL;
		}
		return;
	}

	ACE_DEBUG((LM_WARNING, "group::centralized::client: the node did not exist\n"));
}

void
group::_peer_send_timeout(const addr_inet_type &a) {
	ACE_DEBUG((LM_DEBUG, "group::server: peer send timeout for : " \
	                     "%s:%d\n", 
	                     a.get_host_addr(), 
	                     a.get_port_number()));
	
	observer_notifier()->error(err_timeout_connect, 
	                           "Connection to server timeouted");
	_peers_clear();	
	observer_notifier()->state_changed(group::not_joined);
	
	// throw exception(exception_code::connection, 
	//                "Connection to server timeouted");
}

/*
void
group::_peer_left(const addr_inet_type &a) {
	ACE_DEBUG((LM_DEBUG, "group::server: peer leave for: " \
	                     "%s:%d\n", 
	                     a.get_host_addr(), 
	                     a.get_port_number()));
	
	_peer_container_type::iterator i;
	_peer_container_type *c;
	node *n = NULL;
	
	if ((n = _peer_from_container(&c, &i, a))) {
		ACE_DEBUG((LM_DEBUG, "group::server removing timeouted node\n"));
		c->erase(i);
		
		// So shall it be, that we will put the timeouted node to the
		// timeout list so that the same node won't be checked again
		// unless a change in its status happens
		// _ptimeouted.insert(make_pair(a, n));
		
		// Only notify if the node was confirmed (ie. its addition was
		// or would have been notified)
		if (c == &_participants)
			observer_notifier()->node_removed(n);
			
		// _sub_comms->send_left(n);
		delete n;
	}
}
*/

void
group::_peers_clear() {
	for (_peer_container_type::iterator i = _participants.begin();
	     i != _participants.end(); i++)
	{
		node *n = i->second;
		observer_notifier()->node_removed(n);
		delete n;
		if (n == _self) _self = NULL;
	}
	_participants.clear();
}


node *
group::_peer_from_container(_peer_container_type **peer_cont,
                            _peer_container_type::iterator *i,
                            const addr_inet_type &a)
{
	if ((*i = _participants.find(a)) != _participants.end()) {
		ACE_DEBUG((LM_DEBUG, "group::server: " \
		          "address was in peer participants container\n"));
		*peer_cont = &_participants;
		return (*i)->second;
	}

	return NULL;
}

reudp::dgram_multi &
group::dgram_adapter() {
    return _sub_comms->dgram_adapter();
}  

} // ns client
} // ns centralized
} // ns netcomgrp
