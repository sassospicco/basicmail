/*
 * client-actions.h
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

#ifndef CLIENT_ACTIONS_H_
#define CLIENT_ACTIONS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../common/buffered_io.h"
#include "../common/network.h"
#include "../common/sha1.h"
#include "../common/sha1.polarssl.h"
#include "../common/users.h"
#include "client-console.h"

#define BODY_SIZE_INIT 4096

void perform_send(user* usr, request* req, int sock) {
	char template[10];
	char c;
	
	/*
	 * Computing HMAC (header only)
	 */
	sha1_context ctx;
	sha1_hmac_starts(&ctx, (unsigned char*) usr->password, strlen(usr->password));
	sha1_hmac_update(&ctx, (unsigned char*) req->command, strlen(req->command));
	sha1_hmac_update(&ctx, (unsigned char*) "\n", 1);
	sha1_hmac_update(&ctx, (unsigned char*) req->from, strlen(req->from));
	sha1_hmac_update(&ctx, (unsigned char*) "\n", 1);
	
	/*
	 * Asking for recipient
	 */
	print_console();
	printf("Type recipient: ");
	snprintf(template, sizeof(template), "%%""%d""s", MAX_FLEN);
	scanf(template, req->to);
	do {
		c = getchar();
	} while (c != EOF && c != '\n' );
	clearerr(stdin);
	sha1_hmac_update(&ctx, (unsigned char*) req->to, strlen(req->to));
	sha1_hmac_update(&ctx, (unsigned char*) "\n", 1);
	
	/*
	 * Asking for object
	 */
	print_console();
	printf("Type object: ");
	snprintf(template, sizeof(template), "%%""%d""s", MAX_OLEN);
	scanf(template, req->object);
	do {
		c = getchar();
	} while (c != EOF && c != '\n' );
	clearerr(stdin);
	sha1_hmac_update(&ctx, (unsigned char*) req->object, strlen(req->object));
	sha1_hmac_update(&ctx, (unsigned char*) "\n", 1);
	
	/*
	 * Reading body
	 */
	int body_index = 0;
	int body_size = BODY_SIZE_INIT;
	char* body = (char*) malloc(BODY_SIZE_INIT);
	
	if (body == NULL) {
		printf("Failed to allocate memory.\n");
		exit(EXIT_FAILURE);
	}
	
	print_console();
	printf("Type message body, press ESC and Enter when done.\n");
	
	while (1) {
		c = getchar();
		
		if (c == 27) {
			getchar();
			break;
		}
		
		if (body_index >= body_size) {
			body = (char*) realloc(body, body_size+BODY_SIZE_INIT);
			body_size += BODY_SIZE_INIT;
			
			if (body == NULL) {
				printf("Failed to allocate memory.\n");
				exit(EXIT_FAILURE);
			}
		}
		
		body[body_index++] = c;
	}
	
	/*
	 * Asking for confirmation.
	 */
	print_console();
	printf("Do you want to send your message? [Y/n]\n");
	print_console();
	
	c = getchar();
	
	if (c == 'n' || c == 'N') {
		return;
	} else if (c!= EOF && c != '\n') {
		do {
			c = getchar();
		} while (c != EOF && c != '\n' );
	}
	
	/*
	 * Computing HMAC
	 */
	sha1_hmac_update(&ctx, (unsigned char*) body, body_index);
	unsigned char raw[SHA1_RAW_LEN];
	sha1_hmac_finish(&ctx, raw);
	sha1_to_hex(raw, req->hmac);
	
	/*
	 * Binding buffers to socket
	 */
	bfr_ou netou = {};
	bfr_in netin = {};
	netou.filedes = netin.filedes = sock;
	netou.typedes = netin.typedes = TYPEDES_STREAM;
	
	/*
	 * Binding buffer to STDOUT
	 */
	bfr_ou stdou = {};
	stdou.filedes = STDOUT_FILENO;
	stdou.typedes = TYPEDES_BLOCK;
	
	/*
	 * Sending request
	 */
	write_str(&netou, "basicmail 1\n");
	write_str(&netou, req->hmac);
	write_char(&netou, '\n');
	write_str(&netou, req->command);
	write_char(&netou, '\n');
	write_str(&netou, req->from);
	write_char(&netou, '\n');
	write_str(&netou, req->to);
	write_char(&netou, '\n');
	write_str(&netou, req->object);
	write_char(&netou, '\n');
	write_array(&netou, body, body_index);
	write_char(&netou, 3);
	flush_buffer(&netou);
	
	/*
	 * Showing server response in STDOUT
	 */
	int r = pipe_buffers(&netin, &stdou);
	if (r == 0) {
		printf("Server closed connection.\n");
		close(netin.filedes);
		exit(EXIT_SUCCESS);
	}
	
	free(body);
}

void perform_other(user* usr, request* req, int sock) {
	/*
	 * Computing HMAC
	 */
	sha1_context ctx;
	sha1_hmac_starts(&ctx, (unsigned char*) usr->password, strlen(usr->password));
	sha1_hmac_update(&ctx, (unsigned char*) req->command, strlen(req->command));
	sha1_hmac_update(&ctx, (unsigned char*) "\n", 1);
	sha1_hmac_update(&ctx, (unsigned char*) req->from, strlen(req->from));
	char* trailing = "\nnull\nnull\n";
	sha1_hmac_update(&ctx, (unsigned char*) trailing, strlen(trailing));
	unsigned char raw[SHA1_RAW_LEN];
	sha1_hmac_finish(&ctx, raw);
	sha1_to_hex(raw, req->hmac);
	
	/*
	 * Binding buffers to socket
	 */
	bfr_ou netou = {};
	bfr_in netin = {};
	netou.filedes = netin.filedes = sock;
	netou.typedes = netin.typedes = TYPEDES_STREAM;
	
	/*
	 * Binding buffer to STDOUT
	 */
	bfr_ou stdou = {};
	stdou.filedes = STDOUT_FILENO;
	stdou.typedes = TYPEDES_BLOCK;
	
	/*
	 * Sending request
	 */
	write_str(&netou, "basicmail 1\n");
	write_str(&netou, req->hmac);
	write_char(&netou, '\n');
	write_str(&netou, req->command);
	write_char(&netou, '\n');
	write_str(&netou, req->from);
	write_str(&netou, trailing);
	write_char(&netou, 3);
	flush_buffer(&netou);
	
	/*
	 * Showing server response in STDOUT
	 */
	int r = pipe_buffers(&netin, &stdou);
	if (r == 0 || r == 4) {
		printf("Server closed connection.\n");
		close(netin.filedes);
		exit(EXIT_SUCCESS);
	}
}

#endif /* CLIENT_ACTIONS_H_ */
