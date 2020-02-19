#include <string.h>

#include "../exception.h"
#include "sub_dht_peer_find.h"
#include "util.h"
#include "internal_defines.h"

using namespace std;

namespace netcomgrp {
namespace serverless {

sub_dht_peer_find::sub_dht_peer_find(group *owner) 
	: sub_system(owner, "peer_find"),
	_active(false),
	_finding(false),
	_self_in_dht_now(false),
	_self_in_dht(false),
	_first_find_done(false),
	_timer_id(-1)
{
}

sub_dht_peer_find::~sub_dht_peer_find() {
	_active = false;
	if (_finding) {
		dht::client *d = owner()->dht_client();
		d->handler_cancel(this);
	}
	if (_timer_id != -1) {
		owner()->reactor()->cancel_timer(_timer_id);
	}		
}
		
void
sub_dht_peer_find::join(const char *grp_id) {
	_active          = true;
	_first_find_done = false;
	
	if (_finding) {
		dht::client *d = owner()->dht_client();
		d->handler_cancel(this);
		this->notify(state::peers_find_stop);	
		_finding = false;
	}
	
	_find();
}

void
sub_dht_peer_find::leave() {
	_active = false;
	if (_finding) {
		dht::client *d = owner()->dht_client();
		d->handler_cancel(this);
		_finding = false;
	}
	
	_set_timer();
}

void
sub_dht_peer_find::notice_receive(int notice_id) {
	switch (notice_id) {
	case state::dht_connected:
		_find(); break;
	case state::active_mode_changed:
		ACE_DEBUG((LM_DEBUG, "sub_dht_peer_find::active_mode_changed\n"));
		_set_timer();
		break;
	}
}

void
sub_dht_peer_find::_find() {
	if (_active &&
	    !_finding && 
	    !this->group_id().empty() &&
	    owner()->dht_client()->in_state() == dht::client::connected)
	{
		ACE_DEBUG((LM_DEBUG, "sub_dht_peer_find: finding group id '%s'\n",
		          this->group_id().c_str()));
		owner()->dht_client()->find(this->group_id(), this);
		_finding         = true;
		_self_in_dht_now = false;
		this->notify(state::peers_find_start);		
	}	
}

void
sub_dht_peer_find::_set_timer() {
	time_value_type interval(time_value_type::zero);
	
	if (_active &&
	    owner()->dht_client()->in_state() == dht::client::connected) 
	{
		// If self detected already, self is in DHT and its reasonable
		// to assume others joining the DHT will find us, no need to
		// search often
		// Do searches often (every 30 secs) if no other nodes than
		// ourself found from DHT yet
		// if (this->peer_self_detected()) {
		switch (owner()->active_mode()) {
		case group::active_lazy:
			// interval = time_value_type(60 * 60);
			interval = time_value_type(60 * 10);
			break;
		default:
			// if (this->peer_confirmed_size() > 1) {
			//	interval = time_value_type(60 * 5);
			//} else {
				interval = time_value_type(30);
			// }
			break;
		}
		// All kinds of funny stuff was theorized and tried until settling
		// for the above simple solution. TODO away.
#if 0		
		if (this->peers_complete()) //     &&
		 //   this->peer_self_detected() && 
		 //   _self_detected_time != time_value_type::zero)
		
		// if (this->peer_confirmed_size() > 0 ||
		//    (this->peer_self_detected() &&
		//     _self_detected_time != time_value_type::zero))
		{
			// If complete peer list has been obtained, no need
			// to search for new peers often.
			// If there are confirmed peers, no need to search for
			// new peers often
			// If self detected already, self is in DHT and its reasonable
			// to assume others joining the DHT will find us soon
			interval = time_value_type(60 * 5);
		// } else if (this->peer_self_detected()) {
		//	_self_detected_time = ACE_OS::gettimeofday();
			// Self is detected, but perhaps just recently so do another
			// search after half a minute to ensure no race conditions
			// interval = time_value_type(60);
		} else {
			// No possible peers detected yet, so search often
			interval = time_value_type(10);
		}
#endif
	}
	
	ACE_DEBUG((LM_DEBUG, "sub_dht_peer_find::set_timer interval %d secs\n",
	          interval.sec()));

	if (_curr_interval != interval) {
		if (_timer_id != -1) {
			owner()->reactor()->cancel_timer(_timer_id);
			_timer_id = -1;
		}
		_curr_interval = interval;
	}
	
	if (_timer_id == -1) {
		// No timer set yet
		if (interval != time_value_type::zero) {
			time_value_type delay;
			time_value_type now(ACE_OS::gettimeofday());
			if (_last_find + interval > now) {
				delay = _last_find + interval - now;
			}
			
			ACE_DEBUG((LM_DEBUG, "sub_dht_peer_find::set_timer schedule " \
			           "timer %d secs, start in %d secs\n",
	          interval.sec(), delay.sec()));
			_timer_id = owner()->reactor()->schedule_timer(this, NULL, 
			                                               delay,
			                                               interval);
			if (_timer_id == -1) {
				throw internal_error("sub_dht_peer_find: " \
				                "failed scheduling timer");
			}
		}
	} else {
		// Timer is set, reschedule it or cancel it
		if (interval != time_value_type::zero) {
			ACE_DEBUG((LM_DEBUG, "sub_dht_peer_find::set_timer rescheduling\n"));
			owner()->reactor()->reset_timer_interval(_timer_id, interval);	
		} else {
			ACE_DEBUG((LM_DEBUG, "sub_dht_peer_find::set_timer canceling\n"));
			owner()->reactor()->cancel_timer(_timer_id);
			_timer_id = -1;			
		}
	}
}

int
sub_dht_peer_find::found(const dht::key &k, const dht::value &v) {
	const dht::name_value_map &meta = v.meta();

	ACE_DEBUG((LM_DEBUG, "sub_dht_peer_find: found key %s\n", k.c_str()));
	if (this->group_id() != k.c_str()) {
		ACE_DEBUG((LM_DEBUG, "sub_dht_peer_find: different group id : %s/%s\n",
		          k.c_str(), this->group_id().c_str()));
		return 0;
	}
	// For now assume DHT implementation provides metadata, so
	// discard results that have not netcomgrp metadata set or
	// where it's 0 (to signal the node has exited the network)
	if (meta.get("netcomgrp", "0") == "0") {
		ACE_DEBUG((LM_DEBUG, "sub_dht_peer_find: " \
		"discarded non-participating result node, meta 'netcomgrp=%s'\n",
		          meta.get("netcomgrp", "<unspecified>").c_str()));
		return 0;
	}

	ACE_DEBUG((LM_DEBUG, "sub_dht_peer_find: meta IP:port '%s:%s'\n", 
	           meta.get("ip",   "<unspecified>").c_str(),
	           meta.get("port", "<unspecified>").c_str()));
	
	if (v.size() < 6) {
		ACE_DEBUG((LM_DEBUG, "sub_dht_peer_find: " \
		"discarded a value of wrong size (%d), should be at least 6 bytes\n",
		v.size()));
		return 0;
	}

	addr_inet_type addr;
	
	if (util::addr_deserialize(&addr, v.data(), v.size()) <= 0) {
		ACE_DEBUG((LM_DEBUG, "sub_dht_peer_find: " \
		"failed to unflatten address\n"));
		return 0;
	}
	
	// Report this peer as a potential participant in this group
	int peer_type =	
		this->peer_possible(addr, 
		                   atoi(meta.get("netcomgrp", "0").c_str()));
	// not used currently
	if (peer_type == internal::peer_self) {
		ACE_DEBUG((LM_DEBUG, "sub_dht_peer_find::_found self found!\n"));
		_self_in_dht_now = true;
		ACE_DEBUG((LM_DEBUG, "sub_dht_peer_find::success self_in_dht_now: %d\n",
		          _self_in_dht_now));
#if 0
		if (!_self_in_dht) {
			this->notify(state::dht_self_detected);
		}
		_self_in_dht_now = true;
		_self_in_dht     = true;
#endif
	}
	return 0;
}

void
sub_dht_peer_find::success() {
	this->notify(state::peers_find_stop);
	// Ensure existence of self now that first find has been done, even if
	// self is not found from the DHT yet.
	this->peer_self_ensure();
	
	_finding   = false;
	_last_find = ACE_OS::gettimeofday();
	
	// for now don't need this
	// this->peer_self_detected(_self_in_dht);
	_set_timer();
	
	if (!_first_find_done) {
		_first_find_done = true;
		this->notify(state::dht_first_find_done);
	}
	ACE_DEBUG((LM_DEBUG, "sub_dht_peer_find::success self_in_dht_now: %d\n",
	          _self_in_dht_now));
	if (!_self_in_dht_now) {
		this->notify(state::dht_self_not_found);
	}
	// If nothing was found, ie. nothing to check that might have the full
	// list of other peers, assume that we have the full node list 
	// (ie. ourself). This is as complete node list that we can obtain
	// at the moment.
	//if (this->peers_size() == 0) {
	//	this->peers_complete(true);
	//}
}

void
sub_dht_peer_find::failure(int, const char *str) {
	ACE_DEBUG((LM_WARNING, "sub_dht_peer_find: error: %s\n", str));
	this->notify(state::peers_find_stop);

	_finding   = false;
	_last_find = ACE_OS::gettimeofday();
	
	_set_timer();	
}

int
sub_dht_peer_find::handle_timeout(const ACE_Time_Value &now, const void *arg) {
	ACE_DEBUG((LM_DEBUG, "sub_dht_peer_find: find timer expired\n"));
	_find();
	return 0;
}

} // ns serverless	
} // ns netcomgrp
