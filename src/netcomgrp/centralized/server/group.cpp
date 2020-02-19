#include <utility>

#include "../../exception.h"
#include "group.h"

#include "sub_comms.h"

namespace netcomgrp {
namespace centralized {
namespace server {

group::group() {
	ACE_DEBUG((LM_DEBUG, "netcomgrp::server::group constructor!\n"));
	
	// Unless otherwise instructed, use ACE's system wide reactor
	_reactor     = reactor_type::instance();	
	_sub_comms   = new sub_comms(this);
}

group::~group() {
	delete _sub_comms;
	_peers_clear();
}

void 
group::join(const char *grp_id) {
	observer_notifier()->state_changed(group::joining);
	_sub_comms->join(grp_id);
	observer_notifier()->state_changed(group::joined);
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
		ACE_DEBUG((LM_WARNING, "netcomgrp::server::group::send " \
		          "received a node ptr that was not from server group"));
		return;
	}
	_sub_comms->send(msg, msglen, n);
}

void
group::send(const char *msg, size_t msglen) {
	_sub_comms->send(msg, msglen);
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
	// Return NULL... since a server is not an actual node on the group
	// anyway.
	return NULL;
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

node *
group::_peer_confirm(const addr_inet_type &a, bool *is_new) {
	ACE_DEBUG((LM_DEBUG, "group::server: confirming peer: " \
	                     "%s:%d\n", 
	                     a.get_host_addr(), 
	                     a.get_port_number()));

	_peer_container_type::iterator i;
	_peer_container_type *c;
	node *n = NULL;
	
	if ((n = _peer_from_container(&c, &i, a))) {
		if (c == &_participants) {
			ACE_DEBUG((LM_DEBUG, "group::server: " \
			          "address was already known, returning it\n"));
		    return n;
		}
		throw unexpected_error("peer_confirm: not possible");
	}

	n = new node(a);
	if (is_new) *is_new = true;
	
	// Let other participants know of the new node
	//_sub_comms->send_joined(n);

	ACE_DEBUG((LM_DEBUG, "group::server: " \
	          "address inserted to participants container\n"));	
	_participants.insert(std::make_pair(a, n));

	// The node was added to confirmed nodes, so do notify.
	observer_notifier()->node_added(n);	
	return n;
}

bool
group::_peer_send_timeout(const addr_inet_type &a) {
	ACE_DEBUG((LM_DEBUG, "group::server: peer send timeout for: " \
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
			
		_sub_comms->send_timeout(n);
		delete n;
		return true;
	}
	return false;
}

bool
group::_peer_left(const addr_inet_type &a) {
	ACE_DEBUG((LM_DEBUG, "group::server: peer leave for: " \
	                     "%s:%d\n", 
	                     a.get_host_addr(), 
	                     a.get_port_number()));
	
	_peer_container_type::iterator i;
	_peer_container_type *c;
	node *n = NULL;
	
	if ((n = _peer_from_container(&c, &i, a))) {
		ACE_DEBUG((LM_DEBUG, "group::server removing leaving node\n"));
		c->erase(i);
		
		// So shall it be, that we will put the timeouted node to the
		// timeout list so that the same node won't be checked again
		// unless a change in its status happens
		// _ptimeouted.insert(make_pair(a, n));
		
		// Only notify if the node was confirmed (ie. its addition was
		// or would have been notified)
		if (c == &_participants)
			observer_notifier()->node_removed(n);
			
		ACE_DEBUG((LM_DEBUG, "group::server deleting node %d\n", n));
		delete n;
		ACE_DEBUG((LM_DEBUG, "group::server node deleted, resuming action\n", n));
		return true;
	}
	return false;
}

void
group::_peers_clear() {
	for (_peer_container_type::iterator i = _participants.begin();
	     i != _participants.end(); i++)
	{
		node *n = i->second;
		observer_notifier()->node_removed(n);
		delete n;
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

} // ns server
} // ns centralized
} // ns netcomgrp
