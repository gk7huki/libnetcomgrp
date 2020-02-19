#ifndef _NETCOMGRP_CENTRALIZED_SUB_DHT_FIND_H_
#define _NETCOMGRP_CENTRALIZED_SUB_DHT_FIND_H_

#include <ace/Event_Handler.h>
#include <dht/client.h>

#include <string>

#include "group.h"
#include "sub_system.h"

namespace netcomgrp {
namespace serverless {
	using namespace std;

	class sub_dht_peer_find : public sub_system, 
	                          public dht::search_handler,
	                          public ACE_Event_Handler
	{
		bool _active;
		bool _finding;
		bool _self_in_dht_now;
		bool _self_in_dht;
		bool _first_find_done;
		
		// ACE timer id, used for announcing periodically 
		long _timer_id;
		time_value_type _last_find;
		time_value_type _curr_interval;
		
		// ACE timer id, used for announcing periodically 
		// long _timer_id;
		void _find();
		void _set_timer();
	public:
		sub_dht_peer_find(group *owner);
		virtual ~sub_dht_peer_find();
		
		virtual void join(const char *grp_id);
		virtual void leave();
		virtual void notice_receive(int notice_id);
		// void leave();

		/* dht::search_handler interface */				
		virtual int found(const dht::key &k, const dht::value &v);
		virtual void success();
		virtual void failure(int, const char *);

		// ACE_Event_Handler interface
		virtual int handle_timeout(const ACE_Time_Value &now, const void *arg);
		
	};
} // ns serverless	
} // ns netcomgrp

#endif //_NETCOMGRP_CENTRALIZED_SUB_DHT_FIND_H_
