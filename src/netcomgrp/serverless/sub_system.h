#ifndef _NETCOMGRP_CENTRALIZED_SUB_SYSTEM_H_
#define _NETCOMGRP_CENTRALIZED_SUB_SYSTEM_H_

#include "group.h"
#include "state.h"

namespace netcomgrp {
namespace serverless {
	using namespace std;
	
	// Interface for sub system modules
	class sub_system {
		group      *_owner;
		const char *_id;
	protected:
		inline group *owner() { return _owner; }

		inline const addr_inet_type &external_addr() { 
			return _owner->_external_addr(); 
		}
		inline void external_addr_detected(const addr_inet_type &addr) { 
			return _owner->_external_addr_detected(addr); 
		}

		inline int  peer_possible(const addr_inet_type &addr, long activity) { 
			return _owner->_peer_possible(addr, activity); 
		}
		inline void peer_left(const addr_inet_type &addr) { 
			return _owner->_peer_left(addr); 
		}
		inline node *peer_confirmed_find(const addr_inet_type &addr) {
			return _owner->_peer_confirmed_find(addr);
		}
		inline int  peer_confirmed_size() {
			return _owner->_peer_confirmed_size();
		}
		inline int  peer_self_detected() {
			return _owner->_peer_self_detected();
		}
		inline node *peer_possible_to_check() {
			return _owner->_peer_possible_to_check();
		}
		inline node *peer_confirm(const addr_inet_type &a, long activity) {
			return _owner->_peer_confirm(a, activity);
		}
		inline node *peer_self_ensure() {
			return _owner->_peer_self_ensure();
		}
		inline void peer_send_timeout(const addr_inet_type &a) {
			return _owner->_peer_send_timeout(a);
		}
		inline size_t peer_confirmed_size() const { 
			return _owner->_peer_confirmed_size(); 
		}
		inline group::_peer_container_type::iterator peer_confirmed_begin() { 
			return _owner->_peer_confirmed_begin(); 
		}
		inline group::_peer_container_type::iterator peer_confirmed_end() { 
			return _owner->_peer_confirmed_end(); 
		}

//		inline bool peers_complete() const {
//			return _owner->_peers_complete();
//		}
//		inline void peers_complete(bool v) {
//			return _owner->_peers_complete(v);
//		}
		inline size_t peers_size() const {
			return _owner->_peers_size();
		}
		
		inline const string &group_id() {
			return _owner->_group_id();
		}
		inline void notify(int notify_id) { 
			return _owner->_notify(notify_id); 
		}
		inline reactor_type *reactor() {
			return _owner->reactor();
		}
		inline const addr_inet_type &addr_announce() const {
			return _owner->addr_announce();
		}
		
		inline int peer_user_data_read(const void *buf, size_t nread, 
		                               const node *n) 
		{
			return _owner->_peer_user_data_read(buf, nread, n);
		}
		
		inline netcomgrp::event_observer_notifier *observer_notifier() {
			return _owner->observer_notifier();
		}
	public:
		sub_system(group *g, const char *idstr = "<undefined>");
		virtual ~sub_system();
		
		virtual void join(const char *grp_id) {}
		virtual void leave() {}
		
		virtual void notice_receive(int notice_id) {}
		
		inline const char *id()                  { return _id; }
		inline void        id(const char *idstr) { _id = idstr; }
		
		// Internal
		virtual void peer_confirmed_new(node *n) {};
	};
	
} // ns serverless	
} // ns netcomgrp

#endif //_NETCOMGRP_CENTRALIZED_SUB_SYSTEM_H_
