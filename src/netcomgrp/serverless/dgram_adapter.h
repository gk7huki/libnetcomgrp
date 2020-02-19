#ifndef _NETCOMGRP_SERVERLESS_DGRAM_ADAPTER_H_
#define _NETCOMGRP_SERVERLESS_DGRAM_ADAPTER_H_

#include <list>

#include "../common.h"

namespace netcomgrp {
namespace serverless {	
	class dgram_adapter {
	public:
		virtual ~dgram_adapter();

		// Throw exception if unable to open
		virtual void open(u_short port) = 0;
		virtual void close() = 0;

		struct packet_done {
			const static int success = 1;
			const static int timeout = 2;
			const static int failure = 3;
		};

		typedef int (*packet_done_cb_type)(int, void  *param,
		                                   const void *buf, 
		                                   size_t n,
		                                   const addr_type &addr);

		virtual void packet_done_cb(packet_done_cb_type cb, void *param) = 0;

		// Returns true if there is a need to call send (possibly without
		// any data) to send data from the queues (acks, timeouted resends etc.)
		virtual bool            needs_to_send() = 0;
		virtual time_value_type needs_to_send_when() = 0;
		
		virtual size_t send(const void *buf, size_t len, 
		                    const addr_inet_type &recp) = 0;
		virtual size_t send(const void *buf, size_t len, 
		                    std::list<addr_inet_type>::const_iterator istart,
		                    std::list<addr_inet_type>::const_iterator iend) = 0;
		virtual size_t recv(char **buf, addr_inet_type *from) = 0;
		
		virtual ACE_HANDLE get_handle() const = 0;
	};

} // ns serverless	
} // ns netcomgrp

#endif //_NETCOMGRP_SERVERLESS_DGRAM_ADAPTER_H_
