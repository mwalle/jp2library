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

#ifndef __JP12SERIAL_COMPAT_H
#define __JP12SERIAL_COMPAT_H

#include <jni.h>

typedef const struct JNINativeInterface_ *JNIEnv;

#define JP12FUNC_1(name, ret, arg1) \
	JNIEXPORT ret JNICALL Java_com_hifiremote_jp1_io_JP12Serial_ ## name( \
		JNIEnv *env, arg1)
#define JP12FUNC_2(name, ret, arg1, arg2) \
	JNIEXPORT ret JNICALL Java_com_hifiremote_jp1_io_JP12Serial_ ## name( \
		JNIEnv *env, arg1, arg2)
#define JP12FUNC_3(name, ret, arg1, arg2, arg3) \
	JNIEXPORT ret JNICALL Java_com_hifiremote_jp1_io_JP12Serial_ ## name( \
		JNIEnv *env, arg1, arg2, arg3)
#define JP12FUNC_4(name, ret, arg1, arg2, arg3, arg4) \
	JNIEXPORT ret JNICALL Java_com_hifiremote_jp1_io_JP12Serial_ ## name( \
		JNIEnv *env, arg1, arg2, arg3, arg4)

JP12FUNC_1(getInterfaceName, jstring, jobject);
JP12FUNC_1(getInterfaceVersion, jstring, jobject);
JP12FUNC_1(getJP12InterfaceType, jint, jobject);
JP12FUNC_3(getJP2info, jboolean, jobject, jbyteArray, jint);
JP12FUNC_1(getPortNames, jobjectArray, jobject);
JP12FUNC_2(openRemote, jstring, jobject, jstring);
JP12FUNC_1(closeRemote, void, jobject);
JP12FUNC_1(getRemoteSignature, jstring, jobject);
JP12FUNC_1(getRemoteEepromAddress, jint, jobject);
JP12FUNC_1(getRemoteEepromSize, jint, jobject);
JP12FUNC_4(readRemote, jint, jobject, jint, jbyteArray, jint);
JP12FUNC_4(writeRemote, jint, jobject, jint, jbyteArray, jint);

#endif /* __JP12SERIAL_COMPAT_H */
