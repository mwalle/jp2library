/*
 * JP2 remote protocol library.
 *
 * Copyright (c) 2013 Michael Walle <michael@walle.cc>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

int main(int argc, char **argv)
{
	int rc;
	FILE *f;

	f = fopen(argv[1], "rb");
	fseek(f, 2, SEEK_SET);

	while (!feof(f)) {
		int size;
		uint8_t buf[128], *ptr = buf;

		rc = fread(buf, 1, 2, f);
		if (rc != 2) {
			break;
		}
		size = (buf[0] << 8) | buf[1];

		if (size == 0xffff) {
			break;
		}

		assert(size < 126);
		rc = fread(buf+2, 1, size-2, f);
		if (rc != size-2) {
			break;
		}

		while (size--) {
			printf("%02x ", *ptr++);
		}
		printf("\n");
	}

	return 0;
}
