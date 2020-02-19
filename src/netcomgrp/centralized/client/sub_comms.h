#ifndef _NETCOMGRP_SERVER_SUB_COMMS_H_
#define _NETCOMGRP_SERVER_SUB_COMMS_H_

#include <ace/Event_Handler.h>

#include <list>

#include <reudp/reudp_multi.h>

#include "group.h"
#include "../node.h"
#include "../protocol/data_list.h"
#include "../protocol/address_tuple.h"
#include "../protocol/header.h"

namespace netcomgrp {
namespace centralized {
namespace client {

	class sub_comms : // public sub_system,
	                  public ACE_Event_Handler {
		reudp::dgram_multi     _sock;
		reudp::msg_block_type *_msg;
		reudp::msg_block_type *_msg_recv;
		reudp::time_value_type _next_timeout;
		reudp::addr_inet_type  _addr_server;

		group *_owner;

		enum {
			ex_none = 0,
			ex_close_socket,
		} _exception_command;
		
		protocol::header       _dheader;

		u_short _opened_port;
		u_short _public_port;
		bool    _keepalive_mode;
		long    _keepalive_timer;

				
		void _open();
		void _close();
		void _send_all();
		int  _send_watch();
		void _send_timer_watch();
		void _recv_watch();
		void _check_possible_peers();

		inline void _send_server();
		void _send_join      ();
		void _send_leave     ();
		void _send_nodes_req ();
		
		void _recv();
		void _recv_join_ack   (const addr_inet_type &f, const protocol::header &hdr);
		void _recv_node_joined(const addr_inet_type &f, const protocol::header &hdr);
		void _recv_node_left  (const addr_inet_type &f, const protocol::header &hdr);
		void _recv_nodes_list (const addr_inet_type &f, const protocol::header &hdr);
		void _recv_send       (const addr_inet_type &f, const protocol::header &hdr);
		void _recv_left       (const addr_inet_type &f, const protocol::header &hdr);

		static int _packet_done(int type, void *obj,
		                         const void *buf,
		                         size_t n,
		                         const reudp::addr_type &to);
	public:
		sub_comms(group *owner);
		virtual ~sub_comms();

		virtual int join(const char *grp_id);
		virtual void leave();
		
		inline group *owner() { return _owner; }

		void send(const char *msg, size_t msglen, const node *recp);
		void send(const char *msg, size_t msglen);

		void send(const char *msg, size_t msglen,
                  const protocol::data_list<protocol::address_tuple> &nlist);
		void send(const char *msg, size_t msglen,
                  std::list<const netcomgrp::node *>::const_iterator start,
                  std::list<const netcomgrp::node *>::const_iterator stop);

		// Sends to all participants info that node left
		void send_left(const node *n);
		
		// Sends to all participants info node that joined
		void send_joined(const node *n);
		
		// virtual void notice_receive(int notice_id);
		// virtual void peer_confirmed_new(node *n);
				
		// ACE_Event_Handler interface
		virtual int handle_input(ACE_HANDLE handle);
		virtual int handle_output(ACE_HANDLE handle);
		virtual int handle_exception(ACE_HANDLE handle);
		virtual int handle_close(ACE_HANDLE handle, ACE_Reactor_Mask m);
		
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
} // ns client
} // ns centralized
} // ns netcomgrp

#endif //_NETCOMGRP_SERVER_SUB_COMMS_H_
