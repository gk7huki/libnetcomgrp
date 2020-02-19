#include "../exception.h"

#include "sub_dht_connection.h"
#include "state.h"

namespace netcomgrp {
namespace serverless {

sub_dht_connection::sub_dht_connection(group *owner) 
	: sub_system(owner, "connection")
{
	_reset_state();
}

sub_dht_connection::~sub_dht_connection() {
	if (_connecting) {
		dht::client *d = owner()->dht_client();
		d->handler_cancel(this);
	}	
}

void 
sub_dht_connection::_reset_state() {
	_did_connect   = false;
	_connecting    = false;
	_disconnecting = false;
	_leaving       = false;
	_deannounced   = false;
}

void
sub_dht_connection::_connection_detected() {
	dht::client *d = owner()->dht_client();
	this->external_addr_detected(d->external_addr());
	this->notify(state::dht_connected);
}

void
sub_dht_connection::join(const char *grp_id) {
	if (_leaving)
		throw call_error(
		                "netcomgrp: join() called while in middle of leaving");
	if (_connecting)
		return;
		
	dht::client *d = owner()->dht_client();
	
	switch (d->in_state()) {
	case dht::client::disconnected:
		ACE_DEBUG((LM_DEBUG, "DHT not connected, connecting\n"));
		d->connect(this);
		_connecting  = true;
		_did_connect = true;
		break;
	case dht::client::connected:
		ACE_DEBUG((LM_DEBUG, "DHT already connected " \
		                     "letting connect process\n"));
		_connection_detected();
		break;
	default:
		ACE_DEBUG((LM_ERROR, "DHT in unsupported state (%d) '%s'!\n",
		                     d->in_state(), d->in_state_str()));
		throw call_errorf("DHT in unsupported state (%d) '%s'",
		                 d->in_state(), d->in_state_str());
	}
}

void
sub_dht_connection::leave() {
	if (_disconnecting)
		return;
	
	_leaving = true;
}

void
sub_dht_connection::notice_receive(int notice_id) {
	if (!_leaving) return;
	
	switch (notice_id) {
	case state::dht_deannounced:
		_deannounced = true; break;
	}
	
	if (notice_id == state::dht_deannounced) {
		dht::client *d = owner()->dht_client();
		if (_did_connect && d->in_state() != dht::client::disconnected) {
			if (_connecting) {
				d->handler_cancel(this);
				_connecting = false;
			}
			d->disconnect(this);
			_disconnecting = true;
		} else {
			// If this module didn't connect the DHT, then we won't
			// disconnect it either. For others to notice the module
			// has done what it is supposed to do, send this message still
			this->notify(state::dht_disconnected);
			_leaving = false;
		}
	}
}

void
sub_dht_connection::success() {
	if (_connecting) {
		_connection_detected();
		_connecting = false;
	} 
	if (_disconnecting) {
		this->notify(state::dht_disconnected);
		_reset_state();
	}
}

void
sub_dht_connection::failure(int error, const char *errstr) {
	if (_connecting) {
		// TODO
		// throw exceptionf(exception_code::connecting, 
		//                 "Could not connect to DHT: %s", errstr);
		_connecting = false;
	}
	
	this->notify(state::dht_disconnected);	
	_leaving = false;
}


} // ns serverless	
} // ns netcomgrp
