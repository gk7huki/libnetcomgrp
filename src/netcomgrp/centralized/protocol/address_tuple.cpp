#include "../util.h"
#include "address_tuple.h"

namespace netcomgrp {
namespace centralized {
namespace protocol {

address_tuple::~address_tuple() {}

size_t
address_tuple::serialize(reudp::msg_block_type *msg) const {
	// Serializing this object needs 7 bytes at least
	if (msg->space() < 7)
		throw internal_errorf(
			"address_tuple::serialize " \
			"not enough space (%d) to serialize object",
			msg->space()
		);
		                 
	ACE_UINT8 stype = _type;
	msg->copy((const char *)&stype, sizeof(ACE_UINT8));

	size_t n = util::addr_serialize(msg->wr_ptr(), msg->space(), _addr);
	msg->wr_ptr(n);
	return 1 + n;
}

size_t
address_tuple::deserialize(reudp::msg_block_type *msg) {
	// Deserializing this object needs 7 bytes at least
	if (msg->length() < 7)
		throw internal_errorf(
			"address_tuple::deserialize " \
			"not enough data (%d) to deserialize object",
			msg->length()
		);
	
	_type = static_cast<int>(*(msg->rd_ptr()));
	if (_type != direct)
		throw internal_errorf(
			"address_tuple::deserialize " \
			"invalid address type %d\n",
			_type
		);

	msg->rd_ptr(1);
	size_t n = util::addr_deserialize(&_addr, msg->rd_ptr(), msg->length());
	msg->rd_ptr(n);
	return 1 + n;
}
	
} // ns protocol
} // ns serverless	
} // ns netcomgrp
