#ifndef _NETCOMGRP_SERVERLESS_PROTOCOL_DATA_STRING_H_
#define _NETCOMGRP_SERVERLESS_PROTOCOL_DATA_STRING_H_

#include <string>
#include "common.h"
#include "serializable.h"

namespace netcomgrp {
namespace serverless {
namespace protocol {

	class data_string : public std::string,
	                    public serializable {
	public:
		// Serializable interface
		virtual size_t serialize(reudp::msg_block_type *msg) const;
		virtual size_t deserialize(reudp::msg_block_type *msg);
		
		inline void operator=(const std::string &g) {
			std::string::operator=(g);
		}
	};	
} // ns protocol
} // ns serverless	
} // ns netcomgrp

#endif // _NETCOMGRP_SERVERLESS_PROTOCOL_DATA_STRING_H_
