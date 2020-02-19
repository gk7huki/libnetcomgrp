#ifndef _NETCOMGRP_SERVERLESS_VARIABLE_GUARD_H_
#define _NETCOMGRP_SERVERLESS_VARIABLE_GUARD_H_

#include "../exception.h"

namespace netcomgrp {
namespace serverless {

	template <class T>
	class variable_guard {
		T *_ptr;
		T  _unset_val;
	public:
		variable_guard(T *varptr, const T set) {
			_unset_val = *varptr;
			_ptr       =  varptr;
			*_ptr      =  set;
			
			if (_unset_val == set) {
				// This is for the needs of sub_system_chain and may not
				// be something desirable for other use cases.
				throw internal_errorf("state error, " \
				                 "variable_guard set and unset values same");
			}
			ACE_DEBUG((LM_DEBUG, "guard const: ref value %d\n", *_ptr));
		}
		~variable_guard() {
			*_ptr = _unset_val;
			ACE_DEBUG((LM_DEBUG, "guard destr: ref value %d\n", *_ptr));
		}
	};
		
} // ns serverless	
} // ns netcomgrp

#endif //_VARIABLE_GUARD_H_
