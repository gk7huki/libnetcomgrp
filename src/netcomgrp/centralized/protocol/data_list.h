#ifndef _NETCOMGRP_CENTRALIZED_PROTOCOL_DATA_LIST_H_
#define _NETCOMGRP_CENTRALIZED_PROTOCOL_DATA_LIST_H_

#include <ace/Log_Msg.h>

#include <list>

#include "../../common.h"
#include "../../exception.h"
#include "util.h"

#include "common.h"

#include "serializable.h"

namespace netcomgrp {
namespace centralized {
namespace protocol {
	
	template <class T>
	class data_list : public std::list<T>,
	                  public serializable {
	public:
		// Serializable interface
		virtual size_t serialize(reudp::msg_block_type *msg) const;			
		virtual size_t deserialize(reudp::msg_block_type *msg);
	};

	template <class T> size_t 
	data_list<T>::serialize(reudp::msg_block_type *msg) const {
		ACE_DEBUG((LM_DEBUG, "protocol::data_list: serializing list of %d elements\n",
		          this->size()));
		// Serializing this object needs at least 2 bytes for 
		// the amount of elements in the list
		if (msg->space() < 2)
			throw internal_errorf(
				"data_list::serialize " \
				"not enough space (%d) to serialize object, " \
				"needs at least %d bytes",
				msg->space(), this->size()
			);
		if (this->size() > ACE_UINT16_MAX)
			throw internal_errorf(
				"data_list::serialize " \
				"list is too big to serialize, " \
				"size is %d bytes, allowed max %d list elements",
				this->size(), ACE_UINT16_MAX
			);
	
		ACE_UINT16 wsize = this->size();
		size_t ser_n = util::serialize_basic_data(msg, wsize);
		
		typename std::list<T>::const_iterator i = this->begin();
		for (; i != this->end(); i++) {
			ser_n += i->serialize(msg);
		}
		return ser_n;
	}

	template <class T> size_t
	data_list<T>::deserialize(reudp::msg_block_type *msg) {
		if (msg->length() < 2)
			throw internal_error(
				"data_list::deserialize " \
				"not enough data to deserialize object, " \
				"needs at least 2 bytes"
             );
	
		ACE_UINT16 rsize;
		size_t ser_n = util::deserialize_basic_data(msg, &rsize);
	
		ACE_DEBUG((LM_DEBUG, "data_list::deserialize %d elements\n", ser_n));
		this->clear();
		this->resize(rsize);
		
		typename std::list<T>::iterator i = this->begin();
		for (; i != this->end(); i++) {
			ser_n += i->deserialize(msg);
		}
		return ser_n;
	}
	
} // ns protocol
} // ns serverless	
} // ns netcomgrp

#endif //_NETCOMGRP_CENTRALIZED_PROTOCOL_DATA_LIST_H_
