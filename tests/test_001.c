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
#include <string.h>

#include "jp2library.h"
#include "test.h"

T_DEFS;
static struct jp2_remote *r;

void preload_ack(void)
{
	test_tx_s("\x00\x02\x00\x02", 4);
}

void test_command_info(void)
{
	int rc;
	uint8_t *rx;

	test_clear_buffers();
	preload_ack();

	rc = jp2_simple_command(r, JP2_CMD_INFO);
	t_assert(rc == JP2_ERR_NO_ERR);

	rx = test_rx(4);
	t_assert(!memcmp(rx, "\x00\x02\x50\x52", 4));
}

void test_command_enter_programming_mode(void)
{
	int rc;
	uint8_t *rx;

	test_clear_buffers();
	preload_ack();

	rc = jp2_simple_command(r, JP2_CMD_ENTER_LOADER);
	t_assert(rc == JP2_ERR_NO_ERR);

	rx = test_rx(4);
	t_assert(!memcmp(rx, "\x00\x02\x51\x53", 4));
}

void test_command_exit_programming_mode(void)
{
	int rc;
	uint8_t *rx;

	test_clear_buffers();
	preload_ack();

	rc = jp2_simple_command(r, JP2_CMD_EXIT_LOADER);
	t_assert(rc == JP2_ERR_NO_ERR);

	rx = test_rx(4);
	t_assert(!memcmp(rx, "\x00\x02\x52\x50", 4));
}

void test_simple_command_with_error(void)
{
	int rc;

	test_clear_buffers();

	test_tx_s("\x00\x02\x10\x12", 4);
	rc = jp2_simple_command(r, JP2_CMD_INFO);
	t_assert(rc == -16);
}

void test_simple_command_with_wrong_checksum(void)
{
	int rc;

	test_clear_buffers();

	test_tx_s("\x00\x02\00\x00", 4);
	rc = jp2_simple_command(r, JP2_CMD_INFO);
	t_assert(rc == -JP2_ERR_WRONG_CHECKSUM);
}

void test_connect_16bit(void)
{
	int rc;
	struct jp2_info info;
	uint8_t *rx;

	test_clear_buffers();

	/* ack to enter bootloader */
	test_tx_s("\x00\x02\x00\x02", 4);

	/* response to "get information" command */
	test_tx_s("\x00\x06\x00\x03\x15\xc4\x4e\x9a", 8);

	/* signature */
	test_tx_s("\x00\x28\x00\x33\x32\x32\x34\x30\x33\x42", 10);
	test_tx_s("\x56\x20\x4f\x46\x41\x20\x49\x6e\x66\x20", 10);
	test_tx_s("\x20\x20\x20\x20\x20\x20\x20\x20\x20\x05", 10);
	test_tx_s("\x00\x4b\x7f\x4b\x80\xdf\xff\xe0\x00\xef", 10);
	test_tx_s("\xff\x1b", 2);

	/* ack to exit bootloader */
	test_tx_s("\x00\x02\x00\x02", 4);

	rc = jp2_enter_loader(r);
	t_assert(rc == -JP2_ERR_NO_ERR);
	rx = test_rx(4);
	t_assert(!memcmp(rx, "\x00\x02\x51\x53", 4));

	rc = jp2_get_info(r, &info);
	rx = test_rx(4);
	t_assert(!memcmp(rx, "\x00\x02\x50\x52", 4));
	rx = test_rx(8);
	t_assert(!memcmp(rx, "\x00\x06\x01\xc4\x4e\x00\x26\xab", 8));

	t_assert(rc == -JP2_ERR_NO_ERR);
	t_assert(info.id == 0x0315);
	t_assert(!strcmp(info.signature, "322403BV OFA Inf          "));
	t_assert(info.program_area_begin == 0x500);
	t_assert(info.program_area_end == 0x4b7f);
	t_assert(info.protocol_area_begin == 0x4b80);
	t_assert(info.protocol_area_end == 0xdfff);
	t_assert(info.update_area_begin == 0xe000);
	t_assert(info.update_area_end == 0xefff);

	rc = jp2_exit_loader(r);
	t_assert(rc == -JP2_ERR_NO_ERR);
	rx = test_rx(4);
	t_assert(!memcmp(rx, "\x00\x02\x52\x50", 4));
}

void test_connect_32bit(void)
{
	int rc;
	struct jp2_info info;
	uint8_t *rx;

	test_clear_buffers();

	/* ack to enter bootloader */
	test_tx_s("\x00\x02\x00\x02", 4);

	/* response to "get information" command */
	test_tx_s("\x00\x08\x00\x03\x15\x00\x00\xc4\x4e\x94", 10);

	/* signature */
	test_tx_s("\x00\x34\x00\x32\x30\x34\x39\x30\x37\x20", 10);
	test_tx_s("\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20", 10);
	test_tx_s("\x20\x20\x20\x20\x20\x20\x20\x20\x20\x00", 10);
	test_tx_s("\x00\x0a\x00\x00\x00\xc4\x4d\x00\x00\xc4", 10);
	test_tx_s("\x86\x00\x01\xdb\xff\x00\x01\xdc\x00\x00", 10);
	test_tx_s("\x01\xff\xff\x04", 4);

	/* ack to exit bootloader */
	test_tx_s("\x00\x02\x00\x02", 4);

	rc = jp2_enter_loader(r);
	t_assert(rc == -JP2_ERR_NO_ERR);
	rx = test_rx(4);
	t_assert(!memcmp(rx, "\x00\x02\x51\x53", 4));

	rc = jp2_get_info(r, &info);
	rx = test_rx(4);
	t_assert(!memcmp(rx, "\x00\x02\x50\x52", 4));
	rx = test_rx(10);
	t_assert(!memcmp(rx, "\x00\x08\x01\x00\x00\xc4\x4e\x00\x32\xb1", 10));

	t_assert(rc == -JP2_ERR_NO_ERR);
	t_assert(info.id == 0x0315);
	t_assert(!strcmp(info.signature, "204907                    "));
	t_assert(info.program_area_begin == 0xa00);
	t_assert(info.program_area_end == 0xc44d);
	t_assert(info.protocol_area_begin == 0xc486);
	t_assert(info.protocol_area_end == 0x1dbff);
	t_assert(info.update_area_begin == 0x1dc00);
	t_assert(info.update_area_end == 0x1ffff);

	rc = jp2_exit_loader(r);
	t_assert(rc == -JP2_ERR_NO_ERR);
	rx = test_rx(4);
	t_assert(!memcmp(rx, "\x00\x02\x52\x50", 4));
}

int main()
{
	jp2_init();
	test_init();

	r = jp2_open_remote("/dev/null");
	assert(r);

	t_run_test(test_command_info);
	t_run_test(test_command_enter_programming_mode);
	t_run_test(test_command_exit_programming_mode);
	t_run_test(test_simple_command_with_error);
	t_run_test(test_simple_command_with_wrong_checksum);
	t_run_test(test_connect_16bit);
	t_run_test(test_connect_32bit);

	return 0;
}
