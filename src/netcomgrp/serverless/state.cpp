#include "state.h"

namespace netcomgrp {
namespace serverless {

const char *state::_state_strs[] = {
	"DHT connected",
	"DHT disconnected",
	
	"DHT announced",
	"DHT deannounced",
	"DHT self detected",
	"DHT first find done",
	"DHT self not found",
	
	"Active mode change",
	
	"Signalled leave",
	
	"New possible peer",
	
	"Peers find start",
	"Peers find stop",
	"Announce start",
	"Announce stop",
};

} // ns serverless	
} // ns netcomgrp
