#include "sub_system_chain.h"
#include "variable_guard.h"

using namespace std;

namespace netcomgrp {
namespace serverless {
	
sub_system_chain::sub_system_chain(group *g) 
	: sub_system(g, "command_chain"), _notifying(false)
{
}

sub_system_chain::~sub_system_chain() {
	chain_type::iterator i = _systems.begin();
	for (; i != _systems.end(); i++)
		delete *i;
}

void 
sub_system_chain::add(sub_system *s) {
	_systems.push_back(s);
}

void
sub_system_chain::notice_send(int notice_id) {
	// Since notice_receive might trigger sending another notice,
	// make sure the notices arrive at the correct order - ie
	// if already notifying add to the end of notify queue for
	// dispatching
	ACE_DEBUG((LM_DEBUG, "notice_send: notifying value %d\n", _notifying));
	if (_notifying) {
		_notify_queue.push(notice_id);
		return;
	}
	variable_guard<bool> guard(&_notifying, true);

	notice_receive(notice_id);
	_process_notify_queue();
}

void 
sub_system_chain::_process_notify_queue() {
	while (!_notify_queue.empty()) {
		int notice_id = _notify_queue.front(); _notify_queue.pop();
		notice_receive(notice_id);
	}	
}

void
sub_system_chain::notice_receive(int notice_id) {
	chain_type::iterator i = _systems.begin();
	for (; i != _systems.end(); i++) {
		ACE_DEBUG((LM_DEBUG, "sub_system_chain: notice_receive (%s) to %s\n", 
		                     state::to_str(notice_id),
		                     (*i)->id()));
		(*i)->notice_receive(notice_id);
	}
}

void
sub_system_chain::join(const char *grp_id) {
	ACE_DEBUG((LM_DEBUG, "sub_system_chain: joining, sub systems %d\n", 
	          _systems.size()));

	variable_guard<bool> guard(&_notifying, true);
		
	chain_type::iterator i = _systems.begin();
	for (; i != _systems.end(); i++) {
		ACE_DEBUG((LM_DEBUG, "sub_system_chain: join(%s) to %s\n", 
		           grp_id, (*i)->id()));
		(*i)->join(grp_id);
	}

	_process_notify_queue();
}

void
sub_system_chain::leave() {
	variable_guard<bool> guard(&_notifying, true);
	
	chain_type::iterator i = _systems.begin();
	for (; i != _systems.end(); i++) {
		ACE_DEBUG((LM_DEBUG, "sub_system_chain: leave to %s\n", (*i)->id()));		
		(*i)->leave();
	}
	_process_notify_queue();
}

void
sub_system_chain::peer_confirmed_new(node *n) {
	variable_guard<bool> guard(&_notifying, true);

	chain_type::iterator i = _systems.begin();
	for (; i != _systems.end(); i++) {
		ACE_DEBUG((LM_DEBUG, "sub_system_chain: peer_confirmed_new to %s\n", (*i)->id()));
		(*i)->peer_confirmed_new(n);
	}

	_process_notify_queue();
}

} // ns serverless	
} // ns netcomgrp
