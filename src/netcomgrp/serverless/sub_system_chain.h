#ifndef _NETCOMGRP_SERVERLESS_SUB_SYSTEM_CHAIN_H_
#define _NETCOMGRP_SERVERLESS_SUB_SYSTEM_CHAIN_H_

#include <list>
#include <queue>

#include "sub_system.h"

namespace netcomgrp {
namespace serverless {
	using namespace std;
	
	class sub_system_chain : public sub_system {
		typedef list<sub_system *> chain_type;
		queue<int> _notify_queue;
		chain_type _systems;
		bool       _notifying;
		
		void _process_notify_queue();

	public:
		sub_system_chain(group *g);
		virtual ~sub_system_chain();
		
		void add(sub_system *s);
		void notice_send(int signal_id);
		
		// sub_system interface ... each system is sent the same
		// signal.
		virtual void join(const char *grp_id);
		virtual void leave();
		virtual void peer_confirmed_new(node *n);

	private:
		virtual void notice_receive(int signal_id);
		
	};
	
} // ns serverless	
} // ns netcomgrp

#endif //_NETCOMGRP_SERVERLESS_SUB_SYSTEM_H_
