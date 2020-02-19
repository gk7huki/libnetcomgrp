#ifndef _NETCOMGRP_CENTRALIZED_UTIL_H_
#define _NETCOMGRP_CENTRALIZED_UTIL_H_

#include <utility>
#include <functional>

#include <ace/Event_Handler.h>
#include <ace/Reactor.h>

#include "../common.h"
#include "node.h"

namespace netcomgrp {
namespace serverless {
namespace util {

// Buf must have at least 6 bytes
size_t addr_serialize(void *buf, size_t len, const addr_inet_type &addr);
// Buf must have at least 6 bytes
size_t addr_deserialize(addr_inet_type *addr, const void *buf, size_t len);

inline const addr_inet_type &
addr_node_pair_to_addr(const std::pair<addr_inet_type, node *> &from) {
	return from.second->addr_direct();
}

class addr_node_pair_to_addr_for_sending
  : public std::unary_function<std::pair<const addr_inet_type, node *>, addr_inet_type> {
private:
	time_value_type _set_time;
public:
	inline addr_node_pair_to_addr_for_sending(const time_value_type &upd_time) 
	  : _set_time(upd_time) {}
	  
	inline const addr_inet_type &operator()(
		const std::pair<addr_inet_type, node *> &i
	) {
		node *n = i.second;
		n->last_send(_set_time);
		return n->addr_direct();
	}
};

class node_to_addr_for_sending
  : public std::unary_function<node *, addr_inet_type> {
private:
	time_value_type _set_time;
public:
	inline node_to_addr_for_sending(const time_value_type &upd_time) 
	  : _set_time(upd_time) {}
	  
	inline const addr_inet_type &operator()(
		node *n
	) {
		n->last_send(_set_time);
		return n->addr_direct();
	}
};


} // ns util
} // ns serverless
} // ns netcomgrp

#endif //_UTIL_H_
