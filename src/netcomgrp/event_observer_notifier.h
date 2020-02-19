#ifndef _NETCOMGRP_EVENT_OBSERVER_NOTIFIER_H_
#define _NETCOMGRP_EVENT_OBSERVER_NOTIFIER_H_

#include <map>
#include <list>

#include "event_observer.h"

namespace netcomgrp {
	class event_observer_notifier {
		// typedef std::list<event_observer *>        _observer_list_type;
		typedef std::map<int, event_observer *> _observer_map_type;
		_observer_map_type                      _container;
		
		int _last_received_state;

	protected:
		inline event_observer *_find_obs(int ev_type);
	public:
		event_observer_notifier();
		virtual ~event_observer_notifier();

		void observer_attach(event_observer *h, int event_mask);
		void observer_remove(event_observer *h);
		void observer_remove(event_observer *h, int event_mask);
				
		inline int node_added(const node *);
		inline int node_removed(const node *);
		inline int data_received(const void *data, 
		                         size_t len, 
		                         const node *from);

		inline int state_changed(int state);
		inline int last_received_state() const;
		inline int error(int err, const char *desc);
	};

	inline event_observer *
	event_observer_notifier::_find_obs(int ev_type) {
		_observer_map_type::iterator i = _container.find(ev_type);
		if (i != _container.end()) return i->second;
		return NULL;
	}

	inline int
	event_observer_notifier::node_added(const node *n) {
		event_observer *o = _find_obs(event_observer::mask_node_added);

		return (o ? o->node_added(n) : 0);
	}

	inline int 
	event_observer_notifier::node_removed(const node *n) {
		event_observer *o = _find_obs(event_observer::mask_node_removed);
		return (o ? o->node_removed(n) : 0);
	}

	inline int
	event_observer_notifier::data_received(const void *data, 
	                         size_t len, 
	                         const node *from) 
	{
		event_observer *o = _find_obs(event_observer::mask_data_received);
		return (o ? o->data_received(data, len, from) : 0);
	}		

	inline int 
	event_observer_notifier::state_changed(int state) {
		_last_received_state = state;
		event_observer *o = _find_obs(event_observer::mask_state_changed);
		return (o ? o->state_changed(state) : 0);
	}

	inline int
	event_observer_notifier::last_received_state() const {
		return _last_received_state;
	}

	inline int 
	event_observer_notifier::error(int err, const char *desc) {
		event_observer *o = _find_obs(event_observer::mask_error);
		return (o ? o->error(err, desc) : 0);
	}
	
}

#endif // _NETCOMGRP_EVENT_OBSERVER_NOTIFIER_H_
