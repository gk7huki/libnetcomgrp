#ifndef _NETCOMGRP_SERVERLESS_EVENT_OBSERVER_NOTIFIER_H_
#define _NETCOMGRP_SERVERLESS_EVENT_OBSERVER_NOTIFIER_H_

#include "../event_observer_notifier.h"
#include "event_observer.h"

namespace netcomgrp {
namespace serverless {		
	class event_observer_notifier : public netcomgrp::event_observer_notifier {
	public:
		inline int extra_state_changed(int state);				
	};

	inline int 
	event_observer_notifier::extra_state_changed(int state) {
		event_observer *o 
		  = dynamic_cast<event_observer *>(
		  	_find_obs(event_observer::mask_extra_state_changed)
		  );
		return (o ? o->extra_state_changed(state) : 0);
	}
}
}

#endif // _NETCOMGRP_SERVERLESS_EVENT_OBSERVER_NOTIFIER_H_
