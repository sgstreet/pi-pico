/*
 * bootstrap-tool.c
 *
 *  Created on: Dec 22, 2022
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <byteswap.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <elf.h>

static uint32_t crc32(const uint8_t *data, size_t size)
{
	uint32_t crc = 0xFFFFFFFF;
	for (size_t i = 0; i < size; i++) {
		uint32_t byte = data[i];
		byte = bswap_32(byte);
		for (size_t j = 0; j < 8; ++j) {
			if ((int32_t)(crc ^ byte) < 0)
				crc = (crc << 1) ^ 0x04c11db7;
			else
				crc = crc << 1;
			byte = byte << 1;
		}
	}
	return crc;
}

int main(int argc, char **argv)
{
	int exit_status = EXIT_FAILURE;

	const char *msg = "The quick brown fox jumps over the lazy dog";
	printf("%s: 0x%08x\n", msg, crc32((const uint8_t *)msg, strlen(msg)));

	/* Open the image path */
	int fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "could not open `%s`: %s\n", argv[1], strerror(errno));
	}

	/* Get the length of the image */
	struct stat stat;
	if (fstat(fd, &stat) < 0) {
		fprintf(stderr, "failed to get file size of `%s: %s`\n", argv[1], strerror(errno));
		goto error_close;
	}

	/* Map into our address space */
	void *elf = mmap(0, stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (!elf) {
		fprintf(stderr, "failed mmap `%s`: %s\n", argv[1], strerror(errno));
		goto error_close;
	}

	/* Find the bootstrap section */
	Elf32_Ehdr *file_hdr = elf;
	Elf32_Shdr *sec_hdr = elf + file_hdr->e_shoff;
	Elf32_Shdr *sec_str_hdr = &sec_hdr[file_hdr->e_shstrndx];
	char *sec_names = elf + sec_str_hdr->sh_offset;
	for (int i = 0; i < file_hdr->e_shnum; ++i) {
		if (strcmp(sec_names + sec_hdr[i].sh_name, ".bootstrap") == 0) {
			uint32_t *crc = elf + sec_hdr[i].sh_offset + 252;
			*crc = crc32(elf + sec_hdr[i].sh_offset, 252);
			printf("0x%08x crc: 0x%08x\n", sec_hdr[i].sh_offset + 252, *crc);

		}
	}

	munmap(elf, stat.st_size);

	/* All good */
	exit_status = EXIT_SUCCESS;

error_close:
	close(fd);

	return exit_status;
}


