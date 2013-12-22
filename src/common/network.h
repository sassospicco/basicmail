/*
 * network.h
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

#ifndef NETWORK_H_
#define NETWORK_H_

#include "users.h"

#define MAX_OLEN 50
#define DEFAULT_PORT 7474

typedef struct {
	char version[12];
	char hmac[41];
	char command[7];
	char from[MAX_FLEN+1];
	char to[MAX_FLEN+1];
	char object[MAX_OLEN+1];
} request;

typedef struct {
	char version[12];
} response;

#endif /* NETWORK_H_ */
