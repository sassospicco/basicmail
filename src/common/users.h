/*
 * users.h
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

#ifndef USERS_H_
#define USERS_H_

#define MAX_USRS 100
#define MAX_FLEN 20
typedef struct {
	int id;
	char name[MAX_FLEN +1];
	char password[MAX_FLEN +1];
} user;

#endif /* USERS_H_ */
