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
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define MAP_SIZE 0x4000000
#define MAP_MASK (MAP_SIZE - 1)

#define align(v, a) (((v) + (a) - 1) & ~((a) - 1))
#define max(a, b) ((a) > (b) ? (a) : (b))

#ifdef __arm__
# define mb() __asm__ volatile ("dmb" ::: "memory")
#elif defined __mips__
# define mb() __asm__ volatile ("sync" ::: "memory")
#else
# define mb() __asm__ volatile ("" ::: "memory")
#endif

static inline void writel(uint32_t val, void *addr)
{
	*(volatile uint32_t *)addr = val;
}

int main(int argc, char **argv)
{
	unsigned long map_base = 0, map_size = 0, offset;
	unsigned long addr, val = 0, count, nval, size;
	char *mem = NULL;
	char *rep;
	int fd;

	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0) {
		perror("/dev/mem");
		return 1;
	}

        argc--;
        argv++;

        while (argc >= 2) {
		addr = strtoul(argv[0], &rep, 0);
		argc--;
		argv++;

		switch (*rep) {
		case '+':
		case ':':
			count = strtoul(rep + 1, NULL, 0);
			break;
		case 0:
			count = 1;
			break;
		default:
			fprintf(stderr, "bad address %s\n", argv[1]);
			return 1;
		}

		if (!count) {
			fprintf(stderr, "count is zero\n");
			return 1;
		}

		nval = *rep == '+' ? count : 1;

		if (argc < nval) {
			fprintf(stderr, "too few values\n");
			return 1;
		}

		size = 4 * count;

		if (!mem || addr < map_base ||
		    addr + size >= map_base + map_size) {
			if (mem)
				munmap(mem, map_size);

			offset = addr & MAP_MASK;
			map_base = addr & ~MAP_MASK;
			map_size = max(offset + size, MAP_SIZE);
			map_size = align(map_size, MAP_SIZE);

			mem = mmap(NULL, map_size, PROT_READ | PROT_WRITE,
				   MAP_SHARED, fd, map_base);
			if (mem == MAP_FAILED) {
				perror("mmap");
				return 1;
			}
		}

		addr &= MAP_MASK;

		while (count--) {
			if (nval) {
				val = strtoul(argv[0], NULL, 0);
				nval--;
				argc--;
				argv++;
			}

			writel(val, mem + addr);
			addr += 4;
		}

		mb();
        }

	munmap(mem, MAP_SIZE);
	close(fd);

	return 0;
}
