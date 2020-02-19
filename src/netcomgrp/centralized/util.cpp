#include <assert.h>

#include "../exception.h"
#include "util.h"

namespace netcomgrp {
namespace centralized {
namespace util {

size_t addr_serialize(void *vbuf, size_t len, const addr_inet_type &addr) {
	assert(len >= 6);
	char *cbuf = (char *)vbuf;
	ACE_UINT32 netip = ACE_HTONL(addr.get_ip_address());
	u_short    netpt = ACE_HTONS(addr.get_port_number());
	memcpy(cbuf,     &netip,  4);
	memcpy(cbuf + 4, &netpt,  2);
	return 6;
}

// Buf must have at least 6 bytes
size_t addr_deserialize(addr_inet_type *addr, const void *vbuf, size_t len) {
	assert(len >= 6);
	const char *cbuf        = static_cast<const char *>(vbuf);
	const ACE_UINT32 *netip = reinterpret_cast<const ACE_UINT32 *>(cbuf);
	const u_short    *netpt = reinterpret_cast<const u_short *>(cbuf + 4);

	// Address in the buffer is assumed to be in network byte order
	// already, so set is called with encode = 0
	addr->set(*netpt, *netip, 0);
	
	ACE_DEBUG((LM_DEBUG, "util::addr_deserialize: got address %s:%d\n",
	                     addr->get_host_addr(), 
	                     addr->get_port_number()));
	                     
	return 6;
}


} // ns util
} // ns serverless
} // ns netcomgrp

