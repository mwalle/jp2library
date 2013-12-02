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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

const char *prog;

void dump_hex(uint8_t *ptr, int len)
{
	while (len--) {
		printf("%02x ", *ptr++);
	}
}

void usage()
{
	printf(
		"%s command [args..]\n"
		"\n"
		"Available commands:\n"
		"\tprint <file>\n"
		"\tmerge <outfile> <infile1> <infile2> ..\n"
		"\tdelete <file> <segnum1> <segnum2> ..\n"
		"\tinsert <file> <segnum> <byte1> <byte2>..\n"
		, prog);
}


int main(int argc, char **argv)
{
	int rc;
	uint8_t csum[2];
	uint8_t expected_csum;
	FILE *f;
	int segno = 0;

	prog = argv[0];

	if (argc < 2) {
		usage();
		return EXIT_FAILURE;
	}

	f = fopen(argv[1], "rb");
	rc = fread(csum, 1, 2, f);
	if (rc != 2) {
		printf("Could not read checksum bytes. File too short?\n");
		return EXIT_FAILURE;
	}

	/* initial value, we expect the checksum to be zero */
	expected_csum = csum[0];

	while (!feof(f)) {
		int i;
		int size;
		uint8_t buf[2048];

		rc = fread(buf, 1, 2, f);
		if (rc != 2) {
			printf("Could not segment length. File too short?\n");
			return EXIT_FAILURE;
		}
		size = (buf[0] << 8) | buf[1];

		if (size == 0 || size == 0xffff) {
			/* we reached the end of the segments */
			break;
		}

		assert(size < sizeof(buf) - 2);
		rc = fread(buf+2, 1, size-2, f);
		if (rc != size-2) {
			printf("Could not read entire segment (%d). "
					"File too short?\n", size);
			return EXIT_FAILURE;
		}

		/* print the segments */
		printf("[%2d] ", segno++);
		dump_hex(buf, size);
		printf("\n");

		/* calculate checksum on the fly */
		for (i=0; i < size; i++) {
			expected_csum ^= buf[i];
		}
	}

	if ((csum[0] ^ csum[1]) != 0xff) {
		printf("checksum bytes are not valid");
	}

	if (expected_csum != 0) {
		printf("checksum does not match (%Xh)", expected_csum);
	} else {
		printf("Checksum is valid\n");
	}

	return 0;
}
