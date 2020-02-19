#ifndef _NETCOMGRP_CENTRALIZED_PROTOCOL_ADDRESS_TUPLE_H_
#define _NETCOMGRP_CENTRALIZED_PROTOCOL_ADDRESS_TUPLE_H_

#include "../../common.h"
#include "../../exception.h"

#include "common.h"

#include "serializable.h"

namespace netcomgrp {
namespace centralized {
namespace protocol {

	class address_tuple : public serializable {
	public:
		// IPv4
		const static int direct = 0;
		// Relayed not implemented, all nodes must be able to
		// access directly other nodes (no NAT)
		const static int relayed = 1;
	private:
		addr_inet_type _addr;
		addr_inet_type _addr_proxy;
		int            _type;
	public:

		inline address_tuple();
		inline address_tuple(const addr_inet_type &a, int t = direct);
		inline address_tuple(const address_tuple &o);
		virtual ~address_tuple();
		
		inline const addr_inet_type &addr() const;
		inline int   addr_type() const;

		inline const addr_inet_type &addr_proxy() const;
		inline void                  addr_proxy(const addr_inet_type &a);
		
		inline const address_tuple &operator=(const address_tuple &o);
		
		// Serializable interface
		virtual size_t serialize(reudp::msg_block_type *msg) const;
		virtual size_t deserialize(reudp::msg_block_type *msg);
	};
	
	inline address_tuple::address_tuple() : _type(address_tuple::direct) {}
	inline address_tuple::address_tuple(const addr_inet_type &a, int t)
	  : _addr(a), _type(t) {}
	inline address_tuple::address_tuple(const address_tuple &o)
	  : serializable(o), _addr(o._addr), _type(o._type) {}
	
	inline const addr_inet_type &
	address_tuple::addr() const { return _addr; }

	inline const addr_inet_type &
	address_tuple::addr_proxy() const { return _addr_proxy; }
	inline void
	address_tuple::addr_proxy(const addr_inet_type &a) { _addr_proxy = a; }
	
	inline int
	address_tuple::addr_type() const { return _type; }
	
	inline const address_tuple &
	address_tuple::operator=(const address_tuple &o) {
		serializable::operator=(o);
		_addr       = o._addr;
		_addr_proxy = o._addr_proxy;
		_type = o._type;
		
		return *this;
	}
		
} // ns protocol
} // ns centralized	
} // ns netcomgrp

#endif //_NETCOMGRP_CENTRALIZED_PROTOCOL_ADDRESS_TUPLE_H_
