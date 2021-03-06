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
 * KMC-SS137 kmc-core compliant to Subset137.
 *
 * This files contains the implementation a Key Management Center 
 * compliant to requirements set in:
 * "ERTMS/ECTS; On-line Key Management FFFIS" SUBSET-137 ver 1.0.0. (17/12/2015).
 *
 * @file: kmc-core/kmc-core.c
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
#include <sys/time.h>
#include <libgen.h>

#include "utils.h"
#include "net_utils.h"
#include "msg_definitions.h"
#include "ss137_lib.h"

#define RSA_CA_CERT    "../CA/ca.crt"   /**< RSA root CA Certificate pathname, pem format */
#define RSA_CERT       "./dummy-certs/KmcCore.crt" /**< RSA Certificate pathname, pem format */
#define RSA_KEY        "./dummy-certs/KmcCore.key"  /**< RSA Key pathname , no encryption, pem format*/

k_struct_t k_struct =
{
		KMAC_SIZE,
		{
				0xaabbccdd,
				0xddeeff00,
		},
		0xaabbccdd,
		{
			0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 077,
			0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 077,
			0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 077
		},
		1,
		{
			0xaabbccdd,
		},
		0x00000000,
		0xffffffff
};

k_ident_t k_ident =
{
		0xaabbccdd,
		0xddeeff00
};

k_validity_t k_validity =
{
		{
				0xaabbccdd,
				0xddeeff00
		},
		0x00000000,
		0xffffffff
};


k_entity_t k_entity =
{
		{
				0xaabbccdd,
				0xddeeff00
		},
		1,
		{
			0xaabbccdd
		}
};

ss137_lib_configuration_t ss137_lib_config =
{
	RSA_CA_CERT,
	RSA_KEY,
	RSA_CERT,
	{0xAABBCCDD, "127.0.0.1"},
	{
		{0x11223344, "127.0.0.1"}
	}
};

int main(int argc, char *argv[])
{
	session_t session;
	request_t request;
	response_t response;

	memset(&session, 0, sizeof(session_t));

	if(startClientTLS(&session.tlsID) != SUCCESS)
	{
		exit(1);
	}

	if(connectToTLSServer(session.tlsID, ss137_lib_config.kmsEntitiesId[0].ip) == SUCCESS)
	{
		if(initAppSession(&session, 0x3, ss137_lib_config.kmsEntitiesId[0].expEtcsId) != SUCCESS)
		{
			closeTLSConnection(session.tlsID);
		}
		else
		{
			
			request.reqNum = 1;
			log_print("----------------------------------------------------------\n");
			log_print("----------------------------------------------------------\n");

			memmove(request.kStructList, &k_struct, sizeof(k_struct_t));
			if(performAddKeysTransaction(&response, &session, &request) != SUCCESS)
			{
				exit(1);
			}
			
			log_print("----------------------------------------------------------\n");
			log_print("----------------------------------------------------------\n");
			
			memmove(request.kIdentList, &k_ident, sizeof(k_ident_t));
			if(performDelKeysTransaction(&response, &session, &request)!= SUCCESS)
			{
				exit(1);
			}
			
			log_print("----------------------------------------------------------\n");
			log_print("----------------------------------------------------------\n");
			
			memmove(request.kValidityList, &k_validity, sizeof(k_validity_t));
			if(performUpKeyValiditiesTransaction(&response, &session, &request)!= SUCCESS)
			{
				exit(1);
			}
			
			log_print("----------------------------------------------------------\n");
			log_print("----------------------------------------------------------\n");
			
			memmove(request.kEntityList, &k_entity, sizeof(k_entity_t));
			if(performUpKeyEntitiesTransaction(&response, &session, &request)!= SUCCESS)
			{
				exit(1);
			}
			
			log_print("----------------------------------------------------------\n");
			log_print("----------------------------------------------------------\n");
			
			if(performDeleteAllKeysTransaction(&response, &session)!= SUCCESS)
			{
				exit(1);
			}
			
			log_print("----------------------------------------------------------\n");
			log_print("----------------------------------------------------------\n");
			
			if(performReqDBChecksumTransaction(&response, &session)!= SUCCESS)
			{
				exit(1);
			}

			log_print("----------------------------------------------------------\n");
			log_print("----------------------------------------------------------\n");
			
		}
		
		endAppSession(&session);
		
		closeTLSConnection(session.tlsID);
	}
	
	return(0);
}


	

