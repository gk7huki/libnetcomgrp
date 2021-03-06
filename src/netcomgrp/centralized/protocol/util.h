#ifndef _NETCOMGRP_CENTRALIZED_PROTOCOL_UTIL_H_
#define _NETCOMGRP_CENTRALIZED_PROTOCOL_UTIL_H_

#include <string.h>
#include <reudp/common.h>
#include "serializable.h"

namespace netcomgrp {
namespace centralized {
namespace protocol {
namespace util {

// TODO specialized template for short and long
template<class T> inline size_t
serialize_basic_data(reudp::msg_block_type *msg, const T data) {
	size_t s = sizeof(T);
	ACE_DEBUG((LM_DEBUG, "protocol::util::serialize_basic_data size: %d\n", s));

	T wdata;
	
	if (s == sizeof(ACE_INT16)) {
		ACE_DEBUG((LM_DEBUG, "protocol::util::serialize_basic_data htons\n"));
		wdata = htons(data);
	} else if (s == sizeof(ACE_INT32)) {
		ACE_DEBUG((LM_DEBUG, "protocol::util::serialize_basic_data htonl\n"));		
		wdata = htonl(data);
	} else {
		wdata = data;
	}
	// Copy throws an exception if not enough space for serialization, so
	// no implicit checking needed
	msg->copy(reinterpret_cast<const char *>(&wdata), s);
	return s;
}

inline size_t
serialize(reudp::msg_block_type *msg, const serializable &so) {
	size_t n = so.serialize(msg);
	ACE_DEBUG((LM_DEBUG, "protocol::util::write_serializable size: %d\n", n));
	// TODO network byte order for short and long
	return n;
}

// TODO specialized template for short and long
template<class T> inline size_t
deserialize_basic_data(reudp::msg_block_type *msg, T *data) {
	size_t s = sizeof(T);
	ACE_DEBUG((LM_DEBUG, "protocol::util::deserialize_basic_data size: %d\n", s));
	if (msg->length() < s) {
		throw internal_errorf(
			"protocol::util::deserialize_basic_data: " \
			"not enough data to deserialize object, " \
			"needs at least %d bytes", s
		);
	}
	memcpy(data, msg->rd_ptr(), s);
	if (s == sizeof(ACE_INT16)) {
		ACE_DEBUG((LM_DEBUG, "protocol::util::deserialize_basic_data ntohs\n"));
		*data = ntohs(*data);
	} else if (s == sizeof(ACE_INT32)) {
		ACE_DEBUG((LM_DEBUG, "protocol::util::deserialize_basic_data ntohs\n"));		
		*data = ntohl(*data);
	}
	msg->rd_ptr(s);
	return s;
}

inline size_t
deserialize(reudp::msg_block_type *msg, serializable *so) {
	size_t n = so->deserialize(msg);
	ACE_DEBUG((LM_DEBUG, "protocol::util::deserialize size: %d\n", n));
	return n;
}

} // ns util
} // ns protocol
} // ns serverless	
} // ns netcomgrp


#endif //_NETCOMGRP_CENTRALIZED_PROTOCOL_UTIL_H_
