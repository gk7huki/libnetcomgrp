#ifndef _NETCOMGRP_SERVERLESS_NODE_H_
#define _NETCOMGRP_SERVERLESS_NODE_H_

#include "../node.h"

namespace netcomgrp {
namespace serverless {
	class group;
	
	class node : public netcomgrp::node {
		addr_inet_type  _addr;
		// addr_inet_type  _addr_signal;
		int             _state;
		time_value_type _la;  // Last activity
		time_value_type _ls;  // Last send (to this node)
		
		node();
		node(const addr_inet_type &siga, time_value_type la);
		virtual ~node();
		
		friend class group;		
	public:
		const static int unknown = 1;

		inline time_value_type last_activity() const;
		inline time_value_type last_send()     const { return _ls; }
		inline void            last_send(const time_value_type &t) { _ls = t; }
		
		inline void last_activity(const time_value_type &t);
		
		virtual const addr_inet_type &addr() const;
		virtual netcomgrp::node *duplicate() const;
		
		inline const addr_inet_type &addr_direct() const;
		// inline const addr_inet_type &addr_user() const;
		// inline const addr_inet_type &addr_signal() const;

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

	inline const addr_inet_type &node::addr_direct() const { 
		return _addr; 
	}

/*
	inline const addr_inet_type &node::addr_signal() const { 
		return _addr_signal; 
	}
	inline const addr_inet_type &node::addr_user() const { 
		return _addr; 
	}
*/
	inline bool node::operator<(const node &n) const { return _addr < n._addr; }
	inline bool	node::operator==(const node &n) const { return _addr == n._addr; }
		
	inline bool node::operator<(const addr_inet_type &a) const { return _addr < a; }	
	inline bool	node::operator==(const addr_inet_type &a) const {	return _addr == a;}
	
} // ns serverless	
} // ns netcomgrp

#endif //_NETCOMGRP_GROUP_H_

