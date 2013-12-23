/*
 * server-actions.h
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

#ifndef SERVER_ACTIONS_H_
#define SERVER_ACTIONS_H_

#include <asm-generic/errno-base.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "../common/buffered_io.h"
#include "../common/mutex.h"
#include "../common/network.h"
#include "../common/sha1.h"
#include "../common/sha1.polarssl.h"
#include "../common/users.h"
#include "server.h"
#include "server-store.h"
#include "server-users.h"

void* handle_connection(void*);
int handle_send(request*, bfr_in*, bfr_ou*, sha1_context*);
int handle_read(request*, bfr_ou*);
int handle_delete(request*, bfr_ou*);
int handle_license(request*, bfr_ou*);
int filter_dirs(const struct dirent *);

/*
 * Parent function of working thread.
 * Checking headers and passing control to appropriate action.
 */
void* handle_connection(void* sock_ptr) {
	bfr_in netin = {};
	bfr_ou netou = {};
	request req = {};
	int sock = *((int*) sock_ptr);
	
	netin.filedes = netou.filedes = sock;
	netin.typedes = netou.typedes = TYPEDES_STREAM;
	
	/*
	 * While more bytes are available in input (ie: more requests queued)
	 */
	do {
		/*
		 * Parsing TCP buffer to request struct
		 */
		read_line(&netin, req.version, sizeof(req.version));
		read_line(&netin, req.hmac, sizeof(req.hmac));
		read_line(&netin, req.command, sizeof(req.command));
		read_line(&netin, req.from, sizeof(req.from));
		read_line(&netin, req.to, sizeof(req.to));
		read_line(&netin, req.object, sizeof(req.object));
		
		/*
		 * Checking header
		 */
		if (strcmp(req.version, "basicmail 1") != 0) {
			write_str_c(&netou, "Bad request\n\n");
			consume_garbage(&netin);
			continue;
		}
		
		/*
		 * Finding user
		 */
		int i;
		user from;
		for (i = 0; i < users_len; i++) {
			if (strcmp(users[i].name, req.from) == 0) {
				break;
			}
		}
		if (i < users_len) {
			from = users[i];
		} else {
			write_str_d(&netou, "Unknown user.\n");
			return NULL;
		}
		
		/*
		 * Checking HMAC (header only)
		 */
		sha1_context ctx;
		sha1_hmac_starts(&ctx, (unsigned char*) from.password, strlen(from.password));
		sha1_hmac_update(&ctx, (unsigned char*) req.command, strlen(req.command));
		sha1_hmac_update(&ctx, (unsigned char*) "\n", 1);
		sha1_hmac_update(&ctx, (unsigned char*) req.from, strlen(req.from));
		sha1_hmac_update(&ctx, (unsigned char*) "\n", 1);
		sha1_hmac_update(&ctx, (unsigned char*) req.to, strlen(req.to));
		sha1_hmac_update(&ctx, (unsigned char*) "\n", 1);
		sha1_hmac_update(&ctx, (unsigned char*) req.object, strlen(req.object));
		sha1_hmac_update(&ctx, (unsigned char*) "\n", 1);
		if (strcmp(req.command, "READ") == 0 || strcmp(req.command, "DELETE") == 0) {
			consume_garbage(&netin);
			unsigned char raw[SHA1_RAW_LEN];
			char hex[SHA1_HEX_LEN+1];
			sha1_hmac_finish(&ctx, raw);
			sha1_to_hex(raw, hex);
			if (strcmp(req.hmac, hex) != 0) {
				write_str_d(&netou, "Authentication failed.\n");
#ifdef DEBUG
				printf("Expected HMAC: %s\n", hex);
#endif
				return NULL;
			}
		}
		
		/*
		 * Passing control to appropriate action
		 */
		int handling;
		if (strcmp(req.command, "SEND") == 0) {
#ifdef DEBUG
			printf("User %s requesting write lock\n", req.from);
#endif
			write_lock();
#ifdef DEBUG
			printf("User %s obtaining write lock\n", req.from);
			sleep(5);
#endif
			handling = handle_send(&req, &netin, &netou, &ctx);
#ifdef DEBUG			
			printf("User %s releasing write lock\n", req.from);
#endif
			write_unlock();
		} else if (strcmp(req.command, "READ") == 0) {
#ifdef DEBUG
			printf("User %s requesting read lock\n", req.from);
#endif
			read_lock();
#ifdef DEBUG
			printf("User %s obtaining read lock\n", req.from);
			sleep(5);
#endif
			handling = handle_read(&req, &netou);
#ifdef DEBUG			
			printf("User %s releasing read lock\n", req.from);
#endif
			read_unlock();
		} else if (strcmp(req.command, "DELETE") == 0) {
#ifdef DEBUG
			printf("User %s requesting write lock\n", req.from);
#endif
			write_lock();
#ifdef DEBUG
			printf("User %s obtaining write lock\n", req.from);
			sleep(5);
#endif
			handling = handle_delete(&req, &netou);
#ifdef DEBUG			
			printf("User %s releasing write lock\n", req.from);
#endif
			write_unlock();
		} else if (strcmp(req.command, "LICENSE") == 0) {
			handling = handle_license(&req, &netou);
		} else {
			consume_garbage(&netin);
			write_str_c(&netou, "Unknown command.\n");
			continue;
		}
		
		if (handling > -1) {
#ifdef DEBUG
			printf("Request successfully handled.\n");
#endif
		}
#ifndef DEBUG
	} while (netin.valid > 0);
#else
	} while (netin.valid > 0 && printf("----\n") < 10);
#endif
	
	/*
	 * Closing socket
	 */
	close(sock);
	
#ifdef DEBUG
	printf("Connection closed.\n");
#endif
	
	return &status;
}

int handle_send(request* req, bfr_in* netin, bfr_ou* netou, sha1_context* ctx) {	
	bfr_ou bstore = {};
	
	/*
	 * Checking recipient
	 */
	int i;
	for (i = 0; i < users_len; i++) {
		if (strcmp(users[i].name, req->to) == 0) {
			break;
		}
	}
	if (i >= users_len) {
		write_str_c(netou, "Unknown recipient.\n");
		consume_garbage(netin);
		return -1;
	}
	
	/*
	 * Computing unique filename
	 */
	char uid[21];
	snprintf(uid, sizeof(uid), "%011ld%s", time(NULL), req->hmac);
	
	/*
	 * Opening file
	 */
	char file[strlen(lstore)+1+MAX_FLEN+1+strlen(uid)];
	strcpy(file, lstore);
	strcat(file, "/");
	strcat(file, req->to);
	strcat(file, "/");
	strcat(file, uid);
	bstore.filedes = open(file, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
	bstore.typedes = TYPEDES_BLOCK;
	
	if (bstore.filedes < 0 && errno == ENOENT) {
		file[strlen(lstore)+strlen(req->to)] = '\0';
		mkdir(file, S_IRWXU | S_IRGRP);
		file[strlen(lstore)+strlen(req->to)] = '/';
		bstore.filedes = open(file, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
	}
	
	if (bstore.filedes < 0) {
		write_str_c(netou, "Unable to open file.\n");
		return -1;
	}
	
	/*
	 * Saving message metadata
	 */
	int wresult = 0;
	wresult += write_array(&bstore, req->from, strlen(req->from));
	wresult += write_char(&bstore, '\n');
	wresult += write_array(&bstore, req->object, strlen(req->object));
	wresult += write_char(&bstore, '\n');
	if (wresult < 0) {
		close(bstore.filedes);
		return -1;
	}
	
	/*
	 * Saving message body
	 */
	int guard = 0;
	do {
		char curr = read_char(netin);
		sha1_hmac_update(ctx, (unsigned char*) &curr, 1);
		
		if (curr == '\n') {
			if (guard == 1) {
				break;
			} else {
				guard = 1;
				continue;
			}
		} else {
			guard = 0;
			
			if (curr == '\\') {
				char next = read_char(netin);
				if (next == 'n') {
					sha1_hmac_update(ctx, (unsigned char*) &next, 1);
					wresult += write_char(&bstore, '\n');
				} else  {
					sha1_hmac_update(ctx, (unsigned char*) &next, 1);
					wresult += write_char(&bstore, next);
				}
			} else {
				wresult += write_char(&bstore, curr);				
			}
		}
		
		if (netin->valid < 0 || wresult < 0) {
			return -1;
		}
	} while (netin->valid > 0);
	flush_buffer(&bstore);
	
	/*
	 * Checking HMAC
	 */
	if (sha1_hmac_finish_check(ctx, req->hmac) != 0) {
		write_str_d(netou, "Authentication failed.\n");
		close(bstore.filedes);
		unlink(file);
		return -1;
	}
	
	/*
	 * Closing file
	 */
	close(bstore.filedes);
	
	write_str_c(netou, "Message sent.\n");
	
	return 0;
}

int handle_read(request* req, bfr_ou* netou) {
	/*
	 * Listing directory
	 */
	char dir[strlen(lstore)+MAX_FLEN+1];
	strcpy(dir, lstore);
	strcat(dir, req->from);
	struct dirent** list;
	int n = scandir(dir, &list, filter_dirs, alphasort);
	if (n < 1) {
		if (n == 0 || errno == ENOENT) {
			if (n == 0) {
				free(list);
			}
			write_str_c(netou, "Mailbox is empty.\n");
			return 0;
		} else {
			write_str_c(netou, "Unable to open directory.\n");
			return -1;
		}
	} else {
		while (n--) {
			/*
			 * Opening file
			 */
			bfr_in bmsg = {};
			char file[strlen(lstore)+MAX_FLEN+1+20+1];
			strcpy(file, dir);
			strcat(file, "/");
			strcat(file, list[n]->d_name);
			bmsg.filedes = open(file, O_RDONLY, NULL);
			bmsg.typedes = TYPEDES_BLOCK;
			
			if (bmsg.filedes < 0) {
				write_str_c(netou, "Unable to open file.\n");
				return -1;
			}
			
			/*
			 * Initializing buffer
			 */
#if 10 > MAX_OLEN && 10 > MAX_FLEN
			char field[11];
#elif MAX_OLEN > MAX_FLEN
			char field[MAX_OLEN+1];
#else
			char field[MAX_FLEN+1];
			
#endif
			
			/*
			 * Printing separator
			 */
			write_str(netou, "Message #");
			char n_s[11];
			snprintf(n_s, 10, "%d", n+1);
			write_str(netou, n_s);
			write_char(netou, '\n');
			
			/*
			 * Converting timestamp (from filename) to datetime string
			 */
			memcpy(field, list[n]->d_name, 11);
			field[11] = '\0';
			long time_l;
			sscanf(field, "%ld", &time_l);
			struct tm time_m;
			gmtime_r(&time_l, &time_m);
			strftime(field, sizeof(field), "%Y-%m-%d %H:%M:%S %z", &time_m);
			
			/*
			 * Printing date
			 */
			write_str(netou, "Date: ");
			write_str(netou, field);
			write_char(netou, '\n');
			
			/*
			 * Printing sender
			 */
			read_line(&bmsg, field, MAX_FLEN);
			write_str(netou, "From: ");
			write_str(netou, field);
			write_char(netou, '\n');
			
			/*
			 * Printing object
			 */
			read_line(&bmsg, field, MAX_OLEN);
			write_str(netou, "Object: ");
			write_str(netou, field);
			write_char(netou, '\n');
			
			/*
			 * Printing body
			 */
			pipe_buffers(&bmsg, netou);
			
			write_char(netou, '\n');
			
			if (n > 0) {
				write_str(netou, "--------\n");
			}
			
			free(list[n]);
		}
		
		free(list);
		
		write_char(netou, 3);
		flush_buffer(netou);
	}
	
	return 0;
}

int handle_delete(request* req, bfr_ou* netou) {
	int dresult = 0;
	
	/*
	 * Listing directory and removing files
	 */
	char dir[strlen(lstore)+MAX_FLEN+1];
	strcpy(dir, lstore);
	strcat(dir, req->from);
	struct dirent** list;
	int n = scandir(dir, &list, filter_dirs, alphasort);
	if (n < 1) {
		if (n == 0 || errno == ENOENT) {
			if (n == 0) {
				free(list);
			}
			write_str_c(netou, "Mailbox is already empty.\n");
			return 0;
		} else {
			write_str_c(netou, "Unable to open directory.\n");
			return -1;
		}
	} else {
		while (n--) {
			char file[strlen(lstore)+MAX_FLEN+1+20+1];
			strcpy(file, dir);
			strcat(file, "/");
			strcat(file, list[n]->d_name);
			dresult += unlink(file);
			free(list[n]);
		}
		
		free(list);
	}
	
	if (dresult == 0) {
		write_str_c(netou, "Mailbox emptied.\n");
		return 0;
	} else {
		write_str_c(netou, "Unknown error.\n");
		return -1;
	}
}

int handle_license(request* req, bfr_ou* netou) {
	write_str_c(netou, "Copyright (C) 2013, Stefano Tribioli\nBasicMail is free software: you can redistribute it and/or modify it under the terms of the GNU Affero Public License.\nThe source of BasicMail (including complete license text) is available at [http://github.com/sassospicco/basicmail]\n");
	return 0;
}

int filter_dirs(const struct dirent* item) {
	if (
		strcmp(item->d_name, ".") == 0 ||
		strcmp(item->d_name, "..") == 0
			) {
		return 0;
	} else {
		return 1;
	}
}

#endif /* SERVER_ACTIONS_H_ */
