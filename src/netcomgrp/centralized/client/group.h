#ifndef _NETCOMGRP_CENTRALIZED_SERVER_GROUP_H_
#define _NETCOMGRP_CENTRALIZED_SERVER_GROUP_H_

#include <map>
#include <algorithm>
#include <functional>

#include <reudp/reudp_multi.h>

#include "../../group.h"
#include "../node.h"
#include "../util.h"

namespace netcomgrp {
namespace centralized {
namespace client {
	/**
	 * @brief Client for network communication group that requires a
	 *        central server (see netcomgrp::centralized::server)
	 * 
	 * Uses one UDP socket for communication and borrows heavily from
	 * the serverless implementation's internals. Can not be considered 
	 * robust, used for testing purposes.
	 */	
	class group : public netcomgrp::group {
		typedef std::map<addr_inet_type, node *> _peer_container_type;
		_peer_container_type _participants;
		reactor_type        *_reactor;
		
		class sub_comms     *_sub_comms;
		friend class sub_comms;
		node *_self;
		
		// node *_peer_find(const addr_inet_type &a);
		void  _peer_send_timeout(const addr_inet_type &a);
		// void  _peer_left(const addr_inet_type &a);
		inline size_t _peer_participants_size() { return _participants.size(); }
		inline _peer_container_type::iterator
		       _peer_participants_begin() { return _participants.begin(); }
		inline _peer_container_type::iterator
		       _peer_participants_end() { return _participants.end(); }
		void   _peers_clear();
		node *_peer_from_container(_peer_container_type **peer_cont,
                                   _peer_container_type::iterator *i,
                                    const addr_inet_type &a);

		node *_peer_confirm(const addr_inet_type &a, bool is_self);
		void  _peer_remove(const addr_inet_type &a);
		
		template <typename _Iter, typename _Trans>
		void _peer_participants(_Iter start, _Iter end,
		                        _Trans transform_func)
		{
			std::vector<addr_inet_type> peers_sorted;
			std::vector<addr_inet_type> prtcp_sorted;
			std::list<addr_inet_type>   peers_to_add;
			std::list<addr_inet_type>   peers_remove;
			std::list<addr_inet_type>::iterator i;

			std::insert_iterator<std::list<addr_inet_type> >
			     peers_to_add_ins(peers_to_add, peers_to_add.begin()),
			     peers_remove_ins(peers_remove, peers_remove.begin());

			std::insert_iterator<std::vector<addr_inet_type> >
			     peers_sorted_ins(peers_sorted, peers_sorted.begin()),
			     prtcp_sorted_ins(prtcp_sorted, prtcp_sorted.begin());;

			std::transform(start, end, peers_sorted_ins, 
			               transform_func);
			std::transform(_participants.begin(), _participants.end(),
			               prtcp_sorted_ins, util::addr_node_pair_to_addr());

			std::sort(peers_sorted.begin(), peers_sorted.end());
			std::sort(prtcp_sorted.begin(), prtcp_sorted.end());
			
			std::set_difference(peers_sorted.begin(), 
			                    peers_sorted.end(), 
			                    prtcp_sorted.begin(),
			                    prtcp_sorted.end(),
			                    peers_to_add_ins);
			std::set_difference(prtcp_sorted.begin(),
			                    prtcp_sorted.end(),
			                    peers_sorted.begin(), 
			                    peers_sorted.end(), 
			                    peers_remove_ins); 

			for (i = peers_to_add.begin(); i != peers_to_add.end(); i++) {
				_peer_confirm(*i, false);
			}
			for (i = peers_remove.begin(); i != peers_remove.end(); i++) {
				_peer_remove(*i);
			}			
		}
		
		int   _peer_user_data_read(const void *buf, size_t nread, 
		                           const node *n);				
	public:
	
		group();
		virtual ~group();
		
		// group_id should specify a IP and port to bind in the form of
		// an IP/DNS address, server:port or port.
		virtual void join(const char *group_id);
		virtual void leave();
		
		virtual void send(const char *msg, size_t msglen, 
		                  const netcomgrp::node *recp);
		virtual void send(const char *msg, size_t msglen);
		
		virtual size_t nodes_size() const;
		virtual size_t nodes(std::list<const netcomgrp::node *> *l);

		virtual const node *self() const;
		
		virtual int process(time_value_type &max_wait);
		virtual int process(time_value_type *max_wait = NULL);

		virtual reactor_type *reactor();
		virtual void          reactor(reactor_type *);
        
        /* TODO this is _DEFINITELY_ a temporarily solution that
         * must be replaced with a proper abstraction
         */
         virtual reudp::dgram_multi &dgram_adapter();        
	};
} // ns client
} // ns centralized
} // ns netcomgrp

#endif // _NETCOMGRP_CENTRALIZED_SERVER_GROUP_H_
