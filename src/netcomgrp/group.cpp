#include "event_observer_notifier.h"
#include "group.h"

namespace netcomgrp {
	group::group() : _obs_notifier(new event_observer_notifier)
	{
	}
	
	group::~group() {
		delete _obs_notifier;
	}

	int group::in_state() {
		return observer_notifier()->last_received_state();
	}

	const char *group::in_state_str() {
		return state_str(in_state());
	}

	const char *group::state_str(int state) {
		switch (state) {
		case not_joined: return "not joined";
		case joining:    return "joining";
		case joined:     return "joined";
		case leaving:    return "leaving";
		}
		return "<unknown>";
	}
	
	void group::observer_attach(event_observer *h,
	                            int event_mask)
	{
		observer_notifier()->observer_attach(h, event_mask);
	}
	void group::observer_remove(event_observer *h) {
		observer_notifier()->observer_remove(h);
	}
	void group::observer_remove(event_observer *h, int event_mask) {
		observer_notifier()->observer_remove(h, event_mask);
	}
}

