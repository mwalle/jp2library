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
#include <assert.h>
#include "jp2library.h"

static struct jp2_remote *r;

void usage(const char *prog)
{
	printf("usage: %s <command> [<args>..]\n", prog);
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
	struct jp2_info info;

	jp2_init();
	r = jp2_open_remote("/dev/ttyUSB0");

	jp2_enter_loader(r);

	jp2_get_info(r, &info);

	if (argc < 2) {
		usage(argv[0]);
		return -1;
	}

	if (!strcmp(argv[1], "raw")) {
		rc = cmd_raw(argc, argv);
	}

	jp2_exit_loader(r);

	return rc;
}
