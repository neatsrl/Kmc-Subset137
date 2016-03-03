/**************************************************************************//**
 *
 * ...
 *
 * This file ...
 *
 * @file: ss137/ss137_lib/src/ss137_lib.c
 * $Author: $
 * $Revision: $
 * $Date: $
 *
 * History:
 *
 * Version     Date      Author         Change Description
 *
 *- $Id: $
 *
 ******************************************************************************/

/*****************************************************************************
* INCLUDES
******************************************************************************/

#include <stdio.h>     /* for fopen, snprintf, etc... */
#include <string.h>    /* for memmove, memcmp, memset */
#include <arpa/inet.h> /* for htons, etc.. */
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "common.h"
#include "net_utils.h"
#include "tls_wrapper.h"
#include "msg_definitions.h"
#include "ss137_lib.h"

/*****************************************************************************
* DEFINES
******************************************************************************/

/*****************************************************************************
 * TYPEDEFS
 *****************************************************************************/

/*****************************************************************************
 * VARIABLES
 *****************************************************************************/

/*****************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************************************************/



static int32_t buildMsgHeader(write_stream_t* const ostream,
							  const uint32_t msg_length,
							  const uint32_t msg_type,
							  const session_t* const curr_session);


static int32_t convertMsgHeaderToHost(msg_header_t* const header,
									  read_stream_t* const istream);

static int32_t convertCmdAddKeysToHost(cmd_add_keys_t* const payload,
									   read_stream_t* const istream);

static int32_t convertCmdDeleteKeysToHost(cmd_del_keys_t* const payload,
										  read_stream_t* const istream);
	
static int32_t convertCmdUpKeyValiditiesToHost(cmd_up_key_val_t* const payload,
											   read_stream_t* const istream);

static int32_t convertCmdUpKeyEntitiesToHost(cmd_up_key_ent_t* const payload,
											 read_stream_t* const istream);

static int32_t convertCmdReqKeyOperationToHost(cmd_req_key_op_t* const payload,
											   read_stream_t* const istream);

static int32_t convertNotifKeyUpdateStatusToHost(notif_key_up_status_t* const payload,
												 read_stream_t* const istream);

static int32_t convertNotifSessionInitToHost(notif_session_init_t* const payload,
											 read_stream_t* const istream);

static int32_t convertNotifResponseToHost(notif_response_t* const payload,
										  read_stream_t* const istream);

static int32_t convertNotifKeyOpReqRcvdToHost(notif_key_op_req_rcvd_t* const payload,
											  read_stream_t* const istream);

static int32_t convertNotifKeyDBChecksumToHost(notif_key_db_checksum_t* const payload,
											   read_stream_t* const istream);
	
static int32_t convertMsgHeaderToHost(msg_header_t* const header,
									  read_stream_t* const istream);

static int32_t checkMsgHeader(const msg_header_t* const header,
							  const uint32_t exp_msg_length,
							  const uint32_t exp_sender,
							  const uint32_t exp_seq_num,
							  const uint32_t exp_trans_num);

static int32_t sendMsg(write_stream_t* const ostream,
					   const uint32_t tls_des);

/*****************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 *****************************************************************************/

static uint32_t getMyEtcsIdExp(void)
{
	/* TBD decide how to get my id */
	uint32_t myId = 0x11223344;
	return(myId);
}

static uint8_t getInterfaceVersion(void)
{
	/* TBD decide how to get interface version */
	return(INTERFACE_VERSION);
}


static int32_t buildMsgHeader(write_stream_t* const ostream,
							  const uint32_t msg_length,
							  const uint32_t msg_type,
							  const session_t* const curr_session)
{

	msg_header_t header;
	
	ASSERT(ostream != NULL, E_NULL_POINTER);

	memset(&header, 0U, sizeof(msg_header_t));
	
	header.msgLength = msg_length;
	header.version   = getInterfaceVersion();
	header.recIDExp  = curr_session->peerEtcsIDExp;
	header.sendIDExp = getMyEtcsIdExp();
	header.transNum  = curr_session->myTransNum;
	header.seqNum    = curr_session->mySeqNum;
	header.msgType   = msg_type;

	hostToNet32(ostream, header.msgLength);
	hostToNet8(ostream, &header.version, sizeof(uint8_t));
	hostToNet32(ostream, header.recIDExp);
	hostToNet32(ostream, header.sendIDExp);
	hostToNet32(ostream, header.transNum);
	hostToNet16(ostream, header.seqNum);
	hostToNet8(ostream, &header.msgType, sizeof(uint8_t));
	
	return(RETURN_SUCCESS);
}

int32_t sendCmdAddKeys(const cmd_add_keys_t* const payload,
					   const session_t* const curr_session)
{
	uint32_t i = 0U;
	uint32_t j = 0U;
	uint32_t k = 0U;
	uint32_t msg_length = 0U;
	write_stream_t ostream;

	ASSERT((curr_session != NULL) && (payload != NULL), E_NULL_POINTER);
	ASSERT(payload->reqNum < MAX_REQ_ADD_KEYS,  E_INVALID_PARAM);

	/* prepare output buffer */
	initWriteStream(&ostream);
	
	/* prepare message header */
	msg_length = CMD_ADD_KEYS_MIN_SIZE + (payload->reqNum*K_STRUCT_MIN_SIZE);

	for(k = 0U; k < payload->reqNum; k++)
	{
		msg_length += payload->kStructList[k].peerNum*sizeof(uint32_t);
	}
	
	buildMsgHeader(&ostream, msg_length, CMD_ADD_KEYS, curr_session);

	/* serialize payload */
	hostToNet16(&ostream, payload->reqNum);

	for(i = 0U; i < payload->reqNum; i++)
	{
		hostToNet8(&ostream, &payload->kStructList[i].length, sizeof(uint8_t));
		hostToNet32(&ostream, payload->kStructList[i].kIdent.genID);
		hostToNet32(&ostream, payload->kStructList[i].kIdent.serNum);
		hostToNet32(&ostream, payload->kStructList[i].etcsID);
		hostToNet8(&ostream, payload->kStructList[i].kMAC, (uint32_t)KMAC_SIZE);
		hostToNet16(&ostream, payload->kStructList[i].peerNum);

		ASSERT(payload->kStructList[i].peerNum < MAX_PEER_NUM,  E_INVALID_PARAM);

		for (j = 0U; j < payload->kStructList[i].peerNum; j++)
		{
			hostToNet32(&ostream, payload->kStructList[i].peerID[j]);
		}
		
		hostToNet32(&ostream, payload->kStructList[i].startValidity);
		hostToNet32(&ostream, payload->kStructList[i].endValidity);
	}

	sendMsg(&ostream, curr_session->tls_des);
	
	return(RETURN_SUCCESS);
}


int32_t sendCmdDeleteKeys(const cmd_del_keys_t* const payload,
						  const session_t* const curr_session)
{
	uint32_t msg_length = 0U;
	uint32_t i = 0U;
	write_stream_t ostream;

	ASSERT((curr_session != NULL) && (payload != NULL), E_NULL_POINTER);
	ASSERT(payload->reqNum < MAX_REQ_DEL_KEYS,  E_INVALID_PARAM);

	/* prepare output buffer */
	initWriteStream(&ostream);
	
	/* prepare message header */
	msg_length = CMD_DEL_KEYS_MIN_SIZE + (K_IDENT_SIZE * payload->reqNum);
	
	buildMsgHeader(&ostream, msg_length, CMD_DELETE_KEYS, curr_session);

	/* serialize payload */
	hostToNet16(&ostream, payload->reqNum);
	
	for(i = 0U; i < payload->reqNum; i++)
	{
		hostToNet32(&ostream, payload->kIdentList[i].genID);
		hostToNet32(&ostream, payload->kIdentList[i].serNum);
	}

	sendMsg(&ostream, curr_session->tls_des);
		
	return(RETURN_SUCCESS);
}

int32_t sendCmdDeleteAllKeys(const session_t* const curr_session)
{
	uint32_t msg_length = 0U;
	write_stream_t ostream;

	ASSERT(curr_session != NULL, E_NULL_POINTER);

	/* prepare output buffer */
	initWriteStream(&ostream);
	
	/* prepare message header */
	msg_length = CMD_DEL_ALL_KEYS_SIZE;
	
	buildMsgHeader(&ostream, msg_length, CMD_DELETE_ALL_KEYS, curr_session);

	/* this command does not have payload,
	   it consists only of the message header */
	
	sendMsg(&ostream, curr_session->tls_des);
		
	return(RETURN_SUCCESS);
}

int32_t sendCmdUpKeyValidities(const cmd_up_key_val_t* const payload,
							   const session_t* const curr_session)
{
	uint32_t msg_length = 0U;
	uint32_t i = 0U;
	write_stream_t ostream;

	ASSERT((curr_session != NULL) && (payload != NULL), E_NULL_POINTER);
	ASSERT(payload->reqNum < MAX_REQ_UPDATE,  E_INVALID_PARAM);

	/* prepare output buffer */
	initWriteStream(&ostream);
	
	/* prepare message header */
	msg_length = CMD_UP_KEY_VAL_MIN_SIZE + (K_VALIDITY_SIZE*payload->reqNum);
	
	buildMsgHeader(&ostream, msg_length, CMD_UPDATE_KEY_VALIDITIES, curr_session);

	/* serialize payload */
	hostToNet16(&ostream, payload->reqNum);

	for(i = 0U; i < payload->reqNum; i++)
	{
		hostToNet32(&ostream, payload->kValidityList[i].kIdent.genID);
		hostToNet32(&ostream, payload->kValidityList[i].kIdent.serNum);
		hostToNet32(&ostream, payload->kValidityList[i].startValidity);
		hostToNet32(&ostream, payload->kValidityList[i].endValidity);
	}

	sendMsg(&ostream, curr_session->tls_des);

	return(RETURN_SUCCESS);
}

int32_t sendCmdUpKeyEntities(const cmd_up_key_ent_t* const payload,
							 const session_t* const curr_session)
{
	uint32_t i = 0U;
	uint32_t j = 0U;
	uint32_t k = 0U;
	uint32_t msg_length = 0U;
	write_stream_t ostream;

	ASSERT((curr_session != NULL) && (payload != NULL), E_NULL_POINTER);
	ASSERT(payload->reqNum < MAX_REQ_UPDATE,  E_INVALID_PARAM);
	
	/* prepare output buffer */
	initWriteStream(&ostream);
	
	/* prepare message header */
	msg_length = CMD_UP_KEY_ENT_MIN_SIZE + (payload->kEntityList[i].peerNum * K_ENTITY_MIN_SIZE);
	for(k = 0U; k < payload->reqNum; k++)
	{
		msg_length += payload->kEntityList[i].peerNum*sizeof(uint32_t);
	}
	
	buildMsgHeader(&ostream, msg_length, CMD_UPDATE_KEY_ENTITIES, curr_session);

	/* serialize payload */
	hostToNet16(&ostream, payload->reqNum);

	for(i = 0U; i < payload->reqNum; i++)
	{
		hostToNet32(&ostream, payload->kEntityList[i].kIdent.genID);
		hostToNet32(&ostream, payload->kEntityList[i].kIdent.serNum);
		hostToNet16(&ostream, payload->kEntityList[i].peerNum);

		ASSERT(payload->kEntityList[i].peerNum < MAX_PEER_NUM,  E_INVALID_PARAM);

		for (j = 0U; j < payload->kEntityList[i].peerNum; j++)
		{
			hostToNet32(&ostream, payload->kEntityList[i].peerID[j]);
		}
	}

	sendMsg(&ostream, curr_session->tls_des);
	
	return(RETURN_SUCCESS);
}


int32_t sendCmdReqKeyOperation(const cmd_req_key_op_t* const payload,
							   const session_t* const curr_session)
{
	uint32_t msg_length = 0U;
	write_stream_t ostream;
	
	ASSERT((curr_session != NULL) && (payload != NULL), E_NULL_POINTER);
	ASSERT(strlen(payload->text) < MAX_TEXT_LENGTH, E_NULL_POINTER);

		/* prepare output buffer */
	initWriteStream(&ostream);
	
	/* prepare message header */
	msg_length = CMD_REQUEST_KEY_OP_MIN_SIZE+strlen(payload->text);
	
	buildMsgHeader(&ostream, msg_length, CMD_REQUEST_KEY_OPERATION, curr_session);

	/* serialize payload */

	hostToNet32(&ostream,  payload->etcsID);
	hostToNet8(&ostream,  &payload->reason,  sizeof(uint8_t));
	hostToNet32(&ostream,  payload->startValidity);
	hostToNet32(&ostream,  payload->endValidity);
	hostToNet32(&ostream,  payload->textLength);
	hostToNet8(&ostream, (uint8_t*)payload->text, payload->textLength);

	sendMsg(&ostream, curr_session->tls_des);
	
	return(RETURN_SUCCESS);
}

int32_t sendCmdReqKeyDBChecksum(const session_t* const curr_session)
{
	uint32_t msg_length = 0U;
	write_stream_t ostream;
	
	ASSERT(curr_session != NULL, E_NULL_POINTER);

	/* prepare output buffer */
	initWriteStream(&ostream);
	
	/* prepare message header */
	msg_length = CMD_REQUEST_KEY_DB_CK_SIZE;
	
	buildMsgHeader(&ostream, msg_length, CMD_REQUEST_KEY_DB_CHECKSUM, curr_session);

	sendMsg(&ostream, curr_session->tls_des);
	
	return(RETURN_SUCCESS);
}

int32_t sendNotifKeyUpdateStatus(const notif_key_up_status_t* const payload,
								 const session_t* const curr_session)
{
	uint32_t msg_length = 0U;
	write_stream_t ostream;

	ASSERT((curr_session != NULL) && (payload != NULL), E_NULL_POINTER);

	/* prepare output buffer */
	initWriteStream(&ostream);
	
	/* prepare message header */
	msg_length = NOTIF_KEY_UP_STATUS_SIZE;
	
	buildMsgHeader(&ostream, msg_length, NOTIF_KEY_UPDATE_STATUS, curr_session);

	/* serialize payload */
	hostToNet32(&ostream, payload->kIdent.genID);
	hostToNet32(&ostream, payload->kIdent.serNum);
	hostToNet8(&ostream, &payload->kStatus, sizeof(uint8_t));

	sendMsg(&ostream, curr_session->tls_des);

	return(RETURN_SUCCESS);
}

int32_t sendNotifAckKeyUpStatus(const session_t* const curr_session)
{
	uint32_t msg_length = 0U;
	write_stream_t ostream;
	
	ASSERT(curr_session != NULL, E_NULL_POINTER);

	/* prepare output buffer */
	initWriteStream(&ostream);
	
	/* prepare message header */
	msg_length = NOTIF_ACK_KEY_UP_STATUS_SIZE;
	
	buildMsgHeader(&ostream, msg_length, NOTIF_ACK_KEY_UPDATE_STATUS, curr_session);

	sendMsg(&ostream, curr_session->tls_des);
	
	return(RETURN_SUCCESS);
}


int32_t sendNotifSessionInit(const notif_session_init_t* const payload,
							 const session_t* const curr_session)
{
	uint32_t msg_length = 0U;
	write_stream_t ostream;
	
	ASSERT((curr_session != NULL) && (payload != NULL), E_NULL_POINTER);

	/* prepare output buffer */
	initWriteStream(&ostream);
	
	/* prepare message header */
	msg_length = NOTIF_SESSION_INIT_SIZE;
	
	buildMsgHeader(&ostream, msg_length, NOTIF_SESSION_INIT, curr_session);

	/* serialize payload */
	hostToNet8(&ostream, &payload->nVersion, sizeof(uint8_t));
	hostToNet8(&ostream, &payload->version, sizeof(uint8_t));
	hostToNet8(&ostream, &payload->appTimeout, sizeof(uint8_t));

	sendMsg(&ostream, curr_session->tls_des);
	
	return(RETURN_SUCCESS);
}

int32_t sendNotifEndUpdate(const session_t* const curr_session)
{
	uint32_t msg_length = 0U;
	write_stream_t ostream;
	
	ASSERT(curr_session != NULL, E_NULL_POINTER);

	/* prepare output buffer */
	initWriteStream(&ostream);
	
	/* prepare message header */
	msg_length = NOTIF_END_UPDATE_SIZE;
	
	buildMsgHeader(&ostream, msg_length, NOTIF_END_OF_UPDATE, curr_session);

	sendMsg(&ostream, curr_session->tls_des);
	
	return(RETURN_SUCCESS);
}


int32_t sendNotifResponse(const notif_response_t* const payload,
						  const session_t* const curr_session)
{
	uint32_t msg_length = 0U;
	write_stream_t ostream;
	
	ASSERT((curr_session != NULL) && (payload != NULL), E_NULL_POINTER);
	ASSERT(payload->reqNum < MAX_REQ_NOTIF,  E_INVALID_PARAM);

	/* prepare output buffer */
	initWriteStream(&ostream);
	
	/* prepare message header */
	msg_length = NOTIF_RESPONSE_MIN_SIZE+sizeof(uint8_t)*payload->reqNum;
	
	buildMsgHeader(&ostream, msg_length, NOTIF_RESPONSE, curr_session);

	/* serialize payload */
	hostToNet8(&ostream, &payload->response, sizeof(uint8_t));
	hostToNet16(&ostream, payload->reqNum);
	hostToNet8(&ostream, payload->notificationList, sizeof(uint8_t)*payload->reqNum);

	sendMsg(&ostream, curr_session->tls_des);

	return(RETURN_SUCCESS);
}


int32_t sendNotifKeyOpReqRcvd(const notif_key_op_req_rcvd_t* const payload,
							   const session_t* const curr_session)
{
	uint32_t msg_length = 0U;
	write_stream_t ostream;

	ASSERT((curr_session != NULL) && (payload != NULL), E_NULL_POINTER);

	/* prepare output buffer */
	initWriteStream(&ostream);
	
	/* prepare message header */
	msg_length = NOTIF_KEY_OP_REQ_RCVD_SIZE;
	
	buildMsgHeader(&ostream, msg_length, NOTIF_KEY_OPERATION_REQ_RCVD, curr_session);

	/* serialize payload */
	hostToNet16(&ostream, payload->maxTime);

	sendMsg(&ostream, curr_session->tls_des);

	return(RETURN_SUCCESS);
}

int32_t sendNotifKeyDBChecksum(const notif_key_db_checksum_t* const payload,
							   const session_t* const curr_session)
{
	uint32_t msg_length = 0U;
	write_stream_t ostream;
	
	ASSERT((curr_session != NULL) && (payload != NULL), E_NULL_POINTER);

	/* prepare output buffer */
	initWriteStream(&ostream);
	
	/* prepare message header */
	msg_length = NOTIF_KEY_DB_CHECKSUM_SIZE;
	
	buildMsgHeader(&ostream, msg_length, NOTIF_KEY_DB_CHECKSUM, curr_session);

	/* serialize payload */
	hostToNet8(&ostream, payload->checksum, (uint32_t)CHECKSUM_SIZE);

	sendMsg(&ostream, curr_session->tls_des);
	
	return(RETURN_SUCCESS);
}


static int32_t convertMsgHeaderToHost(msg_header_t* const header,
									  read_stream_t* const istream)
{
	ASSERT((istream != NULL) && (header != NULL), E_NULL_POINTER);

	netToHost32(&header->msgLength, istream);
	netToHost8(&header->version, (uint32_t)sizeof(uint8_t), istream);
	netToHost32(&header->recIDExp, istream);
	netToHost32(&header->sendIDExp, istream);
	netToHost32(&header->transNum, istream);
	netToHost16(&header->seqNum, istream);
	netToHost8(&header->msgType, (uint32_t)sizeof(uint8_t), istream);

    return(RETURN_SUCCESS);
}


static int32_t convertCmdAddKeysToHost(cmd_add_keys_t* const payload,
									   read_stream_t* const istream)
{
	uint32_t i = 0U;
	uint32_t j = 0U;
	
	ASSERT((istream != NULL) && (payload != NULL), E_NULL_POINTER);

	netToHost16(&payload->reqNum, istream);

	for(i = 0U; i < payload->reqNum; i++)
	{
		netToHost8(&payload->kStructList[i].length, sizeof(uint8_t), istream);
		netToHost32(&payload->kStructList[i].kIdent.genID, istream);
		netToHost32(&payload->kStructList[i].kIdent.serNum, istream);
		netToHost32(&payload->kStructList[i].etcsID, istream);
		netToHost8(payload->kStructList[i].kMAC, (uint32_t)KMAC_SIZE, istream);
		netToHost16(&payload->kStructList[i].peerNum, istream);

		for (j = 0U; j < payload->kStructList[i].peerNum; j++)
		{
			netToHost32(&payload->kStructList[i].peerID[j], istream);
		}
		netToHost32(&payload->kStructList[i].startValidity, istream);
		netToHost32(&payload->kStructList[i].endValidity, istream);
	}

	return(RETURN_SUCCESS);
	
}

static int32_t convertCmdDeleteKeysToHost(cmd_del_keys_t* const payload,
										  read_stream_t* const istream)
{
	uint32_t i = 0U;

	ASSERT((istream != NULL) && (payload != NULL), E_NULL_POINTER);

	netToHost16(&payload->reqNum, istream);

	for(i = 0U; i < payload->reqNum; i++)
	{
		netToHost32(&payload->kIdentList[i].genID, istream);
		netToHost32(&payload->kIdentList[i].genID, istream);
	}
	
	return(RETURN_SUCCESS);
}

static int32_t convertCmdUpKeyValiditiesToHost(cmd_up_key_val_t* const payload,
											   read_stream_t* const istream)
{
	uint32_t i = 0U;
	
	ASSERT((istream != NULL) && (payload != NULL), E_NULL_POINTER);

	netToHost16(&payload->reqNum, istream);

	for(i = 0U; i < payload->reqNum; i++)
	{
		netToHost32(&payload->kValidityList[i].kIdent.genID, istream);
		netToHost32(&payload->kValidityList[i].kIdent.serNum, istream);
		netToHost32(&payload->kValidityList[i].startValidity, istream);
		netToHost32(&payload->kValidityList[i].endValidity, istream);
	}
		
	return(RETURN_SUCCESS);
}


static int32_t convertCmdUpKeyEntitiesToHost(cmd_up_key_ent_t* const payload,
											 read_stream_t* const istream)
{
	uint32_t i = 0U;
	uint32_t j = 0U;

	ASSERT((istream != NULL) && (payload != NULL), E_NULL_POINTER);

	netToHost16(&payload->reqNum, istream);

	for(i = 0U; i < payload->reqNum; i++)
	{
		netToHost32(&payload->kEntityList[i].kIdent.genID, istream);
		netToHost32(&payload->kEntityList[i].kIdent.serNum, istream);
		netToHost16(&payload->kEntityList[i].peerNum, istream);

		for (j = 0U; j < payload->kEntityList[i].peerNum; j++)
		{
			netToHost32(&payload->kEntityList[i].peerID[j], istream);
		}
	}
	return(RETURN_SUCCESS);
}

static int32_t convertCmdReqKeyOperationToHost(cmd_req_key_op_t* const payload,
											   read_stream_t* const istream)
{
	ASSERT((istream != NULL) && (payload != NULL), E_NULL_POINTER);

	netToHost32(&payload->etcsID, istream);
	netToHost8(&payload->reason, sizeof(uint8_t), istream);
	netToHost32(&payload->startValidity, istream);
	netToHost32(&payload->endValidity, istream);
	netToHost16(&payload->textLength, istream);
	netToHost8((uint8_t*)payload->text, payload->textLength, istream);

	
	return(RETURN_SUCCESS);
}

static int32_t convertNotifKeyUpdateStatusToHost(notif_key_up_status_t* const payload,
												 read_stream_t* const istream)
{
	ASSERT((istream != NULL) && (payload != NULL), E_NULL_POINTER);

	netToHost32(&payload->kIdent.genID, istream);
	netToHost32(&payload->kIdent.serNum, istream);
	netToHost8(&payload->kStatus, sizeof(uint8_t), istream);
	
	return(RETURN_SUCCESS);
}

static int32_t convertNotifSessionInitToHost(notif_session_init_t* const payload,
											 read_stream_t* const istream)
{
	ASSERT((istream != NULL) && (payload != NULL), E_NULL_POINTER);

	netToHost8(&payload->nVersion, sizeof(uint8_t), istream);
	netToHost8(&payload->version, sizeof(uint8_t), istream);
	netToHost8(&payload->appTimeout, sizeof(uint8_t), istream);
	
	return(RETURN_SUCCESS);
	
}

static int32_t convertNotifResponseToHost(notif_response_t* const payload,
										  read_stream_t* const istream)
{

	ASSERT((istream != NULL) && (payload != NULL), E_NULL_POINTER);

	netToHost8(&payload->response, sizeof(uint8_t), istream);
	netToHost16(&payload->reqNum, istream);
	netToHost8(payload->notificationList, sizeof(uint8_t)*payload->reqNum, istream);

	return(RETURN_SUCCESS);
}

static int32_t convertNotifKeyOpReqRcvdToHost(notif_key_op_req_rcvd_t* const payload,
											  read_stream_t* const istream)
{
	ASSERT((istream != NULL) && (payload != NULL), E_NULL_POINTER);

	netToHost16(&payload->maxTime, istream);

	return(RETURN_SUCCESS);
}

static int32_t convertNotifKeyDBChecksumToHost(notif_key_db_checksum_t* const payload,
											   read_stream_t* const istream)
{
	ASSERT((istream != NULL) && (payload != NULL), E_NULL_POINTER);

	netToHost8(payload->checksum, (uint32_t)CHECKSUM_SIZE, istream);
	
	return(RETURN_SUCCESS);
}

static int32_t checkMsgHeader(const msg_header_t* const header,
							  const uint32_t exp_msg_length,
							  const uint32_t exp_sender,
							  const uint32_t exp_seq_num,
							  const uint32_t exp_trans_num)
{
	int32_t ret_val = 0U;

	ASSERT(header != NULL, E_NULL_POINTER);

	if( header->sendIDExp != exp_sender )
	{
		/* wrong sender id */
		ret_val = RESP_WRONG_SENDER_ID;
	}
	else if( header->recIDExp != getMyEtcsIdExp() )
	{
		/* wrong receiver id */
		ret_val = RESP_WRONG_REC_ID;
	}
	else if( header->msgLength !=  exp_msg_length )
	{
		/* wrong msg length */
		ret_val = RESP_WRONG_LENGTH;
	}
	else if( header->msgType > NOTIF_KEY_DB_CHECKSUM )
	{
		/* msg type not supported */
		ret_val = RESP_NOT_SUPPORTED;
	}
	else if( header->version !=  getInterfaceVersion() )
	{
		/* wrong version */
		ret_val = RESP_WRONG_VERSION;
	}
	else if( header->seqNum !=  exp_seq_num )
	{
		/* wrong sequence number */
		ret_val = RESP_WRONG_SEQ_NUM;
	}
	else if( header->transNum !=  exp_trans_num )
	{
		/* wrong transaction number */
		ret_val = RESP_WRONG_TRANS_NUM;
	}
	else
	{
		/* valid header */
		ret_val = RESP_OK;
	}
	
	return(ret_val);
}

/*****************************************************************************
 * PUBLIC FUNCTION DECLARATIONS
 *****************************************************************************/

/* ostream shall be already initialized */
static int32_t sendMsg(write_stream_t* const ostream,
					   const uint32_t tls_des)
{
	
	uint32_t bytes_sent = 0U;
	
	ASSERT(ostream != NULL, E_NULL_POINTER);
	
	sendTLS(&bytes_sent, ostream->buffer, ostream->curSize, tls_des);
	
	if( bytes_sent != ostream->curSize)
	{
		err_print("Cannot complete send operation of msg (bytes sent %d, expectd %d)\n", bytes_sent, ostream->curSize);
		return(-1);
	}
	
#ifdef __DEBUG__
	char dump_msg[2000];
	char tmp_str[5];
	uint32_t i = 0U;
	memset(dump_msg, 0, 2000);
	memset(tmp_str, 0, 5);
	sprintf(dump_msg, "Dump of message sent(%d bytes): ", ostream->curSize);
	for(i = 0U; i < ostream->curSize; i++)
	{
		sprintf(tmp_str, "0x%02X ", ostream->buffer[i]);
		strcat(dump_msg, tmp_str);
	}
	debug_print("%s\n", dump_msg);
#endif
	
	return(RETURN_SUCCESS);
}

/* istream shall be already initialized */
int32_t receiveMsg(read_stream_t* const istream,
				   const uint32_t tls_des)
{

	receiveTLS(&istream->validBytes, istream->buffer, (uint32_t)MSG_MAX_SIZE, tls_des);

#ifdef __DEBUG__
	char dump_msg[2000];
	char tmp_str[5];
	uint32_t i = 0U;
	memset(dump_msg, 0, 2000);
	memset(tmp_str, 0, 5);
	sprintf(dump_msg, "Dump of message recv(%d bytes): ", istream->validBytes);
	for(i = 0U; i < istream->validBytes; i++)
	{
		sprintf(tmp_str, "0x%02X ", istream->buffer[i]);
		strcat(dump_msg, tmp_str);
	}
	debug_print("%s\n", dump_msg);
#endif
	
	return(RETURN_SUCCESS);
}




int32_t initAppSession(const uint32_t peerETCSID,
					   session_t* const curr_session)
{
	notif_session_init_t msg_payload_sent;
	notif_session_init_t msg_payload_received;
	read_stream_t input_msg;
	msg_header_t msg_header;

	ASSERT(curr_session != NULL, E_NULL_POINTER);

	curr_session->peerEtcsIDExp = peerETCSID;
	
	initReadStream(&input_msg);

	msg_payload_sent.nVersion = NUM_VERSION;
	msg_payload_sent.version = getInterfaceVersion();
	msg_payload_sent.appTimeout = 100;

	sendNotifSessionInit(&msg_payload_sent, curr_session);

	receiveMsg(&input_msg, curr_session->tls_des);

	convertMsgHeaderToHost(&msg_header, &input_msg);

	checkMsgHeader(&msg_header,
				   MSG_HEADER_SIZE+3*sizeof(uint8_t),
				   curr_session->peerEtcsIDExp,
				   msg_header.seqNum,
				   curr_session->myTransNum);

	if( msg_header.msgType != NOTIF_SESSION_INIT)
	{
		/* errore */
	}
	else
	{
		convertNotifSessionInitToHost(&msg_payload_received, &input_msg);
	}

	curr_session->mySeqNum++;
	curr_session->peerTransNum = msg_header.transNum;
	curr_session->peerSeqNum = msg_header.seqNum;

	return(RETURN_SUCCESS);	
}

int32_t endAppSession(session_t* const curr_session)
{

	ASSERT(curr_session != NULL, E_NULL_POINTER);

	/* the trans num for end session shall be set to 0 */
	curr_session->myTransNum = 0U;
		
	sendNotifEndUpdate(curr_session);

	return(RETURN_SUCCESS);
}

int32_t startClientTLS(int32_t* const sock)
{
	ASSERT(sock != NULL, E_NULL_POINTER);

	initClientTLS(sock);

	return(RETURN_SUCCESS);
}

int32_t connectToTLSServer(uint32_t* const tls_des,
						   const int32_t sock,
						   const char* const r_ip,
						   const uint16_t r_port)
{
	ASSERT(tls_des != NULL, E_NULL_POINTER);
	ASSERT(r_ip != NULL, E_NULL_POINTER);

	connectTLS(tls_des, sock, r_ip, r_port);

	return(RETURN_SUCCESS);
}

int32_t startServerTLS(int32_t* const listen_sock,
					   const uint16_t l_port)
{
	ASSERT(listen_sock != NULL, E_NULL_POINTER);
	
	initServerTLS(listen_sock, l_port);

	return(RETURN_SUCCESS);
}

int32_t waitForTLSClient(uint32_t* const tls_des,
						 int32_t* const client_sock,
						 const int32_t listen_sock)
{
	ASSERT(tls_des != NULL, E_NULL_POINTER);
	ASSERT(client_sock != NULL, E_NULL_POINTER);

	acceptTLS(tls_des, client_sock, listen_sock);

	return(RETURN_SUCCESS);
}

int32_t closeTLSConnection(const uint32_t tls_des,
						   const int32_t sock)
{

	closeTLS(tls_des, sock);

	return(RETURN_SUCCESS);
}



