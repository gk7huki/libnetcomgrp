#include "node.h"
// #include "util.h"

namespace netcomgrp {
namespace centralized {

node::node() 
  : _state(unknown),
    _la(time_value_type::zero) {

}

node::node(const addr_inet_type &a) 
  : _addr(a),
    _state(unknown),
    _la(0) 
{
}

node::~node() {}

const addr_inet_type &
node::addr() const {
	ACE_DEBUG((LM_DEBUG, "centralized::node addr requested\n"));	
	return _addr;
}

netcomgrp::node *
node::duplicate() const {
	node *n = new node;
	n->_addr  = _addr;
	n->_state = _state;
	n->_la    = _la;
	
	return n;
}

#if 0
bool 
node::operator<(const netcomgrp::node &n) const {
	ACE_DEBUG((LM_DEBUG, "centralized::node checking < %d\n", &n));
	const node *sn = dynamic_cast<const node *>(&n);
	ACE_DEBUG((LM_DEBUG, "centralized::node checking < %d against %d\n", this, sn));
	// return (sn ? *this < *sn : false);
	bool ret = (sn ? *this < *sn : false);
	ACE_DEBUG((LM_DEBUG, "centralized::node returning %d\n", ret));
	return ret;
}

bool
node::operator==(const netcomgrp::node &n) const {
	ACE_DEBUG((LM_DEBUG, "centralized::node checking against %d\n", &n));
	const node *sn = dynamic_cast<const node *>(&n);
	ACE_DEBUG((LM_DEBUG, "centralized::node checking2 against %d\n", sn));
	ACE_DEBUG((LM_DEBUG, "centralized::node checking %d against %d\n", this, sn));
	bool ret = (sn ? *this == *sn : false);
	ACE_DEBUG((LM_DEBUG, "centralized::node returning %d\n", ret));
	return ret;
	// return (sn ? *this == *sn : false);
}
#endif

} // ns centralized
} // ns netcomgrp
