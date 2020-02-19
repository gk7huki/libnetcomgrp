#include "node.h"

namespace netcomgrp {

node::node() {}
node::~node() {}

bool 
node::operator<(const node &n) const {
	return addr() < n.addr();
}

bool
node::operator==(const node &n) const {
	return addr() == n.addr();
}

}
