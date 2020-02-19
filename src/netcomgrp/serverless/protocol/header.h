#ifndef _NETCOMGRP_SERVERLESS_PROTOCOL_HEADER_H_
#define _NETCOMGRP_SERVERLESS_PROTOCOL_HEADER_H_

#include <ace/Basic_Types.h>

#include "common.h"
#include "address_tuple.h"
#include "data_string.h"
#include "util.h"
#include "serializable.h"

namespace netcomgrp {
namespace serverless {
namespace protocol {
	class header : public serializable {
		ACE_UINT16    _magic_id;
		ACE_UINT32    _version;
		ACE_UINT8     _type;
		ACE_UINT32    _time;
		// address_tuple _addr;
		// address_tuple _addr_recver;
		data_string   _group;
		
	public:
		inline header();

		inline ACE_UINT8 type() const;
		inline void type(ACE_UINT8 t);
		
		inline ACE_UINT32 version() const;
		inline ACE_UINT32 time() const;
		inline ACE_UINT16 magic_id() const;
		
		/*
		 * inline const address_tuple &node_sender() const;
		 * inline void                 node_sender(const address_tuple &a);

		 * inline const address_tuple &node_recver() const;
		 * inline void                 node_recver(const address_tuple &a);
		 */
		inline const std::string &group() const { return _group; }
		inline void               group(const std::string &g) { _group = g; }

		inline bool compatible_with(const header &o) const;

		// Serializable interface
		virtual size_t serialize(reudp::msg_block_type *msg) const;
		virtual size_t deserialize(reudp::msg_block_type *msg);		
	};

	inline header::header() 
	    // magic_id is something to recognize netcomgrp signal packets ('ng')
	  : _magic_id(0x6E67),	
	  	// 0.1
	    _version(0 << 8 | 1 << 0),
	    _type(msg_unknown),
	    _time(0)
	{
		// Making network byte order versions of these
		_magic_id = htons(_magic_id);
		_version  = htonl(_version);
	}

	inline ACE_UINT8  header::type() const   { return _type; }
	inline void header::type(ACE_UINT8 t)    { _type = t; }
	
	inline ACE_UINT32 header::version()  const { return ntohl(_version); }
	inline ACE_UINT32 header::time()     const { return ntohl(_time); }
	inline ACE_UINT16 header::magic_id() const { return ntohs(_magic_id); }

/*
	inline const address_tuple &header::node_sender() const { return _addr_sender; }
	inline void  header::node_sender(const address_tuple &a) { _addr_sender = a; }

	inline const address_tuple &header::node_recver() const { return _addr_recver; }
	inline void  header::node_recver(const address_tuple &a) { _addr_recver = a; }
*/
	inline bool header::compatible_with(const header &o) const {
		if ((_version >> 8) == (o._version >> 8) &&
			_group    == o._group &&
			_magic_id == o._magic_id)
			return true;
		return false;
	}	
} // ns protocol
} // ns serverless	
} // ns netcomgrp

#endif //_NETCOMGRP_SERVERLESS_PROTOCOL_HEADER_H_
