#include "sub_system.h"

namespace netcomgrp {
namespace serverless {
	
sub_system::sub_system(group *g, const char *idstr) 
	: _owner(g), _id(idstr) 
{
}

sub_system::~sub_system() {}
			
} // ns serverless	
} // ns netcomgrp
