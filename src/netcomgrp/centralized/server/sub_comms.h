#ifndef _NETCOMGRP_SERVER_SUB_COMMS_H_
#define _NETCOMGRP_SERVER_SUB_COMMS_H_

#include <list>
#include <queue>

#include <ace/Event_Handler.h>
#include <reudp/reudp_multi.h>

#include "group.h"
#include "../node.h"
#include "../protocol/header.h"
#include "../protocol/address_tuple.h"

namespace netcomgrp {
namespace centralized {
namespace server {

	class sub_comms : // public sub_system,
	                  public ACE_Event_Handler {
		reudp::dgram_multi     _sock;
		reudp::msg_block_type *_msg;
		reudp::time_value_type _next_timeout;

		protocol::header       _dheader;
		
		typedef std::queue<addr_inet_type> _timeouted_type;
		_timeouted_type _timeouted;
		_timeouted_type _to_addr_timeouted;
		addr_inet_type _to_addr;
		
		bool   _msg_busy;  // True if msg is being used at the moment
		group *_owner;
		
		void _open(u_short port);
		void _close();
		void _send_msg_all();
		void _send_msg(const addr_inet_type &to);
		int  _send_watch();
		void _send_timer_watch();
		void _recv_watch();
		void _check_possible_peers();

		void _msg_prepare_resend(const addr_inet_type &f, 
		                         const void *data, 
		                         size_t datalen);
		void _resend(const addr_inet_type &f, const void *data, size_t datalen);
		void _resend(const addr_inet_type &f, const void *data, size_t datalen,
                     std::list<protocol::address_tuple> &nlist);

		// Sends to all participants info that node left
		void _send_join      (const addr_inet_type &f);
		void _send_join_ack  (const addr_inet_type &f);
		void _send_nodes_req (const addr_inet_type &f);
		void _send_nodes_list(const addr_inet_type &f);
		void _send_left      (const addr_inet_type &f);
		void _send_left      (const addr_inet_type &who, const addr_inet_type &to);
		void _send_joined    (const addr_inet_type &f);
		
		void _recv();
		void _recv_join      (const addr_inet_type &f, const protocol::header &hdr);
		void _recv_join_ack  (const addr_inet_type &f, const protocol::header &hdr);
		void _recv_leave     (const addr_inet_type &f, const protocol::header &hdr);
		void _recv_nodes_req (const addr_inet_type &f, const protocol::header &hdr);
		void _recv_nodes_list(const addr_inet_type &f, const protocol::header &hdr);
		void _recv_send      (const addr_inet_type &f, const protocol::header &hdr);

		static int _packet_done(int type, void *obj,
		                         const void *buf,
		                         size_t n,
		                         const reudp::addr_type &to);
		                         
		inline void _msg_start_use() {
			if (_msg_busy) throw unexpected_error(
			                     "_msg_start_use without matching _msg_stop_use");
			_msg_busy = true;
			_msg->reset();
		}
		inline void _msg_stop_use() {
			if (!_msg_busy) throw unexpected_error(
			                     "_msg_stop_use without matching _msg_start_use");
			_msg_busy = false;
		}
		inline bool _msg_in_use() { return _msg_busy; }
		
	public:
		sub_comms(group *owner);
		virtual ~sub_comms();

		virtual void join(const char *grp_id);
		virtual void leave();
		
		inline group *owner() { return _owner; }

		void send(const char *msg, size_t msglen, const node *recp);
		void send(const char *msg, size_t msglen);
		
		// The given node has timeouted, sends information about
		// that to other nodes
		void send_timeout(const node *n);				
		// ACE_Event_Handler interface
		virtual int handle_input(ACE_HANDLE handle);
		virtual int handle_output(ACE_HANDLE handle);
		
		virtual ACE_HANDLE get_handle() const {
			ACE_DEBUG((LM_DEBUG, "sub_comms: WARNING get handle called\n"));
			return ACE_Event_Handler::get_handle();
		}
		virtual int handle_timeout(const ACE_Time_Value &now, const void *arg);
        // End of ACE_Event_Handler interface
        
        /* TODO this is _DEFINITELY_ a temporarily solution that
         * must be replaced with a proper abstraction
         */
         inline reudp::dgram_multi &dgram_adapter() { 
            return _sock;
         }        
	};
} // ns server
} // ns centralized
} // ns netcomgrp

#endif //_NETCOMGRP_SERVER_SUB_COMMS_H_
