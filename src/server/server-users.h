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

void load_users() {
	/*
	 * Reserves maximum possible space for temporary user table
	 */
	user* tmp_users;
	tmp_users = (user*) malloc(MAX_USRS * sizeof(user));
	
	if (tmp_users == NULL) {
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
					tmp_users[u].id = u;
					
					char* tab_ptr = strchr(line, '\t');
					if (tab_ptr != NULL) {
						int pos = tab_ptr - line;
						memcpy(tmp_users[u].name, &line, pos);
						memcpy(tmp_users[u].password, line+pos+1, strlen(line)-pos-1);
						u++;	
					}
				}
				
				i++;
				j = 0;
			} else {
				line[j] = buffer[i];
				i++;
				j++;
			}
		}
	} while (out > 0);
	
	/*
	 * Copying temporary table to definitive location
	 */
	users = malloc(u * sizeof(user));
	
	if (users == NULL) {
		printf("Failed to allocate memory.\n");
		exit(EXIT_FAILURE);
	}
	
	memcpy(users, tmp_users, u * sizeof(user));
	free(tmp_users);
	users_len = u;
}

#endif /* SERVER_USERS_H_ */
