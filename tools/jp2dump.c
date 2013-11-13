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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "jp2library.h"

void usage(const char *prog)
{
	printf("usage: %s <ttydev> <outfile> <start offset> <length>\n", prog);
}

int main(int argc, char **argv)
{
	int rc;
	uint32_t start;
	uint32_t length;
	char *endptr;
	FILE *f;
	uint32_t addr;
	uint8_t *data;
	static struct jp2_remote *r;

	if (argc < 5) {
		usage(argv[0]);
		return 1;
	}

	start = strtoul(argv[3], &endptr, 0);
	if (*endptr != 0) {
		usage(argv[0]);
		return 1;
	}

	length = strtoul(argv[4], &endptr, 0);
	if (*endptr != 0) {
		usage(argv[0]);
		return 1;
	}

	/* try normal read first */
	f = fopen(argv[2], "w");
	if (!f) {
		fprintf(stderr, "Could not open output file: %s",
			strerror(errno));
		return 2;
	}

	jp2_init();
	r = jp2_open_remote(argv[1]);
	if (!r) {
		fprintf(stderr, "Could not open remote: %s", strerror(errno));
		return 3;
	}

	jp2_enter_loader(r);

	data = malloc(length);
	assert(data);

	rc = jp2_read_block(r, start, length, data);
	if (rc > 0) {
		fwrite(data, 1, length, f);
		goto out;
	}
	
	/* didn't work out, try using checksum method */
	printf("\nRead command returned error code. Trying alternative method.\n");
	for (addr = start; addr < start + length; addr++) {
		uint8_t b;

		if ((addr & 0xff) == 0) {
			printf("\rDumping address %05Xh..", addr);
			fflush(stdout);
		}
		rc = jp2_checksum_block(r, addr, addr);
		if (rc < 0) {
			break;
		}
		b = rc & 0xff;
		fwrite(&b, 1, 1, f);
	}

out:
	fclose(f);
	jp2_exit_loader(r);

	if (rc < 0) {
		printf("\nDump failed.\n");
	} else {
		printf("\nDump successful.\n");
	}

	return (rc < 0) ? 4 : 0;
}
