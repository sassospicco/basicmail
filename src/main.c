/*
 * main.c
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

#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define VERSION "1.0"

void welcome();
void usage();
void help();
void copyright();

char home[256+18];

#include "./common/network.h"
#include "./server/server.h"
#include "./client/client.h"

int main(int argc, char *argv[]) {
	signal(SIGPIPE, SIG_IGN);
	
	/*
	 * Action missing
	 */
	if (argc < 2) {
		usage();
		return EXIT_FAILURE;
	}
	
	/*
	 * Retrieving $HOME
	 */
	strcpy(home, getenv("HOME"));
	strcat(home, "/.basicmail");
	
	if (strcmp(argv[1], "server") == 0) {
		welcome();
		
		unsigned int port = DEFAULT_PORT;
		
		int i;
		for (i = 2; i < argc; i++) {
			if (argv[i][0] == '-') {
				switch (argv[i][1]) {
					case 'p':
						sscanf(argv[i+1], "%u", &port);
						i++;
						break;
					default:
						usage();
						return EXIT_FAILURE;
				}
			} else {
				usage();
				return EXIT_FAILURE;
			}
		}
		
		server(port);
	} else if (strcmp(argv[1], "client") == 0) {
		welcome();
		
		struct sockaddr_in server_addr = {};
		server_addr.sin_family = AF_INET;
		int port = DEFAULT_PORT;
		unsigned int a1 = 127, a2 = 0, a3 = 0, a4 = 1;
		unsigned int ah;
		char hostname[256];
		
		int i;
		for (i = 2; i < argc; i++) {
			if (argv[i][0] == '-') {
				switch (argv[i][1]) {
					case 's':
						sscanf(argv[++i], "%u.%u.%u.%u", &a1, &a2, &a3, &a4);
						server_addr.sin_addr.s_addr = htonl((a1 << 24) + (a2 << 16) + (a3 << 8) + a4);
						break;
					case 'h':
						sscanf(argv[++i], "%255s", hostname);
						printf("%s\n", hostname);
						struct hostent* hp = gethostbyname(hostname);
						if (hp == NULL) {
							printf("Hostname not found.\n");
							return EXIT_FAILURE;
						} else {
							memcpy(&server_addr.sin_addr.s_addr, hp->h_addr_list[0], hp->h_length);
							ah = ntohl(server_addr.sin_addr.s_addr);
							printf("Hostname found at IP address %u.%u.%u.%u\n",
									(ah >> 24) & 0xFF,
									(ah >> 16) & 0xFF,
									(ah >> 8) & 0xFF,
									ah & 0xFF
									);
						}
						break;
					case 'p':
						sscanf(argv[++i], "%d", &port);
						break;
					default:
						usage();
						return EXIT_FAILURE;
				}
			} else {
				usage();
				return EXIT_FAILURE;
			}
		}
		
		server_addr.sin_port = htons(port);
		client(&server_addr);
	} else if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
		help();
	} else {
		usage();
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

void welcome() {
	printf("BasicMail %s, a simple message exchange suite.\n", VERSION);
	printf("Run with \'help\' action for usage, license and warranty terms.\n");
}

void usage() {
	printf("BasicMail %s, a simple message exchange suite.\n", VERSION);
	printf("\tUsage: basicmail <server|client|help> [OPTIONS]\n");
}

void help() {
	usage();
	printf("\n");
	printf("Server:\n");
	printf("\t-p,\tListening port, default is %d\n\n", DEFAULT_PORT);
	printf("Client:\n");
	printf("\t-s,\tServer IP address, default is 127.0.0.1\n");
	printf("\t-h,\tServer hostname, e.g. example.com\n");
	printf("\t-p,\tServer port, default is %d\n", DEFAULT_PORT);
	printf("\n");
	printf("\t-s and -h parameters are obviously conflicting, last one will take precedence.\n");
	printf("\n");
	copyright();
}

void copyright() {
	printf("Copyright (C) 2013, Stefano Tribioli\n");
	printf("BasicMail is free software: you can redistribute it and/or modify it under the terms of the GNU Affero Public License.\n");
	printf("The source of BasicMail (including complete license text) is available at [http://github.com/sassospicco/basicmail]\n");
	printf("BasicMail is distributed with ABSOLUTELY NO WARRANTY; see license for details.\n");
}
