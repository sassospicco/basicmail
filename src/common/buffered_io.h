/*
 * buffered_io.h
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

#ifndef BUFFERED_IO_H_
#define BUFFERED_IO_H_

#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BFR_LEN 1024
#define TYPEDES_BLOCK 0
#define TYPEDES_STREAM 1
#define BFR_FLUSH 1

typedef struct {
	int filedes;
	int typedes;
	char bfr[BFR_LEN];
	int valid;
	int index;
} bfr_in;

typedef struct {
	int filedes;
	int typedes;
	char bfr[BFR_LEN];
	int index;
} bfr_ou;

char read_char(bfr_in*);
int read_line(bfr_in*, char*, int);
int consume_garbage(bfr_in*);
int write_char(bfr_ou*, char);
int write_array(bfr_ou*, char*, int);
int write_str(bfr_ou*, char*);
int flush_buffer(bfr_ou*);
int pipe_buffers(bfr_in*, bfr_ou*);

char read_char(bfr_in* bfr) {
	if (bfr->index >= bfr->valid) {
		bfr->index = 0;
		
		if (bfr->typedes == TYPEDES_BLOCK) {
			bfr->valid = read(bfr->filedes, bfr->bfr, BFR_LEN);
		} else if (bfr->typedes == TYPEDES_STREAM) {
			bfr->valid = recv(bfr->filedes, bfr->bfr, BFR_LEN, 0);
		} else {
			bfr->valid = -1;
		}
		
		if (bfr->valid <= 0) {
			return '\0';
		}
	}
	
	char curr = bfr->bfr[bfr->index];
	(bfr->index)++;
	return curr;
}

int read_line(bfr_in* from, char* to, int to_size) {
	int index = 0;
	
	do {
		char curr = read_char(from);
		
		if (from->valid < 0) {
			return from->valid;
		}
		
		if (curr == '\n') {
			to[index] = '\0';
			return index;
		} else if (index < to_size) {
			to[index++] = curr;
		} else {
			to[to_size-1] = '\0';
			index = to_size-1;
		}
	} while (from->valid > 0);
	
	return index;
}

int consume_garbage(bfr_in* from) {
	int guard = 1;
	int counter = 0;
	
	do {
		char curr = read_char(from);
		counter++;
		
		if (from->valid < 0) {
			return -1;
		}
		
		if (curr == '\n') {
			if (guard == 1) {
				return counter;
			} else {
				guard = 1;
			}
		} else {
			guard = 0;
		}
	} while (from->valid > 0);
	
	return counter;
}

int write_char(bfr_ou* bfr, char curr) {
	if (bfr->index >= BFR_LEN) {
		int offset = 0;
		while (offset < BFR_LEN) {
			int written;			
			if (bfr->typedes == TYPEDES_BLOCK) {
				written = write(bfr->filedes, (bfr->bfr)+offset, BFR_LEN-offset);
			} else if (bfr->typedes == TYPEDES_STREAM) {
				written = send(bfr->filedes, (bfr->bfr)+offset, BFR_LEN-offset, 0);
			} else {
				written = -1;
			}
			
			if (written < 0) {
				return -1;
			} else {
#ifdef DEBUG
			printf("Written %d\n", written);
#endif
				offset = offset + written;
			}
		}
		
		bfr->index = 0;
	}
	
	bfr->bfr[bfr->index] = curr;
	(bfr->index)++;
	
	return 0;
}

int write_array(bfr_ou* bfr, char* from, int from_size) {
	int i;
	for (i = 0; i < from_size; i++) {
		if (write_char(bfr, from[i]) < 0) {
			return -1;
		}
	}
	
	return 0;
}

int write_str(bfr_ou* bfr, char* from) {
	return write_array(bfr, from, strlen(from));
}

int fwrite_str(bfr_ou* bfr, char* from) {
	int wres = write_array(bfr, from, strlen(from));
	write_char(bfr, 4);
	flush_buffer(bfr);
	return wres;
}

int flush_buffer(bfr_ou* bfr) {
	int offset = 0;
	while (offset < bfr->index) {
		int written = write(bfr->filedes, (bfr->bfr)+offset, (bfr->index)-offset);
		if (written < 0) {
			return -1;
		} else {
#ifdef DEBUG
			printf("Written %d\n", written);
#endif
			offset = offset + written;
		}
	}
	
	bfr->index = 0;
	
	return 0;
}

int pipe_buffers(bfr_in* from, bfr_ou* to) {
	int counter = 0;
	
	do {
		char curr = read_char(from);

		if (from->valid < 0 || curr == 0 || curr == 4) {
			flush_buffer(to);
			return counter;
		} else {
			write_char(to, curr);
			counter++;
		}
	} while (from->valid > 0);
	
	flush_buffer(to);
	return counter;
}

#endif /* BUFFERED_IO_H_ */
