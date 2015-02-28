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
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define BUFSIZE 256

#define min(a, b) ((a) < (b) ? (a) : (b))

int main(int argc, char **argv)
{
	unsigned long map_offset, map_size;
	unsigned long offset, size;
	long pagesize;
	char *mem, *data;
	char buf[BUFSIZE];
	int fd, n;

	if (argc < 3)
		return 1;

	offset = strtoul(argv[1], NULL, 0);
	size   = strtoul(argv[2], NULL, 0);

	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd == -1) {
		perror("/dev/mem");
		return 1;
	}

	pagesize = sysconf(_SC_PAGESIZE);
	map_offset = offset & ~(pagesize - 1);
	map_size = size + (offset & (pagesize - 1));
	map_size = (map_size + pagesize - 1) & ~(pagesize - 1);

	mem = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED,
		   fd, map_offset);
	if (mem == MAP_FAILED) {
		perror("mmap");
		return 1;
	}

	data = mem + (offset - map_offset);

        while (size) {
		n = min(size, BUFSIZE);

		n = read(0, buf, n);
		if (n < 0) {
			perror("read");
			return 1;
		}
		if (n == 0)
			break;

		memcpy(data, buf, n);

		data += n;
		size -= n;
        }

	munmap(mem, map_size);
	close(fd);

	return 0;
}
