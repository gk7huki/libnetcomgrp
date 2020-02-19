#ifndef _NETCOMGRP_SERVERLESS_SUB_DHT_CONNECTION_H_
#define _NETCOMGRP_SERVERLESS_SUB_DHT_CONNECTION_H_

#include <dht/notify_handler.h>
#include "group.h"
#include "sub_system.h"

namespace netcomgrp {
namespace serverless {

	class sub_dht_connection : public sub_system,
	                           public dht::notify_handler
	{
		group *_owner;
		
		// internal state variables
		bool   _did_connect; // True if this module created the dht connection
		bool   _connecting;
		bool   _disconnecting;
		bool   _leaving;     // True if waiting for leaving conditions
		bool   _deannounced; // True if deannounced received
		
		void _reset_state();
		void _connection_detected();
	public:
		sub_dht_connection(group *owner);
		virtual ~sub_dht_connection();
		
		virtual void join(const char *grp_id);
		virtual void leave();
		virtual void notice_receive(int notice_id);

		// dht::notify_handler interface		
		virtual void success();
		virtual void failure(int error, const char *errstr);
	};
} // ns serverless	
} // ns netcomgrp

#endif //_NETCOMGRP_SERVERLESS_SUB_DHT_ANNOUNCE_H_
