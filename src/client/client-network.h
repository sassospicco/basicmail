/*
 * client-network.h
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

 *  You should have received a copy of the GNU Affero Public License
 *  along with BasicMail. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CLIENT_NETWORK_H_
#define CLIENT_NETWORK_H_

#include <netinet/in.h>
#include <sys/socket.h>

int establish_connection(void* server_addr) {
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	if (sock < 0) {
		return -1;
	}
	
	int cresult = connect(sock, (struct sockaddr*) server_addr, sizeof(struct sockaddr));
	
	if (cresult < 0) {
		return -2;
	}
	
	return sock;
}

#endif /* CLIENT_NETWORK_H_ */
