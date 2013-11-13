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

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "jp2library.h"
#include "test.h"

T_DEFS;
static struct jp2_remote *r;

void preload_ack(void)
{
	test_tx("\x00\x02\x00\x02", 4);
}

void test_command_info(void)
{
	int rc;
	uint8_t *rx;

	test_clear_buffers();
	preload_ack();

	rc = jp2_simple_command(r, JP2_CMD_INFO);
	t_assert(rc == 0);

	rx = test_rx(4);
	t_assert(!memcmp(rx, "\x00\x02\x50\x52", 4));
}

void test_command_enter_programming_mode(void)
{
	int rc;
	uint8_t *rx;

	test_clear_buffers();
	preload_ack();

	rc = jp2_simple_command(r, JP2_CMD_ENTER_PROG);
	t_assert(rc == 0);

	rx = test_rx(4);
	t_assert(!memcmp(rx, "\x00\x02\x51\x53", 4));
}

void test_command_exit_programming_mode(void)
{
	int rc;
	uint8_t *rx;

	test_clear_buffers();
	preload_ack();

	rc = jp2_simple_command(r, JP2_CMD_EXIT_PROG);
	t_assert(rc == 0);

	rx = test_rx(4);
	t_assert(!memcmp(rx, "\x00\x02\x52\x50", 4));
}

void test_simple_command_with_error(void)
{
	int rc;

	test_clear_buffers();

	test_tx("\x00\x02\x10\x12", 4);
	rc = jp2_simple_command(r, JP2_CMD_INFO);
	t_assert(rc == -16);
}

void test_simple_command_with_wrong_checksum(void)
{
	int rc;

	test_clear_buffers();

	test_tx("\x00\x02\00\x00", 4);
	rc = jp2_simple_command(r, JP2_CMD_INFO);
	t_assert(rc == -100);
}

void test_connect(void)
{
	int rc;

	test_clear_buffers();

	/* ack to enter programming mode */
	test_tx("\x00\x02\x00\x02", 4);

	/* response to "get information" command */
	test_tx("\x00\x08\x00\x03\x15\x00\x00\xc4\x4e\x94", 10);

	/* signature */
	test_tx("\x00\x2e\x00\x32\x30\x34\x39\x30\x37\x20", 10);
	test_tx("\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20", 10);
	test_tx("\x20\x20\x20\x20\x20\x20\x20\x20\x20\x00", 10);
	test_tx("\x00\x0a\x00\x00\x00\xc4\x4d\x00\x00\xc4", 10);
	test_tx("\x86\x00\x01\xdb\xff\x00\x01\xc3", 8);

	/* ack to exit programming mode */
	test_tx("\x00\x02\x00\x02", 4);
}

int main()
{
	uint8_t txbuf[2048];

	jp2_init();
	test_init();

	r = jp2_open_remote("/dev/null");
	assert(r);

	t_run_test(test_command_info);
	t_run_test(test_command_enter_programming_mode);
	t_run_test(test_command_exit_programming_mode);
	t_run_test(test_simple_command_with_error);
	t_run_test(test_simple_command_with_wrong_checksum);

	return 0;
}
