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

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "osapi.h"

static int dummy_handle;
static uint8_t *_ut_rxbuf;
static uint8_t *_ut_rxptr_p;
static uint8_t *_ut_rxptr_c;
static uint8_t *_ut_txbuf;
static uint8_t *_ut_txptr_p;
static uint8_t *_ut_txptr_c;

static void *_open_remote(const char *devname, int flags)
{
	return &dummy_handle;
}

static void _close_remote(void *handle)
{
	assert(handle == &dummy_handle);
}

static int _reset_remote(void *handle, bool assert_pin)
{
	assert(handle == &dummy_handle);
	return 0;
}

static ssize_t _read_remote(void *handle, void *buf, size_t count)
{
	assert(handle == &dummy_handle);
	assert(_ut_txptr_p);

	memcpy(buf, _ut_rxptr_c, count);
	_ut_rxptr_c += count;

	return count;
}

static ssize_t _write_remote(void *handle, void *buf, size_t count)
{
	assert(handle == &dummy_handle);
	assert(_ut_txptr_p);

	memcpy(_ut_txptr_p, buf, count);
	_ut_txptr_p += count;

	return count;
}

struct jp2_remote_ops test_ops = {
	.open = _open_remote,
	.close = _close_remote,
	.reset = _reset_remote,
	.read = _read_remote,
	.write = _write_remote,
};

void test_clear_buffers()
{
	_ut_rxptr_p = _ut_rxbuf;
	_ut_rxptr_c = _ut_rxbuf;
	_ut_txptr_p = _ut_txbuf;
	_ut_txptr_c = _ut_txbuf;
}

void test_init()
{
	jp2_set_default_remote_ops(&test_ops);

	_ut_rxbuf = malloc(2048);
	_ut_txbuf = malloc(2048);
	test_clear_buffers();
}

uint8_t *test_rx(int len)
{
	uint8_t *ptr = _ut_txptr_c;
	_ut_txptr_c += len;
	return ptr;
}

void test_tx(uint8_t *data, int len)
{
	memcpy(_ut_rxptr_p, data, len);
	_ut_rxptr_p += len;
}
