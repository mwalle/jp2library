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
#include <stdbool.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <dirent.h>
#include <string.h>

#include "osapi.h"

typedef enum {
	STATE_BLOCKING,
	STATE_NON_BLOCKING,
} state_t;

struct osapi_linux_data {
	int fd;
	struct termios oldtio;
	int flags;
	state_t state;
};

/* non-reentrant! */
static const char *_enumerate_remote(void)
{
	static DIR *dir = NULL;
	struct dirent *dirent;
	static char buf[NAME_MAX + 1];

	if (dir == NULL) {
		dir = opendir("/dev");
		assert(dir);
	}

	while ((dirent = readdir(dir)))
	{
		if (!strncmp(dirent->d_name, "ttyS", 4)) {
			break;
		}
		if (!strncmp(dirent->d_name, "ttyUSB", 6)) {
			break;
		}
	}

	if (dirent == NULL) {
		closedir(dir);
		dir = NULL;
		return NULL;
	}

	strcpy(buf, "/dev/");
	strcpy(buf + 5, dirent->d_name);

	return buf;
}

static void *_open_remote(const char *devname, int flags)
{
	int rc;
	struct osapi_linux_data *d;
	struct termios tio;

	d = malloc(sizeof(*d));
	d->fd = open(devname, O_RDWR);
	if (d->fd < 0) {
		perror("open()");
		return NULL;
	}

	rc = tcgetattr(d->fd, &d->oldtio);
	if (rc < 0) {
		perror("tcgetattr()");
		free(d);
		return NULL;
	}

	tio.c_cflag = B38400 | CS8 | CLOCAL | CREAD;
	tio.c_iflag = IGNPAR | IGNBRK;
	tio.c_oflag = 0;
	tio.c_lflag = 0;
	tio.c_cc[VMIN]=1;
	tio.c_cc[VTIME]=0;
	tcflush(d->fd, TCIFLUSH);

	rc = tcsetattr(d->fd, TCSANOW, &tio);
	if (rc < 0) {
		perror("tcgetattr()");
		free(d);
		return NULL;
	}

	d->state = STATE_BLOCKING;
	d->flags = fcntl(d->fd, F_GETFL, 0);
	if (d->flags < 0) {
		perror("fcntl()");
		free(d);
		return NULL;
	}

	return d;
}

static int _flush_remote(void *handle)
{
	struct osapi_linux_data *d = handle;
	return tcflush(d->fd, TCIFLUSH);
}

static void _close_remote(void *handle)
{
	struct osapi_linux_data *d = handle;
	tcsetattr(d->fd, TCSANOW, &d->oldtio);
	close(d->fd);
	free(d);
}

static int _reset_remote(void *handle, bool assert_pin)
{
	struct osapi_linux_data *d = handle;
	int rc;
	int status;

	rc = ioctl(d->fd, TIOCMGET, &status);
	if (rc) {
		return -1;
	}

	if (assert_pin) {
		status |= TIOCM_RTS;
	} else {
		status &= ~TIOCM_RTS;
	}

	rc = ioctl(d->fd, TIOCMSET, &status);
	if (rc) {
		return -1;
	}

	return 0;
}

/* Either we read exactly the requested bytes or return with an error. Eg.
 * we don't allow short reads. */
static ssize_t _read_remote(void *handle, void *buf, size_t count)
{
	int rc;
	struct osapi_linux_data *d = handle;
	size_t bytes_read = 0;

	if (d->state == STATE_NON_BLOCKING) {
		rc = fcntl(d->fd, F_SETFL, d->flags);
		if (rc < 0) {
			return rc;
		}
		d->state = STATE_BLOCKING;
	}

	while (bytes_read < count) {
		rc = read(d->fd, buf + bytes_read, count - bytes_read);
		if (rc < 0) {
			return rc;
		}
		bytes_read += rc;
	}
	return bytes_read;
}

static ssize_t _read_nonblock_remote(void *handle, void *buf, size_t count)
{
	int rc;
	struct osapi_linux_data *d = handle;
	size_t bytes_read = 0;

	if (d->state == STATE_BLOCKING) {
		rc = fcntl(d->fd, F_SETFL, d->flags | O_NONBLOCK);
		if (rc < 0) {
			return rc;
		}
		d->state = STATE_NON_BLOCKING;
	}

	while (bytes_read < count) {
		rc = read(d->fd, buf + bytes_read, count - bytes_read);
		if (rc < 0) {
			return rc;
		}
		bytes_read += rc;
	}
	return bytes_read;
}

static ssize_t _write_remote(void *handle, void *buf, size_t count)
{
	int rc;
	struct osapi_linux_data *d = handle;

	if (d->state == STATE_NON_BLOCKING) {
		rc = fcntl(d->fd, F_SETFL, d->flags);
		if (rc < 0) {
			return rc;
		}
		d->state = STATE_BLOCKING;
	}

	return write(d->fd, buf, count);
}

static struct osapi_ops linux_ops = {
	.enumerate = _enumerate_remote,
	.open = _open_remote,
	.close = _close_remote,
	.flush = _flush_remote,
	.reset = _reset_remote,
	.read = _read_remote,
	.read_nonblock = _read_nonblock_remote,
	.write = _write_remote,
};

struct osapi_ops *osapi = &linux_ops;
