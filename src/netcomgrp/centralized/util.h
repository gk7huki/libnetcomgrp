#ifndef _NETCOMGRP_CENTRALIZED_UTIL_H_
#define _NETCOMGRP_CENTRALIZED_UTIL_H_

#include <utility>
#include <functional>

#include <ace/Event_Handler.h>
#include <ace/Reactor.h>

#include "../common.h"
#include "./node.h"
#include "./protocol/address_tuple.h"

namespace netcomgrp {
namespace centralized {
namespace util {

// Buf must have at least 6 bytes
size_t addr_serialize(void *buf, size_t len, const addr_inet_type &addr);
// Buf must have at least 6 bytes
size_t addr_deserialize(addr_inet_type *addr, const void *buf, size_t len);

class addr_node_pair_to_addr 
  : std::unary_function<std::pair<addr_inet_type, node *>, addr_inet_type> {
public:
	inline const result_type &operator()(const argument_type &i) {
		return i.second->addr();
	}	
};

class addr_tuple_to_addr 
  : std::unary_function<protocol::address_tuple, addr_inet_type> {
public:
	inline const addr_inet_type &operator()(const protocol::address_tuple &i) {
		return i.addr();
	}	
};

class node_to_addr 
  : std::unary_function<netcomgrp::node *, addr_inet_type> {
public:
	inline const addr_inet_type &operator()(const netcomgrp::node *n) {
		return n->addr();
	}	
};

inline bool addr_node_pair_less(const std::pair<addr_inet_type, node *> &a,
                         const addr_inet_type &at)
{
	return a.second->addr() < at;
}

inline bool addr_node_pair_less(const addr_inet_type &at,
                         const std::pair<addr_inet_type, node *> &a)
{
	return at < a.second->addr();
}

/*bool addr_node_pair_less(const protocol::address_tuple &at, 
                         const std::pair<addr_inet_type, node *> &a) 
{
	return at.addr() < a.second->addr();
}

bool addr_node_pair_less(const std::pair<addr_inet_type, node *> &a,
                         const protocol::address_tuple &at)
{
	return a.second->addr() < at.addr();
}*/

/*
inline const addr_inet_type &
addr_node_pair_to_addr(const std::pair<addr_inet_type, node *> &from) {
	return from.second->addr();
}
*/

} // ns util
} // ns centralized
} // ns netcomgrp

#endif //_UTIL_H_
