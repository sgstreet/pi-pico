#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <byteswap.h>

#include <board/board.h>

#include <compiler.h>
#include <sys/systick.h>

extern uint32_t crc32_small(const uint8_t *buf, unsigned int len, uint32_t seed);

static uint32_t crc32_tool(const uint8_t *data, size_t size)
{
	uint32_t crc = 0xFFFFFFFF;
	for (size_t i = 0; i < size; i++) {
		uint32_t byte = bswap_32(data[i]);
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
	const char *msg = "The quick brown fox jumps over the lazy dog";
	uint32_t crc1 = crc32_small((const uint8_t *)msg, strlen(msg), 0xffffffff);
	uint32_t crc2 = crc32_tool((const uint8_t *)msg, strlen(msg));
	uint32_t crc3 = crc32_tool((const uint8_t *)0x10000000, 252);
	printf("%s: 0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", msg, crc1, crc2, crc3, *(uint32_t *)(0x10000000 + 252));
}

