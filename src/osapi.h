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

#ifndef __OSAPI_H
#define __OSAPI_H

#include <unistd.h>

struct osapi_ops {
	const char *(*enumerate)(void);
	void *(*open)(const char *devname, int flags);
	void (*close)(void *handle);
	int (*reset)(void *handle, bool assert_pin);
	int (*flush)(void *handle);
	ssize_t (*read)(void *handle, void *buf, size_t count);
	ssize_t (*read_nonblock)(void *handle, void *buf, size_t count);
	ssize_t (*write)(void *handle, void *buf, size_t count);
};

extern struct osapi_ops *osapi;

#endif /* __OSAPI_H */
