#include "../exception.h"
#include "sub_dht_announce.h"
#include "util.h"

using namespace std;

namespace netcomgrp {
namespace serverless {

sub_dht_announce::sub_dht_announce(group *owner) 
	: sub_system(owner, "announce")
{	
	// Default interval for announcing is 30 mins
	// _announce_interval = time_value_type(60 * 30);
	// Minimum allowed interval for announcements is 30 seconds
	_min_interval = time_value_type(30);
	// During testing 30 secs
	// _announce_interval = time_value_type(30);
	
	_announcing   = false;
	_deannouncing = false;
	_active       = false;
	_timer_id     = -1;
}

sub_dht_announce::~sub_dht_announce() {
	if (_announcing || _deannouncing) {
		dht::client *d = owner()->dht_client();
		d->handler_cancel(this);
	}
	if (_timer_id != -1) {
		owner()->reactor()->cancel_timer(_timer_id);
	}
}
		
void
sub_dht_announce::join(const char *grp_id) {
	if (_group_id == grp_id) {
		ACE_DEBUG((LM_WARNING, "netcomgrp::sub_dht_announce: join group that is " \
		          "equal to the current group '%s", _group_id.c_str()));
		return;
	}
	_group_id      = grp_id;
	// _announce();
}

void
sub_dht_announce::leave() {
	if (_group_id.empty()) {
		ACE_DEBUG((LM_WARNING, "netcomgrp::sub_dht_announce: leave called, but " \
		          "not part of any group"));
		return;
	}
	else if (_announcing) {
		ACE_DEBUG((LM_WARNING, "netcomgrp::sub_dht_announce: was announcing " \
		          "group id '%s', but leave called in middle\n",
		          _group_id.c_str()));
	}
	
	if (owner()->opts() & group::opt_deannounce_with_leave)
		_deannounce();
	else {
		ACE_DEBUG((LM_DEBUG, "netcomgrp::sub_dht_announce::leave skipping " \
		          "deannouncing\n"));
		this->notify(state::dht_deannounced);
	}
	
	_group_id.erase();	
}

void
sub_dht_announce::notice_receive(int notice_id) {
	switch (notice_id) {
#if 0
	case state::peers_complete:
		_active = true;
		_announce(); break;
#endif
	case state::dht_connected:
	{
		_active = true;
		// _last_find = ACE_OS::gettimeofday();
		// _set_timer();
		_set_timer();
		// _announce();
	}
		break;
	case state::dht_disconnected:
		_active = false; break;
	case state::active_mode_changed:
		ACE_DEBUG((LM_DEBUG, "sub_dht_announce::active_mode_changed\n"));
		_set_timer();
		break;
	case state::dht_self_not_found:
		ACE_DEBUG((LM_DEBUG, "sub_dht_announce::self not found, reschedule announce\n"));
		_set_timer(true);
		break;		
	default:
		return;
	}	
}

void
sub_dht_announce::_announce() {
	if (_announcing || _deannouncing) {
		dht::client *d = owner()->dht_client();
		d->handler_cancel(this);
		this->notify(state::announce_stop);		
		_announcing = _deannouncing = false;
	}

	if (_active && !_group_id.empty()) {
		dht::client *d = owner()->dht_client();
		dht::key   dht_key;
		dht::value dht_value;
		dht_value.allow_hash_transform(false);
		
		// Current time of announce as secs is put to netcomgrp,
		// assume long is 32-bit number => may take up to 10 digits
		char timebuf[11];
		ACE_OS::snprintf(timebuf, sizeof(timebuf), "%d", 
		                 ACE_OS::gettimeofday().sec());
		_prepare_store(&dht_key, &dht_value);
		dht_value.meta().set("netcomgrp", timebuf);
		d->store(dht_key, dht_value, this);
		_announcing = true;
		this->notify(state::announce_start);		
	}
}

void
sub_dht_announce::_set_timer(bool self_not_found) {
	// TODO away, testing
	// return; 
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
		if (self_not_found) {
			interval = _min_interval;
		} else {
			switch (owner()->active_mode()) {
			case group::active_lazy:
				interval = time_value_type(60 * 10);
				break;
			default:
				interval = time_value_type(60 * 2);
				break;
			}
		}
	}
	
	ACE_DEBUG((LM_DEBUG, "sub_dht_announce::set_timer interval %d secs\n",
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
			
			ACE_DEBUG((LM_DEBUG, "sub_dht_announce::set_timer schedule " \
			           "timer %d secs, start in %d secs\n",
	          interval.sec(), delay.sec()));
			_timer_id = owner()->reactor()->schedule_timer(this, NULL, 
			                                               delay,
			                                               interval);
			if (_timer_id == -1) {
				throw internal_error("sub_dht_announce: failed scheduling timer");
			}
		}
	} else {
		// Timer is set, reschedule it or cancel it
		if (interval != time_value_type::zero) {
			ACE_DEBUG((LM_DEBUG, "sub_dht_announce::set_timer rescheduling\n"));
			owner()->reactor()->reset_timer_interval(_timer_id, interval);	
		} else {
			ACE_DEBUG((LM_DEBUG, "sub_dht_announce::set_timer canceling\n"));
			owner()->reactor()->cancel_timer(_timer_id);
			_timer_id = -1;			
		}
	}



#if 0
	if (_timer_id == -1) {
		reactor_type *r = owner()->reactor();
		ACE_DEBUG((LM_DEBUG, "sub_dht_announce: scheduling announcement timer\n"));
		_timer_id = r->schedule_timer(this, NULL, 
		                              _announce_interval,
		                              _announce_interval);
		if (_timer_id == -1) {
			throw internal_error("sub_dht_announce: " \
			                "failed scheduling timer");
		}
	}	
#endif

}

void
sub_dht_announce::_deannounce() {
	if (_announcing || _deannouncing) {
		dht::client *d = owner()->dht_client();
		d->handler_cancel(this);
		this->notify(state::announce_stop);		
		_announcing = _deannouncing = false;
	}

	if (_active && !_group_id.empty())
	{
		dht::client *d = owner()->dht_client();
		dht::key   dht_key;
		dht::value dht_value;
		dht_value.allow_hash_transform(false);
		
		_prepare_store(&dht_key, &dht_value);
		dht_value.meta().set("netcomgrp", "0");
		d->store(dht_key, dht_value, this);
		_deannouncing = true;
	} else {
		this->notify(state::dht_deannounced);
	}
	
	// Remove periodic announcing
	if (_timer_id != -1) {
		ACE_DEBUG((LM_DEBUG, "sub_dht_announce: canceling periodic announcing\n"));
		owner()->reactor()->cancel_timer(_timer_id);
	}	
}

void
sub_dht_announce::_prepare_store(dht::key *k, dht::value *v) {
	// 4 bytes for the IPv4, 2 bytes for the port number
	char addrbuf[4 + 2];
	const addr_inet_type &addr = this->addr_announce();
	util::addr_serialize(addrbuf, sizeof(addrbuf), addr);
	
	ACE_DEBUG((LM_DEBUG, "serverless: contact address %s:%d\n",
	          addr.get_host_addr(), addr.get_port_number()));
	          
	k->set(_group_id);
	v->set(addrbuf, 6);
	dht::value dht_value(addrbuf, 6, false); 
	
	char portstr[5];
	ACE_OS::snprintf(portstr, 5, "%u", addr.get_port_number());
	v->meta().set("ip",   addr.get_host_addr());
	v->meta().set("port", portstr);
}

void
sub_dht_announce::success() {
	this->notify(state::announce_stop);
	
	if (_announcing) {
		ACE_DEBUG((LM_WARNING, "sub_dht_announce: announce done\n"));
		_announcing = false;
	} 
	if (_deannouncing) {
		ACE_DEBUG((LM_WARNING, "sub_dht_announce: deannounce done\n"));
		this->notify(state::dht_deannounced);
		_deannouncing = false;
	}
	
	_last_find = ACE_OS::gettimeofday();
	_set_timer();
}

void
sub_dht_announce::failure(int error, const char *errstr) {
	ACE_DEBUG((LM_WARNING, "sub_dht_announce: error storing to dht: %s\n",
	          errstr));
	success();
}

int
sub_dht_announce::handle_timeout(const ACE_Time_Value &now, const void *arg) {
	ACE_DEBUG((LM_DEBUG, "sub_dht_announce: announcement timer expired\n"));
	_announce();
	return 0;
}

} // ns serverless	
} // ns netcomgrp
