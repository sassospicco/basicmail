/*
 * client.h
 *
 *  Copyright (C) 2013, Stefano Tribioli
 *
 *  This file is part of BasicMail.
 *
 *  BasicMail is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  BasicMail is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero Public License for more details.
 *
 *  You should have received a copy of the GNU Affero Public License
 *  along with BasicMail. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/network.h"
#include "../common/users.h"
#include "client-actions.h"
#include "client-console.h"
#include "client-network.h"

void client(struct sockaddr_in* server) {
	user usr = {};
	char template[10];
	snprintf(template, sizeof(template), "%%""%d""s", MAX_FLEN);
	char c;
	
	/*
	 * Connecting to server
	 */
	int sock = establish_connection(server);

	if (sock < 1) {
		if (sock == -2) {
			printf("No response from server.\n");
		} else {
			printf("Unable to open a network connection.\n");
		}
		
		exit(EXIT_FAILURE);
	}
	
	/*
	 * Asking for username
	 */
	print_console();
	printf("Type username: ");
	scanf(template, usr.name);
	do {
		c = getchar();
	} while (c != EOF && c != '\n' );
	clearerr(stdin);
	
	/*
	 * Asking for password
	 */
	print_console();
	printf("Type password: ");
	scanf(template, usr.password);
	do {
		c = getchar();
	} while (c != EOF && c != '\n' );
	clearerr(stdin);
	
	request req;
	strcpy(req.from, usr.name);
	strcpy(req.command, "AUTH");
	perform_other(&usr, &req, sock);
	
	/*
	 * Action loop
	 */
	while (1) {
		memset(&req, 0, sizeof(req));
		strcpy(req.from, usr.name);
		
		print_console();
		
		/*
		 * Waiting for command
		 */
		snprintf(template, sizeof(template), "%%""%d""s", sizeof(req.command));
		scanf(template, req.command);
		do {
			c = getchar();
		} while (c != EOF && c != '\n' );
		clearerr(stdin);
		
		if (
				strcmp(req.command, "send") == 0 ||
				strcmp(req.command, "read") == 0 ||
				strcmp(req.command, "delete") == 0
				) {
			
			if (strcmp(req.command, "send") == 0) {
				strcpy(req.command, "SEND");
				perform_send(&usr, &req, sock);
			} else if (strcmp(req.command, "read") == 0) {
				strcpy(req.command, "READ");
				perform_other(&usr, &req, sock);
			} else if (strcmp(req.command, "delete") == 0) {
				strcpy(req.command, "DELETE");
				perform_other(&usr, &req, sock);
			}
		} else if (strcmp(req.command, "quit") == 0 || strcmp(req.command, "exit") == 0) {
			exit(EXIT_SUCCESS);
		} else {
			printf("Unknown command\n");
		}
	}
}

#endif /* CLIENT_H_ */
