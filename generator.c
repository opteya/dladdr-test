/* generate GNU assembler code to emit random objects along fixed ones
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

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static uint32_t rand32(void)
{
	return (uint32_t)mrand48();
}

static uint32_t rand32_max(uint32_t max)
{
	if (max == 0)
		return 0;

	if (max == UINT32_MAX)
		return rand32();

	const uint32_t upper_bound = max + 1;
        const uint32_t interval = UINT32_MAX - (UINT32_MAX % upper_bound);
	uint32_t v;

        while (1)
        {
                v = rand32 ();
                if (v < interval)
                        break;
        }

        v %= upper_bound;

	return v;
}

static char *gen_random_name(const char *prefix)
{
	static const char set[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789"
		"_";
	const size_t setlen = sizeof(set) - 1;

	const size_t prefixlen = strlen(prefix);

	static char name[1024 + 1];

	assert(sizeof(name) - 1 > prefixlen);

	char random[256];

	const size_t randomlen = rand32_max(sizeof(random) - 1);
	size_t i;

	static size_t counter; // prevent any kind of collision

	for (i = 0; i < randomlen; i++)
		random[i] = set[rand32_max(setlen - 1)];
	random[i] = '\0';

	snprintf(name, sizeof(name),
		 "%s_%zu_%s",
		 prefix, counter++, random);

	return name;
}

static void emit_random_symbol(const char *prefix)
{
	const uint32_t align = rand32_max(16);
	const uint32_t size = 1 + rand32_max(12345);
	const char *name = gen_random_name(prefix);

	printf("\t.globl %s\n"
	       "\t.p2align %" PRIu32 "\n"
	       "\t.type  %s, @object\n"
	       "\t.size  %s, %" PRIu32 "\n"
	       "%s:\n"
	       "\t.zero  %" PRIu32 "\n"
	       "\n",
	       name,
	       align,
	       name,
	       name, size,
	       name,
	       size);
}

static void emit_known_symbol(const char *name, unsigned long size)
{
	const uint32_t align = rand32_max(16);

	printf("\t.globl %s\n"
	       "\t.p2align %" PRIu32 "\n"
	       "\t.type  %s, @object\n"
	       "\t.size  %s, %lu\n"
	       "%s:\n"
	       "\t.zero  %lu\n"
	       "\n",
	       name,
	       align,
	       name,
	       name, size,
	       name,
	       size);
}

// generate the name for the known symbol (and the prefix for all random symbols)
static const char *gen_known_name(const char *prefix, unsigned long size)
{
	static char name[1024];

	snprintf(name, sizeof(name), "%s%lu", prefix, size);

	return name;
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		fprintf(stderr, "Syntax: %s <name> <size>\n", argv[0]);
		return 1;
	}

	srand48(getpid() ^ time(NULL));

	errno = 0;

	char *ptr = NULL;
	unsigned long size = strtoul(argv[2], &ptr, 0);

	if (size == ULONG_MAX)
	{
		if (errno != 0)
		{
			fprintf(stderr, "%s: %s\n", argv[2], strerror(errno));
			return 1;
		}
	}

	if (ptr && *ptr != '\0')
	{
		fprintf(stderr, "%s: invalid size\n", argv[2]);
		return 1;
	}

	const char *name = gen_known_name(argv[1], size);

	// nitpicking: ensure generated libraries have a licence compatible with dladdr-test programs
	printf("/*\n"
	       " * This library is free software; you can redistribute it and/or\n"
	       " * modify it under the terms of the GNU Lesser General Public\n"
	       " * License as published by the Free Software Foundation; either\n"
	       " * version 2.1 of the License, or (at your option) any later version.\n"
	       " *\n"
	       " * This library is distributed in the hope that it will be useful,\n"
	       " * but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	       " * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"
	       " * Lesser General Public License for more details.\n"
	       " *\n"
	       " * You should have received a copy of the GNU Lesser General Public\n"
	       " * License along with this library; if not, write to the Free Software\n"
	       " * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA\n"
	       " * 02110-1301  USA\n"
	       " */\n");

	printf("\t.section .rodata\n");

	// generate random objects before
	uint32_t count = rand32_max(123);

	for (uint32_t i = 0; i < count; i++)
		emit_random_symbol(name);

	// generate objects with fixed name and size
	emit_known_symbol(name, size);

	// generate some random objects after
	count = rand32_max(123);

	for (uint32_t i = 0; i < count; i++)
		emit_random_symbol(name);

	return 0;
}
