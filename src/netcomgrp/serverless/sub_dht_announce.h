#ifndef _NETCOMGRP_SERVERLESS_SUB_DHT_ANNOUNCE_H_
#define _NETCOMGRP_SERVERLESS_SUB_DHT_ANNOUNCE_H_

#include <ace/Event_Handler.h>
#include <dht/client.h>

#include <string>

#include "group.h"
#include "sub_system.h"

namespace netcomgrp {
namespace serverless {
	using namespace std;
	
	class sub_dht_announce : public sub_system,
	                         public dht::notify_handler,
	                         public ACE_Event_Handler
	{
		string  _group_id;

		bool _announcing;
		bool _deannouncing;
		
		bool _active;  // True if this module is active
		
		// time_value_type _announce_interval;
		// ACE timer id, used for announcing periodically 
		long _timer_id;
		time_value_type _last_find;
		time_value_type _curr_interval;
		time_value_type _min_interval;
		
		void _announce();
		void _deannounce();
		void _set_timer(bool self_not_found = false);		
		void _prepare_store(dht::key *k, dht::value *v);
	public:
		sub_dht_announce(group *owner);
		virtual ~sub_dht_announce();
		
		virtual void join(const char *grp_id);
		virtual void leave();
		virtual void notice_receive(int notice_id);

		// dht::notify_handler interface		
		virtual void success();
		virtual void failure(int error, const char *errstr);
		
		// ACE_Event_Handler interface
		virtual int handle_timeout(const ACE_Time_Value &now, const void *arg);
	};
} // ns serverless	
} // ns netcomgrp

#endif //_NETCOMGRP_SERVERLESS_SUB_DHT_ANNOUNCE_H_
