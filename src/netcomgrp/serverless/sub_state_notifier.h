#ifndef _NETCOMGRP_SERVERLESS_SUB_STATE_NOTIFIER_H_
#define _NETCOMGRP_SERVERLESS_SUB_STATE_NOTIFIER_H_

#include "sub_system.h"

namespace netcomgrp {
namespace serverless {

	class sub_state_notifier : public sub_system {
		// Leave mask
		enum {
			wait_leave_dht    = 0x1,
			wait_leave_signal = 0x2,
			
			wait_leave_all    = 0x3
		};
		unsigned int _wait_cond_leaving;
		bool         _leaving;
	public:
		sub_state_notifier(group *owner);
		virtual ~sub_state_notifier();
		
		virtual void join(const char *grp_id);
		virtual void leave();
		virtual void notice_receive(int notice_id);
	};
} // ns serverless	
} // ns netcomgrp

#endif //_NETCOMGRP_SERVERLESS_SUB_STATE_NOTIFIER_H_
