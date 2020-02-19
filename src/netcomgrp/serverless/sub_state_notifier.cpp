#include "../exception.h"
#include "event_observer_notifier.h"
#include "sub_state_notifier.h"
#include "util.h"

using namespace std;

namespace netcomgrp {
namespace serverless {

sub_state_notifier::sub_state_notifier(group *owner) 
	: sub_system(owner, "state_notifier")
{
	_leaving = false;
}

sub_state_notifier::~sub_state_notifier() {
}
		
void
sub_state_notifier::join(const char *grp_id) {
	_leaving           = false;
	_wait_cond_leaving = 0;
	this->observer_notifier()->state_changed(group::joining);
}

void
sub_state_notifier::leave() {
	_leaving = true;
	_wait_cond_leaving = wait_leave_all;
	this->observer_notifier()->state_changed(group::leaving);
}

void
sub_state_notifier::notice_receive(int notice_id) {
	switch (notice_id) {
#ifndef NETCOMGRP_SERVERLESS_SELF_ADDR_AS_IS	
	case state::self_added:
#else
	case state::dht_first_find_done:
#endif
		this->observer_notifier()->state_changed(group::joined);
		break;
	case state::dht_disconnected:
		_wait_cond_leaving &= ~wait_leave_dht; break;
	case state::signal_left:
		_wait_cond_leaving &= ~wait_leave_signal; break;
	case state::peers_find_start:
	{
		ACE_DEBUG((LM_DEBUG, "group_state_notifier::notice_receive: peers_find_start\n"));
		event_observer_notifier *o = dynamic_cast<event_observer_notifier *>(this->observer_notifier());
		ACE_DEBUG((LM_DEBUG, "group_state_notifier::notice_receive: event observer = %d\n", o));
		if (o) o->extra_state_changed(group::peers_find_start);
	}
		break;
	case state::peers_find_stop:
	{
		event_observer_notifier *o = dynamic_cast<event_observer_notifier *>(this->observer_notifier());
		if (o) o->extra_state_changed(group::peers_find_stop);
	}
		break;
	case state::announce_start:
	{
		event_observer_notifier *o = dynamic_cast<event_observer_notifier *>(this->observer_notifier());
		if (o) o->extra_state_changed(group::announce_start);
	}
		break;
	case state::announce_stop:
	{
		event_observer_notifier *o = dynamic_cast<event_observer_notifier *>(this->observer_notifier());
		if (o) o->extra_state_changed(group::announce_stop);
	}
		break;
	}
		
	if (_leaving && !_wait_cond_leaving) {
		_leaving = false;
		this->observer_notifier()->state_changed(group::not_joined);
	}
}

} // ns serverless	
} // ns netcomgrp
