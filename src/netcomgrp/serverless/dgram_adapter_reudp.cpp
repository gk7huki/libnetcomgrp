#include "dgram_adapter_reudp.h"

namespace netcomgrp {
namespace serverless {

dgram_adapter_reudp::dgram_adapter_reudp()
  : 
	// Receive enough space for UDP
    _buf(new char[_buf_len]),
	_pdone_cb(NULL)
{	
}

dgram_adapter_reudp::~dgram_adapter_reudp() {
	delete [] _buf;
}

void 
dgram_adapter_reudp::packet_done_cb(packet_done_cb_type cb, void *param) {
	// Fuck it, for now like this since the packet done types are the
	// same as in reudp. Should later do a separate callback here to 
	// this class to translate the types correctly. TODO
	_dgram.packet_done_cb(cb, param);
}

bool
dgram_adapter_reudp::needs_to_send() {
	return _dgram.needs_to_send();
}

time_value_type
dgram_adapter_reudp::needs_to_send_when() {
	return _dgram.needs_to_send_when();
}

void 
dgram_adapter_reudp::open(u_short port) {
	reudp::addr_inet_type addr(port);
	_dgram.open(addr);
}

void
dgram_adapter_reudp::close() {
	_dgram.close();
}

size_t
dgram_adapter_reudp::send(const void *buf, size_t len, 
                    const addr_inet_type &recp)
{
	return _dgram.send(buf, len, recp);
}

size_t
dgram_adapter_reudp::send(const void *buf, size_t len, 
                    std::list<addr_inet_type>::const_iterator istart,
                    std::list<addr_inet_type>::const_iterator iend)
{
	return _dgram.send_multi(buf, len, istart, iend);
}

size_t
dgram_adapter_reudp::recv(char **buf, addr_inet_type *from) {
	*buf = _buf;
	//return _dgram.recv(_buf, _buf_len, *from);
	// To ease debugging, ensure NULL termination
	size_t n = _dgram.recv(_buf, _buf_len, *from);
	if (n < _buf_len) _buf[n] = 0;
	return n;
}

} // ns serverless	
} // ns netcomgrp
