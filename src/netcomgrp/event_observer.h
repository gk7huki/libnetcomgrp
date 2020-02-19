#ifndef _NETCOMGRP_EVENT_OBSERVER_H_
#define _NETCOMGRP_EVENT_OBSERVER_H_

#include "common.h"
#include "node.h"

namespace netcomgrp {
	// Forward declaration of group
	class group;
	
	class event_observer {
		class group *_owner;
	public:
		event_observer();
		virtual ~event_observer();
		
		enum {
			mask_node_added    = 0x1,
			mask_node_removed  = 0x2,
			mask_data_received = 0x4,

			mask_state_changed = 0x8,
			mask_error         = 0x10,
			
			mask_last          = 0x20,
			mask_all           = 0xFFFF,
		};
		
		virtual int node_added(const node *)   { return 0; }
		virtual int node_removed(const node *) { return 0; }
		virtual int data_received(const void *, 
		                          size_t, 
		                          const node *) { return 0; }
		
		virtual int state_changed(int)  { return 0; }
		virtual int error(int, const char *) { return 0; }
		
/*		inline class group *owner();
		inline void         owner(class group *);
		*/
	};

/*	inline class group *
	event_observer::owner() { return _owner; }
	inline void
	event_observer::owner(class group *g) { _owner = g; }
	*/
}

#endif // _NETCOMGRP_EVENT_OBSERVER_H_
