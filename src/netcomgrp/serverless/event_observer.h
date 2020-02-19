#ifndef _NETCOMGRP_SERVERLESS_EVENT_OBSERVER_H_
#define _NETCOMGRP_SERVERLESS_EVENT_OBSERVER_H_

#include "../event_observer.h"

namespace netcomgrp {
namespace serverless {		
	class event_observer : public netcomgrp::event_observer {
	public:
		enum {
			mask_extra_state_changed  = event_observer::mask_last,
			mask_last                 = event_observer::mask_last << 1,
		};
	
		virtual int extra_state_changed(int) { return 0; }
	};
}
}

#endif // _NETCOMGRP_SERVERLESS_EVENT_OBSERVER_H_
