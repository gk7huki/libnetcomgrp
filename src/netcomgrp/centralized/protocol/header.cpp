#include "header.h"

namespace netcomgrp {
namespace centralized {
namespace protocol {
	
size_t
header::serialize(reudp::msg_block_type *msg) const {
	size_t n = 0;

	// ACE_UINT32 wrtime = ACE_OS::gettimeofday().sec();
	
	n += util::serialize_basic_data(msg, _magic_id);
	n += util::serialize_basic_data(msg, _version);
	n += util::serialize_basic_data(msg, _type);
	// n += util::serialize_basic_data(msg, wrtime);
	n += util::serialize(msg, _addr);
	// n += util::serialize(msg, _addr_recver);
	// n += util::serialize(msg, _group);
	
	return n;
}

size_t
header::deserialize(reudp::msg_block_type *msg) {
	size_t n = 0;

	n += util::deserialize_basic_data(msg, &_magic_id);
	n += util::deserialize_basic_data(msg, &_version);
	n += util::deserialize_basic_data(msg, &_type);
	// n += util::deserialize_basic_data(msg, &_time);
	n += util::deserialize(msg, &_addr);
	// n += util::deserialize(msg, &_addr_recver);
	// n += util::deserialize(msg, &_group);

	return n;
}
	
} // ns protocol
} // ns centralized
} // ns netcomgrp
