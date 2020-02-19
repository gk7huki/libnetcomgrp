#ifndef _NETCOMGRP_NODE_H_
#define _NETCOMGRP_NODE_H_

#include "common.h"

namespace netcomgrp {
	class node {
	public:
		node();
		virtual ~node();

		// May return all zero address if the node's is not provided
		virtual const addr_inet_type &addr() const = 0;

		// Make a copy of the node
		virtual node *duplicate() const = 0;
		
		// The default operations are to compare the addr() returned,
		// sub-classes can override the default behaviour if needed.
		virtual bool operator<(const node &n)  const;
		virtual bool operator==(const node &n) const;
	};
	
}
#endif //_NETCOMGRP_NODE_H_
