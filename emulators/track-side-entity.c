/*
 *
 * Copyright (C) 2016 Neat S.r.l.
 *
 * This file is part of Kmc-Subset137.
 *
 * Kmc-Subset137 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Kmc-Subset137 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**************************************************************************//**
 *
 * KMC-SS137 track-side-entity emulator for testing as needed by 
 * Kmc-Subset137 project.
 *
 * This files contains the implementation of an emulator for a 
 * Track-side-Entity as described in the document:
 * "ERTMS/ECTS; On-line Key Management FFFIS" SUBSET-137 ver 1.0.0 (17/12/2015)
 * This emulator shall be used only for testing.
 *
 * @file: emulators/track-side-entity.c
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

#include <stdio.h>     /* for fopen, snprintf, etc... */
#include <string.h>    /* for memmove, memcmp, memset */
#include <arpa/inet.h> /* for htons, etc.. */
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <libgen.h>

#include "utils.h"
#include "net_utils.h"
#include "msg_definitions.h"
#include "ss137_lib.h"

#define RSA_CA_CERT    "../CA/ca.crt"   /**< RSA root CA Certificate pathname, pem format */
#define RSA_CERT       "./dummy-certs/KmcEntity.crt" /**< RSA Certificate pathname, pem format */
#define RSA_KEY        "./dummy-certs/KmcEntity.key"  /**< RSA Key pathname , no encryption, pem format*/

ss137_lib_configuration_t ss137_lib_config =
{
	RSA_CA_CERT,
	RSA_KEY,
	RSA_CERT,
	{0x11223344, "127.0.0.1"},
	{
		{0xAABBCCDD, "127.0.0.1"}
	}
};

int main(int argc, char *argv[])
{
	request_t request;
	bool_t stop = FALSE;
	response_t response;
	uint32_t i = 0U;
	uint32_t exp_etcs_id = 0U;

	if(startServerTLS() != SUCCESS)
	{
		exit(1);
	}

	while(1)
	{
		session_t session;
		memset(&session, 0, sizeof(session_t));

		if(listenForTLSClient(&session.tlsID, &exp_etcs_id) != SUCCESS)
		{
			exit(1);
		}

		if(initAppSession(&session, 0xff, exp_etcs_id) != SUCCESS)
		{
			closeTLSConnection(session.tlsID);
			continue;
		}

		log_print("----------------------------------------------------------\n");
		log_print("----------------------------------------------------------\n");

		while(stop == FALSE)
		{
			if(waitForRequestFromKMCToKMAC(&request, &session) != SUCCESS)
			{
				stop = TRUE;
				continue;
			}

			log_print("Request received : %d\n", request.msgType);

			switch(request.msgType)
			{
			case(NOTIF_END_OF_UPDATE):
				stop = TRUE;
				break;
			case(CMD_REQUEST_KEY_DB_CHECKSUM):
				/* evaluate crc */
				for(i=0U; i<sizeof(response.dbChecksumPayload.checksum); i++)
				{
					response.dbChecksumPayload.checksum[i] = i;
				}
				sendNotifKeyDBChecksum(&response, &session);
				break;
			case(CMD_DELETE_ALL_KEYS):
				response.notifPayload.reason = RESP_OK;
				response.notifPayload.reqNum = 0;
				if(sendNotifResponse(&response, &session) != SUCCESS)
				{
					stop = TRUE;
					continue;
				}
				break;
			default:
				/* some processing of request */
				response.notifPayload.reason = RESP_OK;
				response.notifPayload.reqNum = request.reqNum;

				for(i = 0U; i < request.reqNum; i++)
				{
					response.notifPayload.notificationList[i] = 0U;
				}
				
				if(sendNotifResponse(&response, &session) != SUCCESS)
				{
					stop = TRUE;
					log_print("End of update message received \n");
					continue;
				}

				break;
			}
			session.transNum++;
			request.reqNum = 0;
			log_print("----------------------------------------------------------\n");
			log_print("----------------------------------------------------------\n");
			sleep(5U);
		}
		
		stop = FALSE;
		closeTLSConnection(session.tlsID);
	}
	
	return(0);
}





