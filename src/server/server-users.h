/*
 * server-users.h
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

#ifndef SERVER_USERS_H_
#define SERVER_USERS_H_

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../common/users.h"

int users_len;
user* users;

#define USERS_SIZE_INIT 50

void load_users() {
	/*
	 * Tentatively reserving space for user table
	 */
	int users_size = USERS_SIZE_INIT;
	users = (user*) malloc(USERS_SIZE_INIT * sizeof(user));
	
	if (users == NULL) {
		printf("Failed to allocate memory.\n");
		exit(EXIT_FAILURE);
	}

	char users_path[sizeof(home)+10];
	strcpy(users_path, home);
	strcat(users_path, "/cfg/users");
	int fd = open(users_path, O_RDONLY);
	if (fd == -1) {
		printf("Failed to load users file.\n");
		exit(EXIT_FAILURE);
	}
	
	char buffer[(2+MAX_FLEN*2)*10 +1];
	char line[2+MAX_FLEN*2 +1];
	int out;
	
	// u is the offset in tmp_users
	int u = 0;
	// j is the offset in the current line
	int j = 0;
	
	do {
		out = read(fd, &buffer, sizeof(buffer));
		
		if (out < 0) {
			printf("Failed to read users file.\n");
			exit(EXIT_FAILURE);
		}
		
		// i is the offset in the buffer
		int i = 0;
		
		while (i < out) {
			if (buffer[i] == '\n') {
				line[j] = '\0';
				
				/*
				 * Processing the line (if non-empty and not starting with a #)
				 */
				if (line[0] != '#' && j > 0) {
					/*
					 * Reallocating space if necessary
					 */
					if (u >= users_size) {
						users = (user*) realloc(users, users_size+USERS_SIZE_INIT);
						users_size += USERS_SIZE_INIT;
						
						if (users == NULL) {
							printf("Failed to allocate memory.\n");
							exit(EXIT_FAILURE);
						}
					}
					
					char* tab_ptr = strchr(line, '\t');
					if (tab_ptr != NULL) {
						users[u].id = u;
						
						int pos = tab_ptr - line;
						int cpylen;
						
						if (pos <= MAX_FLEN) {
							cpylen = pos;
						} else {
							cpylen = MAX_FLEN;
						}
						memcpy(users[u].name, &line, cpylen);
						users[u].name[cpylen] = '\0';
						
						if (strlen(line)-pos-1 <= MAX_FLEN) {
							cpylen = strlen(line)-pos-1;
						} else {
							cpylen = MAX_FLEN;
						}
						memcpy(users[u].password, line+pos+1, cpylen);
						users[u].password[cpylen] = '\0';
						u++;
					}
				}
				
				i++;
				j = 0;
			} else {
				if (j < sizeof(line)-1) {
					line[j] = buffer[i];
				}
				i++;
				j++;
			}
		}
	} while (out > 0);
	
	/*
	 * Resizing users table
	 */
	users = (user*) realloc(users, u * sizeof(user));
	
	if (users == NULL) {
		printf("Failed to allocate memory.\n");
		exit(EXIT_FAILURE);
	}

	users_len = u;
}

#endif /* SERVER_USERS_H_ */
