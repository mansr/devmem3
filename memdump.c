/*
  Copyright (c) 2015 Mans Rullgard <mans@mansr.com>
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

typedef void (*print_fun)(void *p, long n);

#define min(a, b) ((a) < (b) ? (a) : (b))
#define align(v, a) (((v) + (a) - 1) & ~((a) - 1))

static void die(void)
{
	fprintf(stderr, "usage: memdump {-b|-h|-w|-r} addr [count]\n");
	exit(1);
}

static void print32(void *p, long n)
{
	uint32_t *d = p;
	int i;

	for (i = 0; i < n / 4; i++)
		printf(" %08x", d[i]);
	printf("\n");
}

static void print16(void *p, long n)
{
	uint16_t *d = p;
	int i;

	for (i = 0; i < n / 2; i++)
		printf(" %04x", d[i]);
	printf("\n");
}

static void print8(void *p, long n)
{
	uint8_t *d = p;
	int i;

	for (i = 0; i < n; i++)
		printf(" %02x", d[i]);
	printf("\n");
}

static void print_mem(uint8_t *p, unsigned long size, unsigned long base,
		      print_fun pf)
{
	unsigned long addr = base;
	uint8_t buf[2][16];
	long n;
	int i = 0;
	int skip = 0;

	while (size) {
		n = min(size, 16);
		memcpy(buf[i], p, n);

		if (addr == base || memcmp(buf[i], buf[!i], 16)) {
			printf("%08lx:", addr);
			pf(buf[i], n);
			skip = 0;
		} else if (!skip) {
			printf("...\n");
			skip = 1;
		}

		p += n;
		addr += n;
		size -= n;
		i ^= 1;
	}

	if (skip)
		printf("%08lx\n", addr);
}

int main(int argc, char **argv)
{
	unsigned long addr, offset, map_base, map_size;
	long pagesize, pagemask;
	unsigned long count = 1;
	uint8_t *mem, *data;
	int size, raw = 0;
	print_fun pf;
	int fd;

	if (argc < 3) {
		die();
	}

	if (argv[1][0] != '-')
		die();

	switch (argv[1][1]) {
	case 'b': size = 1; pf = print8;  break;
	case 'h': size = 2; pf = print16; break;
	case 'w': size = 4; pf = print32; break;
	case 'r': size = 1; raw = 1;      break;
	default:  die();
	}

	argc--;
	argv++;

	addr = strtoul(argv[1], NULL, 0);
	if (argc > 2)
		count  = strtoul(argv[2], NULL, 0);

	fd = open("/dev/mem", O_RDONLY | O_SYNC);
	if (fd == -1) {
		perror("/dev/mem");
		return 1;
	}

	pagesize = sysconf(_SC_PAGESIZE);
	pagemask = pagesize - 1;
	map_base = addr & ~pagemask;
	offset = addr & pagemask;
	map_size = align(count * size + offset, pagesize);

	mem = mmap(NULL, map_size, PROT_READ, MAP_SHARED, fd, map_base);
	if (mem == MAP_FAILED) {
		perror("mmap");
		return 1;
	}

	data = mem + offset;

	if (raw) {
		if (write(1, data, count) < 0)
			perror("write");
	} else {
		print_mem(data, count * size, addr, pf);
	}

	munmap(mem, map_size);
	close(fd);

	return 0;
}
