/*
 * mutex.h
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

#ifndef SERVER_MUTEX_H_
#define SERVER_MUTEX_H_

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>

int read_count = 0;
/*
 * Set of 3 semaphores:
 * [0] is held while looking up and editing read_count;
 * [1] is held while a thread (or more) is reading or writing;
 * [2] is held while a thread writing or waiting for writing, it
 * 		is also briefly held by threads willing to read, in order
 * 		to prevent starvation of writing threads.
 */
int sems;

void setup_sem() {
	sems = semget(IPC_PRIVATE, 3, S_IRWXU);
	
	if (
			sems < 0 ||
			semctl(sems, 0, SETVAL, 1) < 0 ||
			semctl(sems, 1, SETVAL, 1) < 0 ||
			semctl(sems, 2, SETVAL, 1) < 0
			) {
		printf("Failed to setup thread synchronization.\n");
		exit(EXIT_FAILURE);
	}
}

void read_lock() {
	struct sembuf acq2 = {2, -1, 1};
	semop(sems, &acq2, 1);
	
	struct sembuf acq0 = {0, -1, 1};
	semop(sems, &acq0, 1);
	
	if (read_count == 0) {
		struct sembuf acq1 = {1, -1, 1};
		semop(sems, &acq1, 1);
	}
	
	read_count++;
	
	struct sembuf rel0 = {0, +1, 1};
	semop(sems, &rel0, 1);
	
	struct sembuf rel2 = {2, +1, 1};
	semop(sems, &rel2, 1);
}

void read_unlock() {
	struct sembuf acq0 = {0, -1, 1};
	semop(sems, &acq0, 1);
	
	read_count--;
	
	if (read_count == 0) {
		struct sembuf rel1 = {1, +1, 1};
		semop(sems, &rel1, 1);
	}
	
	struct sembuf rel0 = {0, +1, 1};
	semop(sems, &rel0, 1);
}

void write_lock() {
	struct sembuf acq2 = {2, -1, 1};
	semop(sems, &acq2, 1);
	
	struct sembuf acq1 = {1, -1, 1};
	semop(sems, &acq1, 1);
}

void write_unlock() {
	struct sembuf rel1 = {1, +1, 1};
	semop(sems, &rel1, 1);
	
	struct sembuf rel2 = {2, +1, 1};
	semop(sems, &rel2, 1);
}

#endif /* SERVER_MUTEX_H_ */
