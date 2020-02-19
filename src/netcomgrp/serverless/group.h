#ifndef _NETCOMGRP_SERVERLESS_GROUP_H_
#define _NETCOMGRP_SERVERLESS_GROUP_H_

#include <string>
#include <list>
#include <map>

#include <dht/client.h>

#include "../group.h"
#include "node.h"

namespace netcomgrp {
namespace serverless {
	using namespace std;
	
	class group : public netcomgrp::group
	{		
		dht::client            *_dht_client;
		
		// TODO make an adapter interface so that the application
		// can control how sends/receives are done
		// class dgram_adapter    *_dgram_adapter;
		class sub_system_chain *_system_manager;
		class sub_comms    *_system_comms;
				
		string          _grp_id;
		addr_inet_type  _addr_ext;
		addr_inet_type  _addr_ann;
		reactor_type   *_reactor;
		node           *_self;
		int             _active_mode;
		int             _opts;
		
		typedef map<addr_inet_type, node *> _peer_container_type;
		// Node must be in only one container at a time
		_peer_container_type _ppossible;  // nodes that need to be checked
		_peer_container_type _pchecking;  // nodes that are being checked
		_peer_container_type _pconfirmed; // nodes that are confirmed
		_peer_container_type _ptimeouted; // nodes that have timeouted
		// bool                 _pself;
		// bool                 _pcomplete;
		
		friend class sub_system;
		
		inline const string &_group_id() const;		
		void  _external_addr_detected(const addr_inet_type &a);
		const  addr_inet_type &_external_addr() { return _addr_ext; }

		void  _peers_clear();
		int   _peer_possible(const addr_inet_type &a, long activity);
		void  _peer_left(const addr_inet_type &a);
		node *_peer_possible_to_check();
		node *_peer_confirm(const addr_inet_type &a, long activity);
		node *_peer_self_ensure();
		inline node *_peer_confirmed_find(const addr_inet_type &a) {
			_peer_container_type::iterator i = _pconfirmed.find(a);
			return (i != _pconfirmed.end() ? i->second : NULL);
		}
		
		void  _peer_send_timeout(const addr_inet_type &a);
		inline _peer_container_type::iterator
		       _peer_confirmed_begin() { return _pconfirmed.begin(); }
		inline _peer_container_type::iterator
		       _peer_confirmed_end() { return _pconfirmed.end(); }

		int _peer_user_data_read(const void *buf, size_t nread, 
		                         const node *n);
		       
		// void _peer_confirmed(const node &a);
		inline size_t _peer_confirmed_size();
		inline bool   _peer_self_detected();
//		inline bool   _peers_complete() const;  
//		void          _peers_complete(bool v);
		inline size_t _peers_size() const;
		
		node *_peer_from_container(_peer_container_type **peer_cont,
                                   _peer_container_type::iterator *i,
                                   const addr_inet_type &a,
                                   int   activity,
                                   bool *is_new);
                                   // bool *is_self = NULL);
		void _notify(int notify_id);
		
	public:
		// extra_state
		enum {
			peers_find_start,
			peers_find_stop,
			announce_start,
			announce_stop,
		};
		
		enum active_mode_type {
			active_normal = 1,
			active_lazy,
		};
		
		// opts
		const static int opt_deannounce_with_leave = 0x1;
		
		group();
		virtual ~group();
		
		// Setting used DHT node is mandatory
		inline void       dht_client(dht::client *use_dht);
		inline dht::client *dht_client();
				
		/**
		 * @brief sets the address and port that is announced & used
		 * @param a  the address (IP & port)
		 * 
		 * By default the used IP is tried to be detected using
		 * DHT's external address. But it can be overridden with
		 * this function. If the IP is other than INADDR_ANY or INADDR_NONE
		 * it is used, otherwise the one detected with DHT is used.
		 * 
		 * If the port is != 0 it is used, otherwise the default
		 * port is used
		 */
		inline void                  addr_announce(const addr_inet_type &a);
		inline const addr_inet_type &addr_announce() const;
		
		inline int active_mode() const { return _active_mode; }
		int active_mode(active_mode_type am);
		inline int opts_enable(int eopts)  { return _opts |= eopts; }
		inline int opts_disable(int dopts) { return _opts &= ~dopts; }
		inline int opts() { return _opts; }
		
		/* start of implemented netcomgrp::group interface */
		// virtual void handler(event_handler *ev_handler);
		virtual void join(const char *group_id);
		virtual void leave();
		
		virtual void send(const char *msg, size_t msglen, const netcomgrp::node *recp);
		virtual void send(const char *msg, size_t msglen);

		virtual size_t nodes_size() const;
		virtual size_t nodes(std::list<const netcomgrp::node *> *l);
		virtual const node *self() const;		
		
		virtual int process(time_value_type &max_wait);
		virtual int process(time_value_type *max_wait);

		virtual reactor_type *reactor();
		virtual void          reactor(reactor_type *);

		/* end of implemented netcomgrp::group interface */	
        
        /* TODO this is _DEFINITELY_ a temporarily solution that
         * must be replaced with a proper abstraction
         */
         virtual reudp::dgram_multi &dgram_adapter();
	};
	
	inline void
	group::dht_client(dht::client *use_dht) { _dht_client = use_dht; }
	inline dht::client *
	group::dht_client() { return _dht_client; }

	inline void group::addr_announce(const addr_inet_type &a) {
		if (a.get_port_number() != 0) {
			_addr_ann.set_port_number(a.get_port_number());
			ACE_DEBUG((LM_DEBUG, "group::addr_announce set address to %s:%d\n",
			          _addr_ann.get_host_addr(), _addr_ann.get_port_number()));
		}
		if (a.get_ip_address() != INADDR_ANY &&
		    a.get_ip_address() != INADDR_NONE)
		{
			_addr_ann.set(_addr_ann.get_port_number(), a.get_ip_address());
			ACE_DEBUG((LM_DEBUG, "group::addr_announce set address to %s:%d\n",
			          _addr_ann.get_host_addr(), _addr_ann.get_port_number()));
		}
	}
	inline const addr_inet_type &group::addr_announce() const {
		return _addr_ann;
	}

	inline const string &
	group::_group_id() const { return _grp_id; }	

	inline size_t
	group::_peer_confirmed_size() {
		return _pconfirmed.size();
	}
	inline bool
	group::_peer_self_detected() {
		// TODO _self node != 0 doesn't guarantee anymore that self has been
		// found from DHT.
		return _self ? true : false; // _pself;
	}

//	inline bool 
//	group::_peers_complete() const {
//		return _pcomplete;
//	}
	inline size_t 
	group::_peers_size() const {
		return _ppossible.size()  +
		       _pchecking.size()  +
		       _pconfirmed.size() + 
		       _ptimeouted.size();
	}

	// inline void
	// group::group_id(const string &grp) { _group_id = grp; }
	
} // ns serverless	
} // ns netcomgrp

#endif //_NETCOMGRP_GROUP_H_
