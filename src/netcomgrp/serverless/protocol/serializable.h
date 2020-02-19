#ifndef _NETCOMGRP_SERVERLESS_SERIALIZABLE_H_
#define _NETCOMGRP_SERVERLESS_SERIALIZABLE_H_

#include <reudp/common.h>

#include "common.h"

namespace netcomgrp {
namespace serverless {
namespace protocol {

	class serializable {
	public:
		virtual ~serializable();
		
		/**
		 * @brief Serialize the object to a message block
		 * @param msg pointer to the message buffer
		 * 
		 * Throw exception if does not fit. Advances the message blocks
		 * wr_ptr by the amount that was written.
		 * 
		 * @returns the number of bytes serialization consumed
		 */
		virtual size_t serialize(reudp::msg_block_type *msg) const = 0;
		/**
		 * @brief Deserializes the object from a flat buffer
		 * @param msg pointer to the message buffer
		 * 
		 * Throw exception if does not fit. Advances the message blocks
		 * rd_ptr by the amount that was read.
		 * 
		 * @returns the number of bytes deserialization used from the buffer
		 */
		virtual size_t deserialize(reudp::msg_block_type *msg) = 0;
	};
} // ns protocol
} // ns serverless	
} // ns netcomgrp

#endif //_NETCOMGRP_SERVERLESS_SERIALIZABLE_H_
