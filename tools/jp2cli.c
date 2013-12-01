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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "jp2library.h"

static struct jp2_remote *r;
static const char *prog;

void usage()
{
	printf(
		"usage: %s <options> <command> ..\n"
		"\n"
		"Available options:\n"
		"\t-D dev  Specify device to use. Default is /dev/ttyUSB0.\n"
		"\t-h      Print this help.\n"
		"\t-v      Be more verbose.\n"
		"\n"
		"Available commands:\n"
		"\tinfo\n"
		"\t        Print information about detected remote.\n"
		"\tread <outfile> <start> <length>\n"
		"\t        Read <length> bytes from remote at offset <start>.\n"
		"\terase <start> <length>\n"
		"\t        Erase the given area. Please not that only whole\n"
		"\t        erase blocks can be erased.\n"
		"\twrite <infile> <start> <length>\n"
		"\t        Write <length> bytes from remote to offset <start>.\n"
		"\traw [bytes..]\n"
		"\t        Send an raw command to the remote.\n"
		, prog);
}

static int cmd_info(int argc, char **argv)
{
	int rc;
	struct jp2_info info;

	rc = jp2_get_info(r, &info);
	if (rc < 0) {
		printf("GET_INFO command failed\n");
		return rc;
	}

	printf("Found remote: %s\n", info.signature);
	printf("Program area: %05x - %05x\n",
			info.program_area_begin, info.program_area_end);
	printf("Protocol area: %05x - %05x\n",
			info.protocol_area_begin, info.protocol_area_end);
	printf("Update area: %05x - %05x\n",
			info.update_area_begin, info.update_area_end);

	return 0;
}

static int cmd_read(int argc, char **argv)
{
	int rc;
	FILE *f;
	int address;
	int length;
	char *endptr;

	if (argc != 4) {
		usage();
		return EXIT_FAILURE;
	}

	address = strtoul(argv[2], &endptr, 0);
	if (*argv[2] != '\0' && *endptr != '\0') {
		printf("could not parse start address\n");
		return -1;
	}

	length = strtoul(argv[3], &endptr, 0);
	if (*argv[3] != '\0' && *endptr != '\0') {
		printf("could not parse length\n");
		return -1;
	}

	f = fopen(argv[1], "wb");
	if (!f) {
		printf("could not open %s: %s", argv[1], strerror(errno));
		return -1;
	}

	while (length > 0) {
		uint8_t buf[128];

		printf("Reading %05Xh\n", address);
		rc = jp2_read_block(r, address, sizeof(buf), buf);
		if (rc < 0) {
			printf("could not read from remote\n");
			break;
		}

		length -= rc;
		address += rc;

		rc = fwrite(buf, rc, 1, f);
		if (rc < 0) {
			printf("could not write to file\n");
			break;
		}
	}

	fclose(f);

	return rc;
}

static int cmd_erase(int argc, char **argv)
{
	return 0;
}

static int cmd_write(int argc, char **argv)
{
	return 0;
}

static int cmd_raw(int argc, char **argv)
{
	uint8_t cmd[16];
	int i;
	uint8_t *rxdata;

	for (i = 0; i < argc-2; i++) {
		cmd[i] = strtoul(argv[i+2], NULL, 0);
	}
	jp2_command(r, cmd, i, &rxdata);

	return 0;
}

int main(int argc, char **argv)
{
	int rc;
	int opt;
	struct jp2_info info;
	const char *dev = "/dev/ttyUSB0";

	prog = argv[0];

	while ((opt = getopt(argc, argv, "D:hv")) != -1) {
		switch (opt) {
		case 'D':
			dev = optarg;
			break;
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case 'v':
			setenv("JP2_DEBUG", "1", 1);
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
		}
	}

	if (optind >= argc) {
		usage();
		exit(EXIT_FAILURE);
	}

	jp2_init();
	r = jp2_open_remote(dev);

	jp2_enter_loader(r);

	jp2_get_info(r, &info);

	if (!strcmp(argv[optind], "info")) {
		rc = cmd_info(argc - optind, argv + optind);
	} else if (!strcmp(argv[optind], "read")) {
		rc = cmd_read(argc - optind, argv + optind);
	} else if (!strcmp(argv[optind], "erase")) {
		rc = cmd_erase(argc - optind, argv + optind);
	} else if (!strcmp(argv[optind], "write")) {
		rc = cmd_write(argc - optind, argv + optind);
	} else if (!strcmp(argv[optind], "raw")) {
		rc = cmd_raw(argc - optind, argv + optind);
	}

	jp2_exit_loader(r);

	return rc;
}
