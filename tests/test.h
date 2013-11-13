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

#ifndef __TEST_H
#define __TEST_H

#include <setjmp.h>

#define t_assert_msg(test, msg) \
	do {                                             \
		if (!(test)) {                               \
			fprintf(stderr, msg);                    \
			longjmp(t_env, 1);                       \
		}                                            \
	} while (0)
#define t_assert(test) t_assert_msg(test, "Assertion: '" #test "' failed.")
#define t_run_test(test) \
	do {                                             \
		if (!setjmp(t_env)) {                        \
			fprintf(stderr, "Running " #test ".. "); \
			test();                                  \
			fprintf(stderr, "ok\n");                 \
		} else {                                     \
			fprintf(stderr, "failed\n");             \
		}                                            \
		t_tests_run++;                                 \
	} while (0)

#define T_DEFS           \
	jmp_buf t_env;       \
	int t_tests_run = 0;

extern jmp_buf t_env;
extern int t_tests_run;

void test_init(void);
void test_tx(uint8_t *data, int len);
uint8_t *test_rx(int len);

#endif /* __TEST_H */
