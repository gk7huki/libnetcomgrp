#ifndef _NETCOMGRP_COMMON_H_
#define _NETCOMGRP_COMMON_H_

#include <ace/Log_Msg.h>
#include <ace/Addr.h>
#include <ace/INET_Addr.h>
#include <ace/Time_Value.h>
#include <ace/Reactor.h>

/**
 * @file    common.h
 * @date    Apr 9, 2005
 * @author  Arto Jalkanen
 * @brief   Some common definitions for Networked Communication Group
 *
 */

namespace netcomgrp {
	typedef unsigned long int addr_ip_type;
	typedef ACE_Addr          addr_type;
	typedef ACE_INET_Addr     addr_inet_type;
	typedef ACE_Time_Value    time_value_type;
	typedef ACE_UINT32        uint32_t;
	typedef ACE_Byte          byte_t;
	typedef ACE_Reactor       reactor_type;

	// various error codes that are thrown as exceptions
/*
	namespace exception_code {
		enum {
			unspecified   = 0,			
			unexpected,
			out_of_memory,
			use_error,
			call_error,
			internal,
			
			// Can be used for example in case the internal communication
			// protocol encountered an invalid packet
			protocol_error,
			
			// Codes below this are free for miscellaneous usage
			last,
		};
	}
*/
}

#endif //_NETCOMGRP_COMMON_H_
