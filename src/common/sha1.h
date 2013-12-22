/*
 * sha1.h
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

#ifndef SHA1_H_
#define SHA1_H_

#include <stddef.h>
#include <string.h>

#define SHA1_RAW_LEN 20
#define SHA1_HEX_LEN 40

#include "sha1.polarssl.h"

void sha1_to_hex(unsigned char raw[20], char* hex) {
	char map[] = "0123456789abcdef";
	int i;
	for (i = 0; i < 20; i++) {
		hex[i*2] = map[(unsigned int) ((raw[i] >> 4) & 0x0F)];
		hex[i*2+1] = map[(unsigned int) (raw[i] & 0x0F)];
	}
	hex[40] = '\0';
}

void sha1_hex(const unsigned char* input, size_t ilen, char output[41]) {
	unsigned char raw[20];
	sha1(input, ilen, raw);
	sha1_to_hex(raw, output);
}

int sha1_hmac_finish_check(sha1_context *ctx, char against[41]) {
	unsigned char raw[20];
	char hex[41];
	sha1_hmac_finish(ctx, raw);
	sha1_to_hex(raw, hex);
	if (strcmp(hex, against) == 0) {
		return 0;
	} else {
#ifdef DEBUG
		printf("Expected HMAC: %s\n", hex);
#endif
		return -1;
	}
}

#endif /* SHA1_H_ */
