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
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "osapi.h"
#include "jp2library.h"

struct jp2_remote {
	void *handle; /* opaque to this library */
	uint8_t txbuf[2048];
	uint8_t rxbuf[2048];
	int addr_width;
};

#define JP2_CHUNK_SIZE 128

#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif

const char* jp2_version = "0.2" GIT_VERSION;
static int debug_level = 0;

static char *hexdump(uint8_t *data, int len)
{
	char *buf, *ptr;
	ptr = buf = malloc(len * 3 + 1);

	while (len--) {
		ptr += sprintf(ptr, "%02x ", *data++);
	}

	return buf;
}

static int debug(int lvl, const char *fmt, ...)
{
	int rc;
	va_list ap;

	if (lvl <= debug_level) {
		va_start(ap, fmt);
		rc = vfprintf(stderr, fmt, ap);
		va_end(ap);
	}

	return rc;
}

/*
 * Helper functions
 */

static inline uint16_t read_u16_from_buf(uint8_t **ptr)
{
	uint16_t val;

	assert(ptr);

	val = *(*ptr)++; val <<= 8;
	val |= *(*ptr)++;

	return val;
}

static inline uint32_t read_u32_from_buf(uint8_t **ptr)
{
	uint32_t val;

	assert(ptr);

	val = *(*ptr)++; val <<= 8;
	val |= *(*ptr)++; val <<= 8;
	val |= *(*ptr)++; val <<= 8;
	val |= *(*ptr)++;

	return val;
}

static inline int write_u16_to_buf(uint8_t **ptr, uint16_t val)
{
	*(*ptr)++ = (val >> 8) & 0xff;
	*(*ptr)++ = val & 0xff;
	return 2;
}

static inline int write_u32_to_buf(uint8_t **ptr, uint32_t val)
{
	*(*ptr)++ = (val >> 24) & 0xff;
	*(*ptr)++ = (val >> 16) & 0xff;
	*(*ptr)++ = (val >> 8) & 0xff;
	*(*ptr)++ = val & 0xff;
	return 4;
}

/* The JP2 protocol uses a checksum which is an XOR across all data bytes */
static uint8_t jp2_checksum(uint8_t *data, int len)
{
	uint8_t csum = 0;

	while (len--) {
		csum ^= *data++;
	}

	return csum;
}

/* RTS# is connected to the RESET# input of the remote, pulsing this pin
 * for 35ms will put the remote into programming mode */
static void jp2_reset(struct jp2_remote *r)
{
	int rc;

	debug(1, "%s: pulsing RTS#\n", __func__);

	rc = osapi->reset(r->handle, true);
	assert(!rc);

	usleep(135000);

	rc = osapi->reset(r->handle, false);
	assert(!rc);
}

static int jp2_send(struct jp2_remote *r, uint8_t *data, int len)
{
	int rc;

	assert(len < (sizeof(r->txbuf) - 3));

	/* put in the length */
	r->txbuf[0] = ((len + 1) >> 8) & 0xff;
	r->txbuf[1] = (len + 1) & 0xff;

	memcpy(r->txbuf + 2, data, len);

	r->txbuf[len+2] = jp2_checksum(r->txbuf, len + 2);

	debug(1, "%s: %s\n", __func__, hexdump(r->txbuf, len + 3));

	rc = osapi->write(r->handle, r->txbuf, len + 3);
	if (rc != len + 3) {
		return -1;
	}

	return 0;
}

static int jp2_receive(struct jp2_remote *r, uint8_t **data)
{
	int rc;
	int len;
	uint8_t csum;

	/* first read the length */
	rc = osapi->read(r->handle, r->rxbuf, 2);
	if (rc < 0) {
		debug(1, "%s: read() returned error %d\n", __func__, rc);
		return -1;
	}
	assert(rc == 2);

	debug(1, "%s: len=%s\n", __func__, hexdump(r->rxbuf, 2));
	len = (r->rxbuf[0] << 8) | r->rxbuf[1];
	assert(len < (sizeof(r->rxbuf) - 2));
	/* we expect at least an error code and a checksum byte */
	assert(len >= 2);

	/* read remaining bytes */
	rc = osapi->read(r->handle, r->rxbuf + 2, len);
	if (rc < 0) {
		debug(1, "%s: read() returned error %d\n", __func__, rc);
		return -1;
	}
	assert(rc == len);

	debug(1, "%s: %s\n", __func__, hexdump(r->rxbuf, len + 2));

	/* check checksum */
	csum = jp2_checksum(r->rxbuf, len + 2);
	if (csum != 0) {
		debug(1, "%s: checksum error (%02x).\n", __func__, csum);
		return -JP2_ERR_WRONG_CHECKSUM;
	}

	/* check error code */
	if (r->rxbuf[2] != JP2_ERR_NO_ERR) {
		return -r->rxbuf[2];
	}

	/* if we received actual data, return it */
	if ((len > 2) && data) {
		*data = r->rxbuf + 3;
	}

	return len - 2;
}

int jp2_command(struct jp2_remote *r, uint8_t *txdata, int txlen,
	uint8_t **rxdata)
{
	int rc;

	rc = jp2_send(r, txdata, txlen);
	if (rc < 0) {
		return rc;
	}

	return jp2_receive(r, rxdata);
}

int jp2_simple_command(struct jp2_remote *r, uint8_t cmd)
{
	return jp2_command(r, &cmd, 1, NULL);
}

static int _jp2_read_block(struct jp2_remote *r, uint32_t address,
		uint16_t len, uint8_t **data)
{
	uint8_t buf[16], *ptr = buf;
	int txlen;
	assert(len <= JP2_CHUNK_SIZE);

	*ptr++ = JP2_CMD_READ;
	txlen = 1;
	if (r->addr_width == 2) {
		txlen += write_u16_to_buf(&ptr, address);
	} else {
		txlen += write_u32_to_buf(&ptr, address);
	}
	txlen += write_u16_to_buf(&ptr, len);

	return jp2_command(r, buf, txlen, data);
}

int jp2_read_block(struct jp2_remote *r, uint32_t address, uint16_t len,
		uint8_t *data)
{
	int rc;
	uint16_t rxlen;
	uint8_t *_data;
	uint32_t bytes_read = 0;

	while (bytes_read < len)
	{
		rxlen = len - bytes_read;
		if (rxlen > JP2_CHUNK_SIZE) {
			rxlen = JP2_CHUNK_SIZE;
		}
		rc = _jp2_read_block(r, address, rxlen, &_data);
		if (rc < 0) {
			return rc;
		}
		assert(rc == rxlen);

		memcpy(data, _data, rxlen);

		data += rxlen;
		address += rxlen;
		bytes_read += rxlen;
	}

	return bytes_read;
}

static int _jp2_write_block(struct jp2_remote *r, uint32_t address,
		uint16_t len, uint8_t *data)
{
	int rc;
	uint8_t buf[16], *ptr = buf;
	uint8_t *buf2;
	int txlen;

	*ptr++ = JP2_CMD_WRITE;
	txlen = 1;
	if (r->addr_width == 2) {
		txlen += write_u16_to_buf(&ptr, address);
	} else {
		txlen += write_u32_to_buf(&ptr, address);
	}

	/* XXX */
	buf2 = malloc(len + txlen);
	memcpy(buf2, buf, txlen);
	memcpy(buf2 + txlen, data, len);
	txlen += len;

	rc = jp2_command(r, buf2, txlen, NULL);
	free(buf2);

	return rc;
}

int jp2_write_block(struct jp2_remote *r, uint32_t address, uint16_t len,
		uint8_t *data)
{
	int rc;
	uint16_t txlen;
	uint32_t bytes_written = 0;

	while (bytes_written < len)
	{
		txlen = len - bytes_written;
		if (txlen > JP2_CHUNK_SIZE) {
			txlen = JP2_CHUNK_SIZE;
		}
		rc = _jp2_write_block(r, address, txlen, data);
		if (rc < 0) {
			return rc;
		}
		assert(rc == txlen);

		address += txlen;
		data += txlen;
		bytes_written += txlen;
	}

	return bytes_written;
}

int jp2_erase_block(struct jp2_remote *r, uint32_t start, uint32_t end)
{
	uint8_t buf[16], *ptr = buf;
	int txlen;

	*ptr++ = JP2_CMD_ERASE;
	txlen = 1;
	if (r->addr_width == 2) {
		txlen += write_u16_to_buf(&ptr, start);
		txlen += write_u16_to_buf(&ptr, end);
	} else {
		txlen += write_u32_to_buf(&ptr, start);
		txlen += write_u32_to_buf(&ptr, end);
	}
	return jp2_command(r, buf, txlen, NULL);
}

int jp2_checksum_block(struct jp2_remote *r, uint32_t start, uint32_t end)
{
	int rc;
	uint8_t buf[16], *ptr = buf;
	int txlen;
	uint8_t *data;

	*ptr++ = JP2_CMD_CHECKSUM;
	txlen = 1;
	if (r->addr_width == 2) {
		txlen += write_u16_to_buf(&ptr, start);
		txlen += write_u16_to_buf(&ptr, end);
	} else {
		txlen += write_u32_to_buf(&ptr, start);
		txlen += write_u32_to_buf(&ptr, end);
	}

	rc = jp2_command(r, buf, txlen, &data);
	if (rc < 0) {
		return rc;
	}
	if (rc != 1) {
		return JP2_ERR_UNSUPPORTED;
	}
	return *data;
}

int jp2_get_info(struct jp2_remote *r, struct jp2_info *info)
{
	int rc;
	uint8_t cmd = JP2_CMD_INFO;
	uint8_t *data;
	uint32_t info_area_offset;
	uint32_t info_area_size;

	if (info == NULL) {
		return -1;
	}

	rc = jp2_command(r, &cmd, 1, &data);
	if (rc < 0) {
		return rc;
	}

	/* the info command either returns a 16bit or a 32bit offset for the
	 * info block */
	if (rc != 4 && rc != 6) {
		debug(1, "%s: unknown response (%d)\n", __func__, rc);
		return -1;
	}

	r->addr_width = (rc == 4) ? 2 : 4;
	info->id = read_u16_from_buf(&data);

	debug(1, "%s: remote id is %04Xh (%dbit device)\n",
		__func__, info->id, r->addr_width * 8);

	if (r->addr_width == 2) {
		info_area_offset = read_u16_from_buf(&data);
		info_area_size = JP2_SIGNATURE_LEN + 6 * sizeof(uint16_t);
	} else {
		info_area_offset = read_u32_from_buf(&data);
		info_area_size = JP2_SIGNATURE_LEN + 6 * sizeof(uint32_t);
	}
	debug(1, "%s: info_area @%x\n", __func__, info_area_offset);

	rc = _jp2_read_block(r, info_area_offset, info_area_size, &data);
	if (rc < 0) {
		return rc;
	}

	assert(sizeof(info->signature) >= JP2_SIGNATURE_LEN + 1);
	strncpy(info->signature, (char*)data, JP2_SIGNATURE_LEN);
	info->signature[JP2_SIGNATURE_LEN] = '\0';

	data += JP2_SIGNATURE_LEN;
	if (r->addr_width == 2) {
		info->program_area_begin = read_u16_from_buf(&data);
		info->program_area_end = read_u16_from_buf(&data);
		info->protocol_area_begin = read_u16_from_buf(&data);
		info->protocol_area_end = read_u16_from_buf(&data);
		info->update_area_begin = read_u16_from_buf(&data);
		info->update_area_end = read_u16_from_buf(&data);
	} else {
		info->program_area_begin = read_u32_from_buf(&data);
		info->program_area_end = read_u32_from_buf(&data);
		info->protocol_area_begin = read_u32_from_buf(&data);
		info->protocol_area_end = read_u32_from_buf(&data);
		info->update_area_begin = read_u32_from_buf(&data);
		info->update_area_end = read_u32_from_buf(&data);
	}

	debug(1, "%s program_area_begin=%x\n", __func__,
			info->program_area_begin);
	debug(1, "%s program_area_end=%x\n", __func__,
			info->program_area_end);
	debug(1, "%s protocol_area_begin=%x\n", __func__,
			info->protocol_area_begin);
	debug(1, "%s protocol_area_end=%x\n", __func__,
			info->protocol_area_end);
	debug(1, "%s update_area_begin=%x\n", __func__,
			info->update_area_begin);
	debug(1, "%s update_area_end=%x\n", __func__,
			info->update_area_end);

	return 0;
}

int jp2_enter_loader(struct jp2_remote *r)
{
	jp2_reset(r);

	/* we need to wait until the processor starts up */
	usleep(150000);

	/* flush any spurious characters in the input buffer */
	osapi->flush(r->handle);

	return jp2_simple_command(r, JP2_CMD_ENTER_LOADER);
}

int jp2_exit_loader(struct jp2_remote *r)
{
	return jp2_simple_command(r, JP2_CMD_EXIT_LOADER);
}

struct jp2_remote *jp2_open_remote(const char *devname)
{
	struct jp2_remote *r;

	r = malloc(sizeof(*r));
	assert(r);

	memset(r, 0, sizeof(*r));

	r->handle = osapi->open(devname, 0);
	if (r->handle == NULL) {
		free(r);
		return NULL;
	}

	return r;
}

void jp2_close_remote(struct jp2_remote *r)
{
	osapi->close(r->handle);
	free(r);
}

int jp2_init(void)
{
	if (getenv("JP2_DEBUG")) {
		debug_level = 1;
	}

	return 0;
}

