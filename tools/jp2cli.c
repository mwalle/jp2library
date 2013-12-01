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
		"\twrite <infile> <address>\n"
		"\t        Write to offset <address>.\n"
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
			printf("could not read from remote (%d)\n", rc);
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
	int rc;
	int address;
	int length;
	char *endptr;

	if (argc != 3) {
		usage();
		return EXIT_FAILURE;
	}

	address = strtoul(argv[1], &endptr, 0);
	if (*argv[1] != '\0' && *endptr != '\0') {
		printf("could not parse start address\n");
		return -1;
	}

	length = strtoul(argv[2], &endptr, 0);
	if (*argv[2] != '\0' && *endptr != '\0') {
		printf("could not parse length\n");
		return -1;
	}

	rc = jp2_erase_block(r, address, address + length);
	if (rc < 0) {
		printf("could not erase block (%d)\n", rc);
		return -1;
	}

	return 0;
}

static int cmd_write(int argc, char **argv)
{
	int rc;
	int address;
	char *endptr;
	FILE *f;

	if (argc != 3) {
		usage();
		return EXIT_FAILURE;
	}

	address = strtoul(argv[2], &endptr, 0);
	if (*argv[2] != '\0' && *endptr != '\0') {
		printf("could not parse address\n");
		return -1;
	}

	f = fopen(argv[1], "rb");
	if (!f) {
		printf("could not open %s: %s", argv[1], strerror(errno));
		return -1;
	}

	while (!feof(f)) {
		uint8_t buf[128];
		rc = fread(buf, 1, sizeof(buf), f);
		if (rc < 0) {
			printf("could not read from file: %s\n", strerror(errno));
			return -1;
		}

		rc = jp2_write_block(r, address, rc, buf);
		if (rc < 0) {
			printf("could not write to the remote (%d)\n", rc);
			return -1;
		}

		address += rc;
	}

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
	bool o_noenter = false;
	bool o_noleave = false;

	prog = argv[0];

	while ((opt = getopt(argc, argv, "D:hvLE")) != -1) {
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
		case 'E':
			o_noenter = true;
			break;
		case 'L':
			o_noleave = true;
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

	if (!o_noenter) {
		jp2_enter_loader(r, true);
	}

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

	if (!o_noleave) {
		jp2_exit_loader(r);
	}

	return rc;
}
