/*
 * server-network.h
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

#ifndef SERVER_NETWORK_H_
#define SERVER_NETWORK_H_

#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#define MAX_WAITING 20

int setup_socket(unsigned int hport) {
	int sock_wait;
	int cresult = 0;
	int port = htons(hport);
	
	struct sockaddr_in addr = {AF_INET, port, {INADDR_ANY}};
	
	sock_wait = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	cresult += bind(sock_wait, (struct sockaddr*) &addr, sizeof(addr));
	cresult += listen(sock_wait, MAX_WAITING);
	
	if (sock_wait < 0 || cresult < 0) {
		printf("Unable to open socket on port %d.\n", hport);
		printf("Errno: %d\n", errno);
		exit(EXIT_FAILURE);
	}
	
	return sock_wait;
}

#endif /* SERVER_NETWORK_H_ */
