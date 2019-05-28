/* use dladdr() to lookup known symbols from addresses
 *
 * Copyright (C) 2019 OPTEYA SAS
 * Copyright (C) 2019 Yann Droneaud <ydroneaud@opteya.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#define _GNU_SOURCE // RTLD_DEFAULT
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef UINTPTR_WIDTH
# if UINTPTR_WIDTH == 32
#  define test_UINTPTR_C(v) UINT32_C(v)
# elif UINTPTR_WIDTH == 64
#  define test_UINTPTR_C(v) UINT64_C(v)
# else
#  error UINTPTR_WIDTH not supported
# endif
#else
# error UINTPTR_WIDTH not defined
#endif

static const uintptr_t test_sizemax = test_UINTPTR_C(SIZEMAX);

static void test(void)
{
	uintptr_t i;

	for (i = 1; i <= test_sizemax; i *= 2)
	{
		char name[strlen("symbol") + 10 + 1];
		snprintf(name, sizeof(name), "symbol%" PRIuPTR, i);

		uint8_t *addr = dlsym(RTLD_DEFAULT, name);
		if (addr == NULL)
		{
			fprintf(stderr, "%s: %s\n", name, dlerror());
			break;
		}

		uintptr_t offset = i / 2;
		Dl_info info;
		if (dladdr(addr + offset, &info) == 0)
		{
			fprintf(stderr, "%s!%p+%#" PRIxPTR ": %s\n", name, addr, offset, dlerror());
			abort();
		}

		if (info.dli_sname == NULL)
		{
			fprintf(stderr, "%s!%p+%#" PRIxPTR ": name is NULL\n", name, addr, offset);
			abort();
		}

		if (strcmp(info.dli_sname, name) != 0)
		{
			fprintf(stderr, "%s!%p+%#" PRIxPTR ": name mismatch: %s\n", name, addr, offset, info.dli_sname);
			abort();
		}

		if (info.dli_saddr != addr)
		{
			fprintf(stderr, "%s!%p+%#" PRIxPTR ": addr mismatch: %p\n", name, addr, offset, info.dli_saddr);
			abort();
		}
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Syntax: %s <count>\n", argv[0]);
		return 1;
	}

	errno = 0;

	char *ptr = NULL;
	unsigned long count = strtoul(argv[1], &ptr, 0);

	if (count == ULONG_MAX)
	{
		if (errno != 0)
		{
			fprintf(stderr, "%s: %s\n", argv[1], strerror(errno));
			return 1;
		}
	}

	if (ptr && *ptr != '\0')
	{
		fprintf(stderr, "%s: invalid size\n", argv[1]);
		return 1;
	}

	for (unsigned long i = 0; i < count; i++)
		test();

	return 0;
}
