#ifndef _NETCOMGRP_CENTRALIZED_DGRAM_ADAPTER_REUDP_H_
#define _NETCOMGRP_CENTRALIZED_DGRAM_ADAPTER_REUDP_H_

#include <reudp/reudp_multi.h>

#include "dgram_adapter.h"

namespace netcomgrp {
namespace serverless {	
	class dgram_adapter_reudp : public dgram_adapter {
		reudp::dgram_multi _dgram;
		char *_buf;
		const static size_t _buf_len = 65536;
		packet_done_cb_type _pdone_cb;
		
	public:
		dgram_adapter_reudp();
		virtual ~dgram_adapter_reudp();

		virtual void packet_done_cb(packet_done_cb_type cb, void *param);
		virtual bool            needs_to_send();
		virtual time_value_type needs_to_send_when();

		// Throw exception if unable to open
		virtual void open(u_short port);
		virtual void close();
		
		virtual size_t send(const void *buf, size_t len, 
		                    const addr_inet_type &recp);
		virtual size_t send(const void *buf, size_t len, 
		                    std::list<addr_inet_type>::const_iterator istart,
		                    std::list<addr_inet_type>::const_iterator iend);
		virtual size_t recv(char **buf, addr_inet_type *from);

		virtual ACE_HANDLE get_handle() const { return _dgram.get_handle(); }		
	};

} // ns serverless	
} // ns netcomgrp

#endif //_NETCOMGRP_CENTRALIZED_DGRAM_ADAPTER_H_
