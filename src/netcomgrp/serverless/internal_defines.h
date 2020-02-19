#ifndef _NETCOMGRP_SERVERLESS_INTERNAL_DEFINES_H_
#define _NETCOMGRP_SERVERLESS_INTERNAL_DEFINES_H_

namespace netcomgrp {
namespace serverless {
namespace internal {
	// Peer types (group's _peer_possible returns these)
	enum {
		peer_known,
		peer_timeouted,
		peer_self,
		peer_new,
	};
} // ns internal	
} // ns serverless	
} // ns netcomgrp

#endif //_NETCOMGRP_SERVERLESS_INTERNAL_DEFINES_H_
