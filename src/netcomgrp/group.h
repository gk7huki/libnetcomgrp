#ifndef _NETCOMGRP_GROUP_H_
#define _NETCOMGRP_GROUP_H_

#include <list>

#include <reudp/reudp_multi.h>

#include "common.h"
#include "node.h"
#include "event_observer.h"
#include "event_observer_notifier.h"

namespace netcomgrp {
	class group {
		event_observer_notifier *_obs_notifier;
	protected:
		inline event_observer_notifier *observer_notifier() {
			return _obs_notifier;
		}
		inline event_observer_notifier *
		observer_notifier(event_observer_notifier *on) {
			return _obs_notifier = on;
		}
		// void event_occurred(int ev_type);
	public:
		// State of group
		enum {
			not_joined = 1,
			joining,
			joined,
			leaving,
			last,
		};

		// Non-fatal error codes (observable with event_observer)
		enum {
			err_unspecified = 0,
			err_connect,
			err_timeout_connect,
			err_timeout_read,
			err_timeout_write,
			err_io_read,
			err_io_write,
			// Codes below this are free for miscellaneous usage by derived
			// class for example
			err_last,
		};
		
		/**
		 * @brief Returns the current state of the group object
		 */		
		virtual int in_state();
		/**
		 * @brief Returns the current state as a string
		 */
	 	virtual const char *in_state_str();
		/**
		 * @brief Returns the state as a string
		 */
	 	virtual const char *state_str(int state);
	
		group();
		/**
		 * @brief Destructor
		 * 
		 * When destructor called the implementation should clean up as
		 * quickly as possible. If any handlers are pending they do
		 * not need to be called. For clean shutdown disconnect() should
		 * be called first (if connect() has been called) */
		virtual ~group();

		virtual void observer_attach(event_observer *h,
		                             int event_mask);
		virtual void observer_remove(event_observer *h);
		virtual void observer_remove(event_observer *h, int event_mask);
		
		virtual void join(const char *group_id) = 0;
		virtual void leave() = 0;
		
		// First sends to the specified recipient of the group, second 
		// form to the whole group
		virtual void send(const char *msg, size_t msglen, const node *recp) = 0;
		virtual void send(const char *msg, size_t msglen) = 0;
		// TODO where's the interface to sending to multiple nodes the same
		// message?
		
		// TODO I don't like this solution for getting the list of nodes.
		virtual size_t nodes_size() const = 0;
		virtual size_t nodes(std::list<const node *> *l) = 0;
		
		/**
		 * Returns the node representing self in the group.
		 * 
		 * Can return NULL if not in joined status.
		 */
		virtual const node *self() const = 0;
		
		virtual int process(time_value_type &max_wait) = 0;
		virtual int process(time_value_type *max_wait = NULL) = 0;

		/**
		 * @brief Returns the reactor which is used for event demultiplexing
		 * 
		 * Besides calling process(), which usually should use internally
		 * (ACE's) reactor for its processing needs, the application can
		 * directly call reactor. The netcomgrp implementation registers
		 * the events it looks for to the reactor and thus will be notified
		 * accordingly if the application is running in an reactor loop.
		 * Calling directly the reactor is an alternative to using process()
		 * and usually the recommended way if the application needs to look
		 * for other I/O events. See ACE documentation for
		 * information on reactor usage.
		 * 
		 * By default implementations should use ACE_Reactor::instance()
		 * as their reactor unless otherwise set by the application.
		 */
		virtual reactor_type *reactor() = 0;

		/**
		 * @brief Sets the reactor which is used for event demultiplexing
		 * @param reactor The reactor DHT implementation should use. 
		 *                
		 * Besides calling process(), which usually should use internally
		 * (ACE's) reactor for its processing needs, the application can
		 * directly call reactor. The netcomgrp implementation registers
		 * the events it looks for to the reactor and thus will be notified
		 * accordingly if the application is running in an reactor loop.
		 * Calling directly the reactor is an alternative to using process()
		 * and usually the recommended way if the application needs to look
		 * for other I/O events. See ACE documentation for
		 * information on reactor usage.
		 * 
		 * By default implementations should use ACE_Reactor::instance()
		 * as their reactor unless otherwise set by the application.
		 */
		virtual void          reactor(reactor_type *) = 0;
        
        /* TODO this is _DEFINITELY_ a temporarily solution that
         * must be replaced with a proper abstraction
         */
         virtual reudp::dgram_multi &dgram_adapter() = 0;
	};
	
}

#endif // _NETCOMGRP_GROUP_H_
