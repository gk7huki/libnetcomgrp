#ifndef _NETCOMGRP_CENTRALIZED_SERVER_GROUP_H_
#define _NETCOMGRP_CENTRALIZED_SERVER_GROUP_H_

#include <map>

#include <reudp/reudp_multi.h>

#include "../../group.h"
#include "../node.h"

namespace netcomgrp {
namespace centralized {
namespace server {
	/**
	 * @brief Centralized server for network communication group
	 * 
	 * Implements a central server for network communication group 
	 * that keeps a list of nodes that are joined and serves joining
	 * nodes with list of other nodes joined.
	 * 
	 * Uses one UDP socket for communication and lifts heavily from
	 * the serverless implementation's internals. Can not be considered 
	 * robust, used for testing purposes.
	 */	
	class group : public netcomgrp::group {
		typedef std::map<addr_inet_type, node *> _peer_container_type;
		_peer_container_type _participants;
		reactor_type        *_reactor;
		
		class sub_comms     *_sub_comms;
		friend class sub_comms;

		node *_peer_find(const addr_inet_type &a);
		node *_peer_confirm(const addr_inet_type &a, bool *is_new = NULL);
		bool  _peer_send_timeout(const addr_inet_type &a);
		bool  _peer_left(const addr_inet_type &a);
		inline size_t _peer_participants_size() { return _participants.size(); }
		inline _peer_container_type::iterator
		       _peer_participants_begin() { return _participants.begin(); }
		inline _peer_container_type::iterator
		       _peer_participants_end() { return _participants.end(); }
		void   _peers_clear();
		node *_peer_from_container(_peer_container_type **peer_cont,
                                   _peer_container_type::iterator *i,
                                    const addr_inet_type &a);

		//int   _peer_user_data_read(const void *buf, size_t nread, 
		//                           const node *n);				
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
} // ns server
} // ns centralized
} // ns netcomgrp

#endif // _NETCOMGRP_CENTRALIZED_SERVER_GROUP_H_
