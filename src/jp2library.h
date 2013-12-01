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

#ifndef __JP2LIBRARY_H
#define __JP2LIBRARY_H

#include <stdint.h>
#include <stdbool.h>

struct jp2_remote;

#define JP2_SIGNATURE_LEN 26

enum {
	JP2_CMD_READ = 0x01,		/* address, length */
	JP2_CMD_WRITE = 0x02,		/* address, data */
	JP2_CMD_ERASE = 0x03,		/* start, end address */
	JP2_CMD_CHECKSUM = 0x04,	/* start address, end address */
	JP2_CMD_INFO = 0x50,		/* no arguments */
	JP2_CMD_ENTER_LOADER = 0x51,	/* no arguments */
	JP2_CMD_EXIT_LOADER = 0x52,	/* no arguments */
};

enum {
	JP2_ERR_NO_ERR = 0x00,
	JP2_ERR_UNKNOWN_COMMAND = 0x01,
	JP2_ERR_WRONG_CHECKSUM = 0x02,
	JP2_ERR_INVALID_ARGUMENT = 0x03,
	JP2_ERR_DATA_UNALIGNED = 0x04,	/* returned by write command if data
					   bytes are not a multiple of two */
	JP2_ERR_UNSUPPORTED = 0x100,
};

struct jp2_info {
	uint16_t id;			/* loader version? */
	char signature[JP2_SIGNATURE_LEN + 1];
	uint32_t program_area_begin;
	uint32_t program_area_end;
	uint32_t protocol_area_begin;
	uint32_t protocol_area_end;
	uint32_t update_area_begin;
	uint32_t update_area_end;
};

extern const char* jp2_version;

int jp2_init(void);
struct jp2_remote *jp2_open_remote(const char *devname);
void jp2_close_remote(struct jp2_remote *r);

int jp2_simple_command(struct jp2_remote *r, const uint8_t cmd);
int jp2_command(struct jp2_remote *r, const uint8_t *txdata, int txlen,
	uint8_t **rxdata);

/* specific commands */
int jp2_read_block(struct jp2_remote *r, uint32_t address, uint16_t len,
		uint8_t *data);
int jp2_erase_block(struct jp2_remote *r, uint32_t start, uint32_t end);
int jp2_write_block(struct jp2_remote *r, uint32_t address, uint16_t len,
		uint8_t *data);
int jp2_checksum_block(struct jp2_remote *r, uint32_t start, uint32_t end);
int jp2_get_info(struct jp2_remote *r, struct jp2_info *info);
int jp2_enter_loader(struct jp2_remote *r, bool extended_mode);
int jp2_exit_loader(struct jp2_remote *r);

#endif /* __JP2LIBRARY_H */
