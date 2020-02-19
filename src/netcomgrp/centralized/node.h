#ifndef _NETCOMGRP_SERVERLESS_NODE_H_
#define _NETCOMGRP_SERVERLESS_NODE_H_

#include "../node.h"

namespace netcomgrp {
namespace centralized {
	// Forward declarations for friend classes
	namespace server {
		class group;
	}
	namespace client {
		class group;
	}
	class node : public netcomgrp::node {
		addr_inet_type  _addr;
		int             _state;
		time_value_type _la;  // Last activity

		node();
		node(const addr_inet_type &a);
		virtual ~node();
		
		friend class server::group;
		friend class client::group;		
	public:
		const static int unknown = 1;

		inline time_value_type last_activity() const;
		inline void last_activity(const time_value_type &t);
		
		virtual const addr_inet_type &addr() const;

		virtual netcomgrp::node *duplicate() const;

#if 0						
		virtual bool operator<(const netcomgrp::node &n) const;
		virtual bool operator==(const netcomgrp::node &n) const;
#endif

		inline bool operator<(const node &n) const;
		inline bool operator==(const node &n) const;

		inline bool operator<(const addr_inet_type &a) const;
		inline bool operator==(const addr_inet_type &a) const;
	};

	inline time_value_type node::last_activity() const { return _la;}
	inline void node::last_activity(const time_value_type &t) { _la = t; }
	
	inline bool node::operator<(const node &n) const { return _addr < n._addr; }
	inline bool	node::operator==(const node &n) const { return _addr == n._addr; }
		
	inline bool node::operator<(const addr_inet_type &a) const { return _addr < a; }	
	inline bool	node::operator==(const addr_inet_type &a) const { return _addr == a;}
	
} // ns centralized
} // ns netcomgrp

#endif //_NETCOMGRP_GROUP_H_

