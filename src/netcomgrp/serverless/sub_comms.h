#ifndef _NETCOMGRP_SERVERLESS_SUB_COMMS_H_
#define _NETCOMGRP_SERVERLESS_SUB_COMMS_H_

#include <list>

#include <ace/Event_Handler.h>
#include <reudp/reudp_multi.h>

#include "node.h"
#include "sub_system.h"
#include "protocol/header.h"

namespace netcomgrp {
namespace serverless {

	class sub_comms : public sub_system,
	                  public ACE_Event_Handler {
		reudp::dgram_multi     _sock;
		reudp::msg_block_type *_msg_send;
		reudp::msg_block_type *_msg_recv;
		reudp::time_value_type _next_timeout;
				
		protocol::header       _dheader;

		typedef std::queue<addr_inet_type> _timeouted_type;
		_timeouted_type _timeouted;

		u_short _opened_port;
		u_short _public_port;
		bool    _keepalive_mode;
		long    _keepalive_timer;
		
		void _open(u_short port);
		void _close();		
		int  _send_watch();
		void _send_timer_watch();
		void _recv_watch();
		void _check_possible_peers();
		void _send_join(node *n);
		void _send_join_ack(node *n);
		void _send_leave();
		void _send_nodes_req(node *n);
		void _send_nodes_list(node *n);
		void _send_my_ip_resp(node *n);
		void _send_my_ip_req(node *n);
		
		inline void _send_msg(node *n) {
			n->last_send(ACE_OS::gettimeofday());
			_sock.send(_msg_send->base(), _msg_send->length(), n->addr());
		}
		inline void _send_msg(
			std::map<addr_inet_type, node *>::iterator nbegin,
			std::map<addr_inet_type, node *>::iterator nend			
		) {
			time_value_type now = ACE_OS::gettimeofday();
			_sock.send_multi(_msg_send->base(), _msg_send->length(), 
			                 nbegin, 
			                 nend,
			                 util::addr_node_pair_to_addr_for_sending(now));
		}
		inline void _send_msg(
			std::list<node *>::iterator nbegin,
			std::list<node *>::iterator nend
	    ) {
			time_value_type now = ACE_OS::gettimeofday();
			_sock.send_multi(_msg_send->base(), _msg_send->length(), 
			                 nbegin,
			                 nend,
			                 util::node_to_addr_for_sending(now));
		}
		
		void _recv();
		void _recv_join      (node *n, const protocol::header &hdr);
		void _recv_join_ack  (node *n, const protocol::header &hdr);
		void _recv_leave     (node *n, const protocol::header &hdr);
		void _recv_nodes_req (node *n, const protocol::header &hdr);
		void _recv_nodes_list(node *n, const protocol::header &hdr);
		void _recv_send      (node *n, const protocol::header &hdr);
		void _recv_my_ip_req (node *n, const protocol::header &hdr);
		void _recv_my_ip_resp(node *n, const protocol::header &hdr);

		static int _packet_done(int type, void *obj,
		                        const void *buf,
		                        size_t n,
		                        const reudp::addr_type &to);
	public:
		sub_comms(group *owner);
		virtual ~sub_comms();

		virtual void join(const char *grp_id);
		virtual void leave();
		virtual void send(const char *msg, size_t msglen, const node *recp);
		virtual void send(const char *msg, size_t msglen);		
		virtual void notice_receive(int notice_id);
		virtual void peer_confirmed_new(node *n);
				
		// ACE_Event_Handler interface
		virtual int handle_input(ACE_HANDLE handle);
		virtual int handle_output(ACE_HANDLE handle);
		virtual int handle_timeout(const ACE_Time_Value &now, const void *arg);
		
		virtual ACE_HANDLE get_handle() const {
			ACE_DEBUG((LM_DEBUG, "sub_dgram_new: WARNING get handle called\n"));
			return ACE_Event_Handler::get_handle();
		}
        // End of ACE_Event_Handler interface
        
        /* TODO this is _DEFINITELY_ a temporarily solution that
         * must be replaced with a proper abstraction
         */
         inline reudp::dgram_multi &dgram_adapter() { 
            return _sock;
         }  
        
	};
} // ns serverless	
} // ns netcomgrp

#endif //_NETCOMGRP_SERVERLESS_SUB_COMMS_H_
