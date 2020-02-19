#include "../exception.h"
#include "group.h"
#include "state.h"
#include "sub_system_chain.h"
// #include "sub_state_manager.h"
#include "sub_dht_connection.h"
#include "sub_dht_announce.h"
#include "sub_dht_peer_find.h"
// #include "sub_dgram_signal.h"
#include "sub_comms.h"
#include "sub_state_notifier.h"
#include "internal_defines.h"
#include "util.h"
#include "event_observer_notifier.h"

namespace netcomgrp {
namespace serverless {

group::group() {
	ACE_DEBUG((LM_DEBUG, "netcomgrp::serverless::group constructor\n"));
	
	_dht_client  = NULL;
	
	// TODO what port to choose as default?
	_addr_ann.set_port_number(8859);
	_self  = NULL;
	_active_mode = active_normal;
	_opts        = opt_deannounce_with_leave;

	// Install own observer notifier
	delete observer_notifier();
	observer_notifier(new netcomgrp::serverless::event_observer_notifier);
	
	_system_manager    = new sub_system_chain(this);
	_system_comms      = new sub_comms(this);
	// sub_state_manager *_state_manager  = new sub_state_manager(this);
	
	// _system_manager->add(_state_manager);
	_system_manager->add(new sub_dht_connection(this));
	_system_manager->add(new sub_dht_announce(this));
	_system_manager->add(new sub_dht_peer_find(this));
	// _system_manager->add(new sub_dgram_signal(this));
	_system_manager->add(_system_comms);
	_system_manager->add(new sub_state_notifier(this));
	
	// Unless otherwise instructed, use ACE's system wide reactor
	_reactor   = reactor_type::instance();	
}

group::~group() {
	delete _system_manager;
}

void 
group::join(const char *grp_id) {
	_grp_id = grp_id;
	ACE_DEBUG((LM_DEBUG, "system manager: %d\n", _system_manager));
	_system_manager->join(grp_id);
	// _state->join(this, grp_id);
}

void 
group::leave() {
	_system_manager->leave();
	_grp_id.erase();

	_peers_clear();

	// _pcomplete = false;
	
	// _state->leave(this, grp_id);	
}

int
group::active_mode(active_mode_type am) {
	ACE_DEBUG((LM_DEBUG, "netcomgrp::serverless::group::active_mode change\n"));
	if (am != _active_mode) {
		_active_mode = am;
		_notify(state::active_mode_changed);
	}
	return _active_mode;
}

void
group::_peers_clear() {	
	// Collect all addresses, then call for each peer_left
	std::vector<addr_inet_type> addrs; // (_peers_size());
	std::insert_iterator< vector<addr_inet_type> > ins(addrs, addrs.begin());
	ins = std::transform(_ppossible.begin(), _ppossible.end(), ins,
	               util::addr_node_pair_to_addr);
	ins = std::transform(_pchecking.begin(), _pchecking.end(), ins,
	               util::addr_node_pair_to_addr);
	ins = std::transform(_pconfirmed.begin(), _pconfirmed.end(), ins,
	               util::addr_node_pair_to_addr);
	ins = std::transform(_ptimeouted.begin(), _ptimeouted.end(), ins,
	               util::addr_node_pair_to_addr);
	               
	std::vector<addr_inet_type>::iterator i;
	for (i = addrs.begin(); i != addrs.end(); i++)
		_peer_left(*i);	
}

void
group::send(const char *msg, size_t msglen, const netcomgrp::node *recp) {
	const node *n = dynamic_cast<const node *>(recp);
	if (!n) {
		ACE_DEBUG((LM_WARNING, "netcomgrp::serverless::group::send " \
		          "received a node ptr that was not from serverless group"));
		return;
	}
	// Don't allow sending unless joined or joining.
	// Sending during joining is allowed since events about nodes 
	// can start coming in before full joined state has been reached.
	if (in_state() == group::joined ||
	    in_state() == group::joining) 
	{
		_system_comms->send(msg, msglen, n);
	} else {
		ACE_DEBUG((LM_WARNING, "netcomgrp::serverless::group::send " \
		          "tried to send but not in joined state, ignored, "
		          "current state is '%s'\n", in_state_str()));
	}
}

void
group::send(const char *msg, size_t msglen) {
	// Don't allow sending unless joined or joining.
	// Sending during joining is allowed since events about nodes 
	// can start coming in before full joined state has been reached.
	if (in_state() == group::joined ||
	    in_state() == group::joining) 
	{
		_system_comms->send(msg, msglen);	
	} else {
		ACE_DEBUG((LM_WARNING, "netcomgrp::serverless::group::send " \
		          "tried to send but not in joined state, ignored, "
		          "current state is '%s'\n", in_state_str()));
	}
}

size_t
group::nodes_size() const {
	return _pconfirmed.size();
}

size_t
group::nodes(std::list<const netcomgrp::node *> *l) {
	_peer_container_type::const_iterator i = _pconfirmed.begin();
	for (; i != _pconfirmed.end(); i++) {
		l->push_back(i->second);	
	}
	return _pconfirmed.size();
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
		throw call_error(
		                "netcomgrp::group::reactor NULL pointer not allowed");
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

void
group::_notify(int noticy_id) {
	_system_manager->notice_send(noticy_id);	
}

//void
//group::_peers_complete(bool v) {
//	if (!_pcomplete && v) {
//		_notify(state::peers_complete);
//	}
//	_pcomplete = v;
//}

void
group::_external_addr_detected(const addr_inet_type &a) {
	// For now, TODO set the port to correct, check the address obtained,
	// or set with options
	_addr_ext = a;
	if (_addr_ext == addr_inet_type())
		throw internal_error("group::serverless: " \
		                "failed detecting external address\n");
		                
	ACE_DEBUG((LM_DEBUG, "group::serverless: external address detected: " \
	                     "%s:%d\n", 
	                     _addr_ext.get_host_addr(), 
	                     _addr_ext.get_port_number()));
	                     
	if (_addr_ann.get_ip_address() == INADDR_NONE ||
	    _addr_ann.get_ip_address() == INADDR_ANY)
	{
		ACE_DEBUG((LM_DEBUG, "group::serverless: setting announce address\n"));
		_addr_ann.set(_addr_ann.get_port_number(), a.get_ip_address());
	}
}

int
group::_peer_user_data_read(const void *buf, size_t nread, 
                            const node *n)
{
	return observer_notifier()->data_received(buf, nread, n);	
}

int
group::_peer_possible(const addr_inet_type &a, long activity) {
	// TODO check if the peer already exists in potential list,
	// or in known peers
	ACE_DEBUG((LM_DEBUG, "group::serverless: potential peer received: " \
	                     "%s:%d last_activity: %d\n", 
	                     a.get_host_addr(), 
	                     a.get_port_number(),
	                     activity));

	_peer_container_type::iterator i;
	_peer_container_type *c = NULL;
	node *n     = NULL;
	bool is_new = false;
	n = _peer_from_container(&c, &i, a, activity, &is_new);

	if (c == &_ptimeouted) {
		ACE_DEBUG((LM_DEBUG, "group::serverless peer in list of " \
		          "timeouted peers. Last activity %d, received %d\n",
		          i->second->last_activity().sec(),
		          activity));
		if (i->second->last_activity().sec() < activity) {
			ACE_DEBUG((LM_DEBUG, "group::serverless activity is newer " \
			          " so trying this node again\n"));
			c->erase(i);
			delete n;
			if (n == _self) _self = NULL;
			// Do again peer from container to create the new node,
			// this time it can't be in timeouted container
			n = _peer_from_container(&c, &i, a, activity, &is_new);
		} else {
			ACE_DEBUG((LM_DEBUG, "group::serverless peer has already " \
			          "been checked and is apparently not active\n"));
			return internal::peer_timeouted;
		}
	} 

	if (n == _self) {
		// Remove from the container it was in and confirm immediately
		// - no need to confirm self...?
		ACE_DEBUG((LM_DEBUG, "group::serverless: address is self, " \
		           "confirming it\n"));
		_peer_confirm(a, activity);
		return internal::peer_self;
	}
	
	if (!is_new) {
		ACE_DEBUG((LM_DEBUG, "group::serverless: address is already known\n"));		
		return internal::peer_known;
	}

	_notify(state::peer_new_possible);	
	
	return internal::peer_new;
}

void
group::_peer_left(const addr_inet_type &a) {
	ACE_DEBUG((LM_DEBUG, "group::serverless: peer left received from: " \
	                     "%s:%d\n", 
	                     a.get_host_addr(), 
	                     a.get_port_number()));
	
	_peer_container_type::iterator i;
	_peer_container_type *c = NULL;
	bool is_new = false;
	node *n = NULL;
	
	n = _peer_from_container(&c, &i, a, 0, &is_new);
	ACE_DEBUG((LM_DEBUG, "group::serverless removing node that left\n"));
	c->erase(i);
				
	// Only notify if the node was confirmed (ie. its addition was
	// or would have been notified)
	if (c == &_pconfirmed)
		observer_notifier()->node_removed(n);
		
	delete n;
	if (_self == n) _self = NULL;
}

node *
group::_peer_possible_to_check() {
	if (_ppossible.empty()) return NULL;

	_peer_container_type::iterator i = _ppossible.begin();	
	node *n = i->second;
	_pchecking[i->first] = i->second;
	_ppossible.erase(i);
	return n;
}

node *
group::_peer_confirm(const addr_inet_type &a, long activity) {
	ACE_DEBUG((LM_DEBUG, "group::serverless: confirming peer: " \
	                     "%s:%d activity %d\n", 
	                     a.get_host_addr(), 
	                     a.get_port_number(),
	                     activity));

	// If a new node is to be confirmed, first must ensure that
	// self node has been created and reported before any other.
	_peer_self_ensure();
	
	_peer_container_type::iterator i;
	_peer_container_type *c;
	node *n = NULL;
	// bool is_self = false;
	bool is_new  = false;
	
	n = _peer_from_container(&c, &i, a, activity, &is_new);
	if (c == &_pconfirmed) {
		ACE_DEBUG((LM_DEBUG, "group::serverless: " \
		          "address was already in peer confirmed container, returning it\n"));
		// Update activity
		if (activity > 0) {
			n->last_activity(time_value_type(activity));
		}
	    return n;
	}
	ACE_DEBUG((LM_DEBUG, "group::serverless removing confirmed node " \
	          "from its previous container\n"));
	c->erase(i);

	ACE_DEBUG((LM_DEBUG, "group::serverless: " \
	          "address inserted to confirmed container\n"));
	
	_pconfirmed.insert(make_pair(a, n));

	// Since self is always in pconfirmed when it's detected,
	// this won't be called when self node found.
	// Detection of NATted status depends on this behaviour too
	if (is_new || c != &_pconfirmed) {
		_system_manager->peer_confirmed_new(n);
	}


	// The node was added to confirmed nodes, so do notify.
	observer_notifier()->node_added(n);	
	return n;
}

void
group::_peer_send_timeout(const addr_inet_type &a) {
	ACE_DEBUG((LM_DEBUG, "group::serverless: peer send timeout for: " \
	                     "%s:%d\n", 
	                     a.get_host_addr(), 
	                     a.get_port_number()));
	
	_peer_container_type::iterator i;
	_peer_container_type *c = NULL;
	node *n = NULL;
	
	n = _peer_from_container(&c, &i, a, 0, NULL);
	ACE_DEBUG((LM_DEBUG, "group::serverless removing timeouted node\n"));
	c->erase(i);
		
	// So shall it be, that we will put the timeouted node to the
	// timeout list so that the same node won't be checked again
	// unless a change in its status happens
	_ptimeouted.insert(make_pair(a, n));
		
	// Only notify if the node was confirmed (ie. its addition was
	// or would have been notified)
	if (c == &_pconfirmed) {
		observer_notifier()->node_removed(n);		
	}
}

node *
group::_peer_from_container(_peer_container_type **peer_cont,
                            _peer_container_type::iterator *i,
                            const addr_inet_type &ao,
                            int activity,
                            bool *is_new
                            // bool *is_self)
)
{
#ifndef NETCOMGRP_SERVERLESS_SELF_ADDR_AS_IS
	addr_inet_type a;
	/* It seems some routers do not allow sending to ones
	 * public address packets... so instead of using
	 * such addresses, use localhost directly
	 */
	if (ao == _addr_ann) {
		ACE_DEBUG((LM_DEBUG, "netcomgrp::serverless: %s:%d is our announce address\n",
		           ao.get_host_addr(), ao.get_port_number()));
		a = addr_inet_type(_addr_ann.get_port_number(), INADDR_LOOPBACK);
		ACE_DEBUG((LM_DEBUG, "netcomgrp::serverless: now using loopback %s:%d",
		          a.get_host_addr(), a.get_port_number()));
	} else {
		a = ao;
	}
#else
	const addr_inet_type &a = ao;
#endif
	if ((*i = _pconfirmed.find(a)) != _pconfirmed.end()) {
		ACE_DEBUG((LM_DEBUG, "group::serverless: " \
		          "address was in peer confirmed container\n"));
		*peer_cont = &_pconfirmed;
		return (*i)->second;
	}
	if ((*i = _ppossible.find(a)) != _ppossible.end()) {
		ACE_DEBUG((LM_DEBUG, "group::serverless: " \
		          "address was in peer possible container\n"));
		*peer_cont = &_ppossible;
		return (*i)->second;
	}	
	if ((*i = _pchecking.find(a)) != _pchecking.end()) {
		ACE_DEBUG((LM_DEBUG, "group::serverless: " \
		          "address was in peer checking container\n"));
		*peer_cont = &_pchecking;
		return (*i)->second;
	}
	if ((*i = _ptimeouted.find(a)) != _ptimeouted.end()) {
		ACE_DEBUG((LM_DEBUG, "group::serverless: " \
		          "address was in peer timeouted container\n"));
		*peer_cont = &_ptimeouted;
		return (*i)->second;
	}

	if (// a == _addr_ext ||
	    a == _addr_ann) 
	{
		ACE_DEBUG((LM_DEBUG, "group::serverless: address is self\n"));
		_peer_self_ensure();
		*i = _pconfirmed.find(a);
		if (*i == _pconfirmed.end()) {
			ACE_ERROR((LM_ERROR, "group::serverless::_peer_from_container: "
			          "did self_ensure yet did not come to pconfirmed\n"));
			throw unexpected_error("group::serverless::_peer_from_container fatal");
		}
		*peer_cont = &_pconfirmed;
		return _self;
	}

	ACE_DEBUG((LM_DEBUG, "group::serverless::_peer_from_container "
	          "not in any container, creating new\n"));
	// If not found from any of the containers, create new node, 
	// and insert it into the ppossible container
	node *n = new node(a, time_value_type(activity));
	if (is_new) *is_new = true;
	*i         =  _ppossible.insert(make_pair(a, n)).first;
	*peer_cont = &_ppossible;

#if 0	
	if (// a == _addr_ext ||
	    a == _addr_ann) 
	{
		ACE_DEBUG((LM_DEBUG, "group::serverless: address is self\n"));
		_self = n;
	}
#endif

	return n;
}

node *
group::_peer_self_ensure() {
	if (!_self) {
		ACE_DEBUG((LM_DEBUG, "group::serverless creating self node: %s:%d\n",
		          _addr_ann.get_host_addr(), _addr_ann.get_port_number()));

#ifndef NETCOMGRP_SERVERLESS_SELF_ADDR_AS_IS	
		node *n = new node(addr_inet_type(_addr_ann.get_port_number(), INADDR_LOOPBACK), 
		                   time_value_type(0));
#else
		node *n = new node(_addr_ann, time_value_type(0));
#endif
		_pconfirmed[_addr_ann] = n;
		_self = n;
		observer_notifier()->node_added(n);
		this->_notify(state::self_added);		
	}
	return _self;	
}

reudp::dgram_multi &
group::dgram_adapter() {
    return _system_comms->dgram_adapter();
}  

} // ns serverless	
} // ns netcomgrp
