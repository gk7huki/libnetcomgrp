#include "node.h"
#include "util.h"

namespace netcomgrp {
namespace serverless {

node::node() 
  : _state(unknown),
    _la(time_value_type::zero) {

}

node::node(const addr_inet_type &siga, time_value_type la) 
  : _addr(siga), // (util::addr_signal_to_addr_user(siga)), 
    // _addr_signal(siga),
    _state(unknown),
    _la(la),
    _ls(ACE_OS::gettimeofday())
{
}

node::~node() {}

const addr_inet_type &
node::addr() const {
	return _addr;
}

netcomgrp::node *
node::duplicate() const {
	node *n          = new node;
	n->_addr         = _addr;
	// n->_addr_signal  = _addr_signal;
	// n->_state        = _state;
	n->_la           = _la;
	n->_ls           = _ls;
	
	return n;
}

#if 0			
bool 
node::operator<(const netcomgrp::node &n) const {
	ACE_DEBUG((LM_DEBUG, "serverless::node checking < %d\n", &n));
	const node *sn = dynamic_cast<const node *>(&n);
	ACE_DEBUG((LM_DEBUG, "serverless::node checking < %d against %d\n", this, sn));
	// return (sn ? *this < *sn : false);
	bool ret = (sn ? *this < *sn : false);
	ACE_DEBUG((LM_DEBUG, "serverless::node returning %d\n", ret));
	return ret;
	
	// const node *sn = dynamic_cast<const node *>(&n);
	//return (sn ? *this < *sn : false);
}

bool
node::operator==(const netcomgrp::node &n) const {
	ACE_DEBUG((LM_DEBUG, "serverless::node checking against %d\n", &n));
	const node *sn = dynamic_cast<const node *>(&n);
	ACE_DEBUG((LM_DEBUG, "serverless::node checking2 against %d\n", sn));
	ACE_DEBUG((LM_DEBUG, "serverless::node checking %d against %d\n", this, sn));
	bool ret = (sn ? *this == *sn : false);
	ACE_DEBUG((LM_DEBUG, "serverless::node returning %d\n", ret));
	return ret;
	
	// const node *sn = dynamic_cast<const node *>(&n);
	// return (sn ? *this == *sn : false);
}
#endif
	
} // ns serverless	
} // ns netcomgrp
