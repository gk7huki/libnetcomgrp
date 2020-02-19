#ifndef _NETCOMGRP_CENTRALIZED_PROTOCOL_PDU_H_
#define _NETCOMGRP_CENTRALIZED_PROTOCOL_PDU_H_

#include "header.h"
#include "data_list.h"
#include "util.h"

namespace netcomgrp {
namespace centralized {
namespace protocol {
namespace pdu {

inline size_t
serialize_header(reudp::msg_block_type *msg,
                 header &hdr)
{
	return hdr.serialize(msg);
}
inline size_t
deserialize_header(reudp::msg_block_type *msg,
                   header *hdr)
{
	return hdr->deserialize(msg);
}

inline size_t
serialize_subject(reudp::msg_block_type *msg,
                  const address_tuple &addr)
{
	return addr.serialize(msg);
}

inline size_t
deserialize_subject(reudp::msg_block_type *msg,
                    address_tuple *addr)
{
	return addr->deserialize(msg);
}

inline size_t
serialize_nodes_list(reudp::msg_block_type *msg,
                     const data_list<address_tuple> &nlist)
                     // bool  is_complete)
{
	size_t n = 0;
	// ACE_UINT8 complete = (is_complete ? 1 : 0);
	
	n += util::serialize(msg, nlist);
	// n += util::serialize_basic_data(msg, complete);
	
	return n;		
}
inline size_t
deserialize_nodes_list(reudp::msg_block_type *msg,
                       data_list<address_tuple> *nlist)
                       // bool *is_complete)
{
	size_t n = 0;
	// ACE_UINT8 complete;
	
	n += util::deserialize(msg, nlist);
	// n += util::deserialize_basic_data(msg, &complete);
	
	// *is_complete = (complete ? true : false);
	
	return n;
		
}

inline size_t
serialize_data(reudp::msg_block_type *msg,
               const char *dataptr, size_t datalen)
                     // bool  is_complete)
{
	if (datalen > 65536) 
		throw call_errorf(
			"Can not serialize data of length %u since it is " \
			"bigger than 65536 bytes", datalen
		);
	size_t n = 0;
	ACE_UINT16 len = datalen;
	
	n += util::serialize_basic_data(msg, len);
	n += len;
	msg->copy(dataptr, datalen);

	return n;		
}

inline size_t
deserialize_data(reudp::msg_block_type *msg,
                 const char **dataptr, size_t *datalen)
                       // bool *is_complete)
{
	size_t n = 0;

	ACE_UINT16 len;
	
	n += util::deserialize_basic_data(msg, &len);
	n += len;
	*dataptr = msg->rd_ptr();
	*datalen = len;
	if (msg->rd_ptr() + len > msg->wr_ptr())
		throw protocol_errorf(
			"Tried deserializing data block of %d bytes " \
			"but not enough data read for that", len
		);
		                 
	msg->rd_ptr(len);

	return n;		
}

inline size_t
serialize_send(reudp::msg_block_type *msg,
               const char *dataptr, size_t datalen,
               const data_list<address_tuple> &nlist)
{
	size_t n = 0;
	
	n += serialize_data(msg, dataptr, datalen);
	n += serialize_nodes_list(msg, nlist);
	
	return n;
}

inline size_t
deserialize_send(reudp::msg_block_type *msg,
                 const char **dataptr, size_t *datalen,
                 data_list<address_tuple> *nlist)
{
	size_t n = 0;
	
	n += deserialize_data(msg, dataptr, datalen);
	n += deserialize_nodes_list(msg, nlist);
	
	return n;
}


} // ns util
} // ns protocol
} // ns serverless	
} // ns netcomgrp


#endif //_NETCOMGRP_CENTRALIZED_PROTOCOL_UTIL_H_
