/*
   Licensed under GNU General Public License v2 or later.
   */
#if 0
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <sys/types.h>
#include "hw/misc/s32g2/shmem.h"
#include <errno.h>

void shared_memory_init(char* region_path, size_t size, void** buf)
{
	/* Create shared memory object and set its size to the size
	   of our structure. */

	int fd = shm_open(region_path, O_CREAT | O_EXCL | O_RDWR,
			S_IRUSR | S_IWUSR);
	if (fd == -1)
	{
		printf("ERROR: shm_open %d\n", errno);
		return;
	}

	if (ftruncate(fd, size) == -1)
	{
		printf("ERROR: ftruncate %d\n", errno);
		return;
	}

	/* Map the object into the caller's address space. */

	*buf = mmap(NULL, size,
			PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, 0);
	if (*buf == MAP_FAILED)
	{
		printf("ERROR: mmap %d\n", errno);
	}

}



#endif
