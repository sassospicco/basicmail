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

/*
 * Length of buffer
 */
#define BFR_LEN 1024
#define TYPEDES_BLOCK 0
#define TYPEDES_STREAM 1

/**
 * The following struct is passed to each of the "read" functions defined here.
 * It represents an I/O buffer, hosting the buffer itself, the file descriptor
 * value and two key fields:
 * valid is -1 if the last operation failed, otherwise it holds the number of
 * valid, readable bytes in the buffer;
 * index is the offset at which the next byte will be read.
 */

typedef struct {
	int filedes;
	int typedes;
	char bfr[BFR_LEN];
	int valid;
	int index;
} bfr_in;

/**
 * The following struct is passed to each of the "write" functions defined here.
 * It represents an I/O buffer, hosting the buffer itself, the file descriptor
 * value and one key field:
 * index is the offset at which the next byte will be written.
 */
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

/**
 * Reading a single char.
 * This function returns the char directly. An error can be detected checking
 * the passed struct's "valid" field for -1.
 */
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

/**
 * Reading a line.
 * The line will be placed in passed array to, of size to_size. If passed array
 * is too short, the line will be truncated. Either case, a null char is duly
 * appended.
 * Returns -1 in case of error, the length of line (excluding null char)
 * otherwise.
 */
int read_line(bfr_in* from, char* to, int to_size) {
	int index = 0;
	
	do {
		char curr = read_char(from);
		
		if (from->valid < 0) {
			return -1;
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

/**
 * Skipping to next message.
 * This function will read and discard characters until a 0x3 guard char is
 * found.
 * Returns -1 in case of error, the length of discarded segment otherwise.
 */
int consume_garbage(bfr_in* from) {
	int counter = 0;
	
	do {
		char curr = read_char(from);
		counter++;
		
		if (from->valid < 0) {
			return -1;
		}
		
		if (curr == 3) {
			return counter;
		}
	} while (from->valid > 0);
	
	return counter;
}

/**
 * Writing a single byte to buffer.
 * A single byte is written, but the buffer itself is kept and not flushed.
 * Returns -1 in case of error, 0 otherwise.
 */
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
				offset = offset + written;
			}
		}
		
		bfr->index = 0;
	}
	
	bfr->bfr[bfr->index] = curr;
	(bfr->index)++;
	
	return 0;
}

/**
 * Writing an array of bytes to buffer.
 * Handy shortcut to write an array of arbitrary size. It calls write_char for
 * every char in passed array. Size must be provided.
 * Returns -1 in case of error, 0 otherwise.
 */
int write_array(bfr_ou* bfr, char* from, int from_size) {
	int i;
	for (i = 0; i < from_size; i++) {
		if (write_char(bfr, from[i]) < 0) {
			return -1;
		}
	}
	
	return 0;
}

/**
 * Writing a null-terminated string to buffer.
 * Handy shortcut to write a string of arbitrary size. It calls write_char for
 * every char in passed string. Size is detected with strlen.
 * Returns -1 in case of error, 0 otherwise.
 */
int write_str(bfr_ou* bfr, char* from) {
	return write_array(bfr, from, strlen(from));
}

/**
 * Writing a null-terminated string and a 0x3 to file.
 * Handy shortcut to write a string of arbitrary size, send a 0x3 guard char
 * (separating messages) and flush buffer.
 */
void write_str_c(bfr_ou* bfr, char* from) {
	write_array(bfr, from, strlen(from));
	write_char(bfr, 3);
	flush_buffer(bfr);
}

/**
 * Writing a null-terminated string and a 0x4 to file.
 * Handy shortcut to write a string of arbitrary size, send a 0x4 guard char
 * (closing connection), flush buffer and close file.
 */
void write_str_d(bfr_ou* bfr, char* from) {
	write_array(bfr, from, strlen(from));
	write_char(bfr, 4);
	flush_buffer(bfr);
	close(bfr->filedes);
}

/**
 * Flushing buffer, writing its content to file.
 * Returns -1 in case of error, 0 otherwise.
 */
int flush_buffer(bfr_ou* bfr) {
	int offset = 0;
	while (offset < bfr->index) {
		int written = write(bfr->filedes, (bfr->bfr)+offset, (bfr->index)-offset);
		if (written < 0) {
			return -1;
		} else {
			offset = offset + written;
		}
	}
	
	bfr->index = 0;
	
	return 0;
}

/**
 * Copying data from one buffer to another until a guard is found.
 * Valid guards are EOF condition; \0x0, \0x3 and \0x4 chars; error condition.
 * Returns -1 in case of (reading) error, 0 if EOF is reached, guard value if
 * a guard character is found. 
 */
int pipe_buffers(bfr_in* from, bfr_ou* to) {
	do {
		char curr = read_char(from);

		if (from->valid < 0) {
			flush_buffer(to);
			return -1;
		} else if (curr == 0 || curr == 3 || curr == 4) {
			flush_buffer(to);
			return curr;
		} else {
			write_char(to, curr);
		}
	} while (from->valid > 0);
	
	flush_buffer(to);
	return 0;
}

#endif /* BUFFERED_IO_H_ */
