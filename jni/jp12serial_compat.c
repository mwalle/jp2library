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
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdlib.h>
#include <jni.h>

#include "jp2library.h"
#include "osapi.h"
#include "jp12serial_compat.h"

/* Unfortunately, there is no jni handle attribute in the io.JP12Serial class.
 * Therefore, we have to remember our state here. */
static struct jp2_remote *r;
static struct jp2_info info;

static void jp2_initialize(void)
{
	static bool init_done = false;
	if (!init_done) {
		jp2_init();
		init_done = true;
	}
}

JP12FUNC_1(getInterfaceName, jstring, jobject obj)
{
	jp2_initialize();
	return (*env)->NewStringUTF(env, "jp2library");
}

JP12FUNC_1(getInterfaceVersion, jstring, jobject obj)
{
	jp2_initialize();
	return (*env)->NewStringUTF(env, jp2_version);
}

JP12FUNC_1(getJP12InterfaceType, jint, jobject obj)
{
	jp2_initialize();

	/* JP2_14 */
	return 1;
}

JP12FUNC_3(getJP2info, jboolean, jobject obj, jbyteArray buffer, jint length)
{
	jp2_initialize();
	return false;
}

JP12FUNC_1(getPortNames, jobjectArray, jobject obj)
{
	jobjectArray array;
	const char *name;
	int port = 0;
	jstring portnames[32];

	jp2_initialize();

	while ((name = osapi->enumerate()) && port < 32) {
		portnames[port++] = (*env)->NewStringUTF(env, name);
	}

	array = (*env)->NewObjectArray(env, port,
			(*env)->FindClass(env, "java/lang/String"),
			NULL);

	while (port--)
	{
		(*env)->SetObjectArrayElement(env, array, port,
				portnames[port]);
	}

	return array;
}

JP12FUNC_2(openRemote, jstring, jobject obj, jstring jportname)
{
	int rc;
	const char *portname;

	jp2_initialize();

	if (jportname == NULL) {
		/* XXX: auto-detect not supported yet */
		return NULL;
	}

	portname = (*env)->GetStringUTFChars(env, jportname, NULL);
	r = jp2_open_remote(portname);
	(*env)->ReleaseStringUTFChars(env, jportname, portname);

	rc = jp2_enter_loader(r);
	if (rc) {
		jp2_close_remote(r);
		return NULL;
	}

	rc = jp2_get_info(r, &info);
	if (rc) {
		jp2_close_remote(r);
		return NULL;
	}

	return jportname;
}

JP12FUNC_1(closeRemote, void, jobject obj)
{
	jp2_initialize();
	jp2_close_remote(r);
}

JP12FUNC_1(getRemoteSignature, jstring, jobject obj)
{
	int i;
	char signature[9];

	jp2_initialize();

	memset(signature, '_', 8);
	for (i = 0; i < 8; i++) {
		if (!isprint(info.signature[i])) {
			break;
		}
		signature[i] = info.signature[i];
	}
	signature[8] = '\0';
	return (*env)->NewStringUTF(env, signature);
}

JP12FUNC_1(getRemoteEepromAddress, jint, jobject obj)
{
	jp2_initialize();
	return info.update_area_begin;
}

JP12FUNC_1(getRemoteEepromSize, jint, jobject obj)
{
	jp2_initialize();
	return info.update_area_end - info.update_area_begin + 1;
}

JP12FUNC_4(readRemote, jint, jobject obj, jint address, jbyteArray jbuffer,
		jint _unused)
{
	int rc;
	jbyte *buf;
	int len;

	jp2_initialize();

	len = (*env)->GetArrayLength(env, jbuffer);
	buf = (*env)->GetByteArrayElements(env, jbuffer, NULL);
	rc = jp2_read_block(r, address, len, (uint8_t*)buf);
	(*env)->ReleaseByteArrayElements(env, jbuffer, buf, 0);

	return rc;
}

JP12FUNC_4(writeRemote, jint, jobject obj, jint address, jbyteArray jbuffer,
		jint length)
{
	int rc;
	jbyte *buf;
	int len;

	jp2_initialize();

	len = (*env)->GetArrayLength(env, jbuffer);
	buf = malloc(len * sizeof(jbyte));

	/* XXX: convert to exception */
	assert(buf);

	(*env)->GetByteArrayRegion(env, jbuffer, 0, len, buf);
	rc = jp2_write_block(r, address, len, (uint8_t*)buf);
	free(buf);

	return rc;
}
