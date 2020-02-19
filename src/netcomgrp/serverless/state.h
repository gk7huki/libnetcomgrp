#ifndef _NETCOMGRP_SERVERLESS_STATE_H_
#define _NETCOMGRP_SERVERLESS_STATE_H_

namespace netcomgrp {
namespace serverless {

	class state {
		static const char *_state_strs[];
	public:
		enum {
			dht_connected = 0,
			dht_disconnected,
			
			dht_announced,
			dht_deannounced,
			dht_self_detected,
			dht_first_find_done,
			dht_self_not_found,
			
			active_mode_changed,
			
			signal_left,
			
			peer_new_possible,
			
			peers_find_start,
			peers_find_stop,
			announce_start,
			announce_stop,

			self_added,			
		};
		inline static const char *to_str(int n) {
			return _state_strs[n];
		}
	};
} // ns serverless	
} // ns netcomgrp

#endif //_NETCOMGRP_SERVERLESS_STATE_H_
