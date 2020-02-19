#ifndef _NETCOMGRP_CENTRALIZED_PROTOCOL_COMMON_H_
#define _NETCOMGRP_CENTRALIZED_PROTOCOL_COMMON_H_

#include <ace/Basic_Types.h>

namespace netcomgrp {
namespace centralized {
namespace protocol {
	const static int msg_unknown  = 1,
	                 msg_join     = 2,
		             msg_join_ack = 3,
		             msg_nodes_req  = 4,
		             msg_nodes_list = 5,
		             msg_leave      = 6,
		             
		             msg_node_joined = 7,
		             msg_node_left   = 8,
		             msg_send        = 9,

		             msg_keepalive  = 10;
		             
	
// TODO away, this is in newer ace versions
#ifndef ACE_UINT8
#define ACE_UINT8 unsigned char
#endif

} // ns protocol
} // ns serverless	
} // ns netcomgrp

#endif //_NETCOMGRP_CENTRALIZED_PROTOCOL_COMMON_H_
