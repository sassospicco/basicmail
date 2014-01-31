/*
 * server.h
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

#ifndef SERVER_H_
#define SERVER_H_

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "../common/mutex.h"

int status;
pthread_t tid;

#include "server-store.h"
#include "server-users.h"
#include "server-network.h"
#include "server-actions.h"

/**
 * server function enstablishes a listening socket on passed port and spawns a
 * new thread for every incoming connection
 */
void server(unsigned int port) {
	load_users();
	init_lstore();
	
	int sock_wait = setup_socket(port);
	setup_sem();
	
	printf("Server will now listen on port %u\n", port);
	
	/*
	 * Listening loop
	 */
	while (1) {
		int sock = accept(sock_wait, NULL, NULL);
		
		if (sock < 0) {
			printf("Unable to listen on socket.\n");
			exit(EXIT_FAILURE);
		}
		
		printf("Incoming connection.\n");
		
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		
		if (pthread_create(&tid, &attr, handle_connection, (void*) &sock) != 0) {
			printf("Connection handling failed.\n");
		}
	}
}

#endif /* SERVER_H_ */
