#include "../../common.h"
#include "../../exception.h"
#include "common.h"
#include "data_string.h"
#include "util.h"

namespace netcomgrp {
namespace centralized {
namespace protocol {

size_t
data_string::serialize(reudp::msg_block_type *msg) const {
	// Serializing this object needs 1 byte for size information and
	// as many bytes as are in the string for the rest.
	if (msg->space() < 1 + size())
		throw internal_errorf(
			"data_string::serialize " \
			"not enough space (%d) to serialize object, " \
			"needs %d bytes",
			msg->space(), 1 + size()
		);
	if (size() > 255)
		throw internal_errorf(
			"data_string::serialize " \
			"string is too big to serialize, " \
			"size is %d bytes, allowed max 256 bytes",
			size()
		);

	ACE_UINT8 wsize = size();
	util::serialize_basic_data(msg, wsize);
	
	msg->copy(data(), wsize);
	
	return 1 + wsize;
}

size_t
data_string::deserialize(reudp::msg_block_type *msg) {
	if (msg->length() < 1)
		throw internal_errorf(
			"data_string::deserialize " \
			"not enough data to deserialize object, " \
			"needs at least 1 bytes"
		);

	ACE_UINT8 rsize = static_cast<ACE_UINT8>(*(msg->rd_ptr()));

	if (msg->length() - 1 < rsize)
		throw internal_errorf(
			"data_string::deserialize " \
			"not enough data to deserialize, " \
			"data available %d bytes, needs %d bytes",
			msg->length() - 1, rsize
		);
	
	assign(msg->rd_ptr() + 1, rsize);
	msg->rd_ptr(1 + rsize);
	
	return 1 + rsize;
}
	
} // ns protocol
} // ns serverless	
} // ns netcomgrp
