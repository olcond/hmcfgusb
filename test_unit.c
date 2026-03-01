/* Unit tests for hmcfgusb
 *
 * Tests hardware-independent modules: util.c, firmware.c, hm.h macros.
 * Uses assert() — failure aborts with file/line info.
 *
 * Build and run:  CC=gcc make test
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* Include firmware.c directly to access static crc16() */
#include "firmware.c"

/* hm.h for packet macros (SRC, DST, etc.) */
#include "hm.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
	tests_run++; \
	printf("  %s... ", #name); \
	name(); \
	tests_passed++; \
	printf("OK\n"); \
} while(0)

/* ── util.c tests ───────────────────────────────────────────────── */

static void test_ascii_to_nibble_digits(void)
{
	assert(ascii_to_nibble('0') == 0);
	assert(ascii_to_nibble('1') == 1);
	assert(ascii_to_nibble('5') == 5);
	assert(ascii_to_nibble('9') == 9);
}

static void test_ascii_to_nibble_upper(void)
{
	assert(ascii_to_nibble('A') == 10);
	assert(ascii_to_nibble('B') == 11);
	assert(ascii_to_nibble('F') == 15);
}

static void test_ascii_to_nibble_lower(void)
{
	assert(ascii_to_nibble('a') == 10);
	assert(ascii_to_nibble('c') == 12);
	assert(ascii_to_nibble('f') == 15);
}

static void test_ascii_to_nibble_invalid(void)
{
	assert(ascii_to_nibble('G') == 0);
	assert(ascii_to_nibble('z') == 0);
	assert(ascii_to_nibble(' ') == 0);
	assert(ascii_to_nibble('/') == 0);
	assert(ascii_to_nibble(':') == 0);
}

static void test_nibble_to_ascii_all(void)
{
	const char *expected = "0123456789ABCDEF";
	for (uint8_t i = 0; i < 16; i++) {
		assert(nibble_to_ascii(i) == expected[i]);
	}
}

static void test_nibble_to_ascii_masked(void)
{
	/* Values > 15 should be masked with & 0xf */
	assert(nibble_to_ascii(0x10) == '0');
	assert(nibble_to_ascii(0x1A) == 'A');
	assert(nibble_to_ascii(0xFF) == 'F');
}

static void test_validate_nibble_valid(void)
{
	assert(validate_nibble('0') == 1);
	assert(validate_nibble('9') == 1);
	assert(validate_nibble('A') == 1);
	assert(validate_nibble('F') == 1);
	assert(validate_nibble('a') == 1);
	assert(validate_nibble('f') == 1);
}

static void test_validate_nibble_invalid(void)
{
	assert(validate_nibble('G') == 0);
	assert(validate_nibble('g') == 0);
	assert(validate_nibble(' ') == 0);
	assert(validate_nibble('/') == 0);
	assert(validate_nibble(':') == 0);
	assert(validate_nibble('@') == 0);
	assert(validate_nibble('`') == 0);
	assert(validate_nibble(0x00) == 0);
	assert(validate_nibble(0xFF) == 0);
}

static void test_validate_nibble_boundaries(void)
{
	/* Just below/above valid ranges */
	assert(validate_nibble('0' - 1) == 0);
	assert(validate_nibble('9' + 1) == 0);
	assert(validate_nibble('A' - 1) == 0);
	assert(validate_nibble('F' + 1) == 0);
	assert(validate_nibble('a' - 1) == 0);
	assert(validate_nibble('f' + 1) == 0);
}

static void test_ascii_nibble_roundtrip(void)
{
	/* Converting nibble→ascii→nibble should be identity for 0-15 */
	for (uint8_t i = 0; i < 16; i++) {
		char c = nibble_to_ascii(i);
		assert(ascii_to_nibble((uint8_t)c) == i);
	}
}

/* ── hm.h macro tests ──────────────────────────────────────────── */

static void test_src_dst_roundtrip(void)
{
	uint8_t buf[16] = {0};
	uint32_t src = 0x123456;
	uint32_t dst = 0xABCDEF;

	SET_SRC(buf, src);
	SET_DST(buf, dst);

	assert((uint32_t)SRC(buf) == src);
	assert((uint32_t)DST(buf) == dst);
}

static void test_src_dst_zero(void)
{
	uint8_t buf[16] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	                   0xFF, 0xFF, 0xFF, 0xFF};
	SET_SRC(buf, 0x000000);
	SET_DST(buf, 0x000000);

	assert(SRC(buf) == 0x000000);
	assert(DST(buf) == 0x000000);
}

static void test_src_dst_max(void)
{
	uint8_t buf[16] = {0};
	SET_SRC(buf, 0xFFFFFF);
	SET_DST(buf, 0xFFFFFF);

	assert(SRC(buf) == 0xFFFFFF);
	assert(DST(buf) == 0xFFFFFF);
}

static void test_src_dst_independent(void)
{
	uint8_t buf[16] = {0};
	SET_SRC(buf, 0xAABBCC);
	SET_DST(buf, 0x112233);

	/* Changing DST should not affect SRC and vice versa */
	assert(SRC(buf) == 0xAABBCC);
	assert(DST(buf) == 0x112233);

	SET_SRC(buf, 0x000000);
	assert(DST(buf) == 0x112233);
}

static void test_payloadlen_roundtrip(void)
{
	uint8_t buf[16] = {0};

	SET_LEN_FROM_PAYLOADLEN(buf, 0);
	assert(buf[LEN] == 0x09);
	assert(PAYLOADLEN(buf) == 0);

	SET_LEN_FROM_PAYLOADLEN(buf, 10);
	assert(buf[LEN] == 19);
	assert(PAYLOADLEN(buf) == 10);

	SET_LEN_FROM_PAYLOADLEN(buf, 37);
	assert(buf[LEN] == 46);
	assert(PAYLOADLEN(buf) == 37);
}

static void test_hm_field_offsets(void)
{
	uint8_t buf[16] = {0};

	buf[LEN] = 0x15;
	buf[MSGID] = 0x42;
	buf[CTL] = 0xA4;
	buf[TYPE] = 0x11;

	assert(buf[0x00] == 0x15);
	assert(buf[0x01] == 0x42);
	assert(buf[0x02] == 0xA4);
	assert(buf[0x03] == 0x11);
	assert(PAYLOAD == 0x0a);
}

/* ── firmware.c tests (crc16) ───────────────────────────────────── */

static void test_crc16_empty(void)
{
	uint8_t buf[1] = {0};
	/* Zero-length input should return the initial CRC unchanged */
	assert(crc16(buf, 0, CRC16_INIT) == CRC16_INIT);
	assert(crc16(buf, 0, 0x0000) == 0x0000);
	assert(crc16(buf, 0, 0x1234) == 0x1234);
}

static void test_crc16_deterministic(void)
{
	uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
	uint16_t crc1 = crc16(data, sizeof(data), CRC16_INIT);
	uint16_t crc2 = crc16(data, sizeof(data), CRC16_INIT);
	assert(crc1 == crc2);
}

static void test_crc16_different_data(void)
{
	uint8_t data1[] = {0x00};
	uint8_t data2[] = {0x01};
	uint16_t crc1 = crc16(data1, 1, CRC16_INIT);
	uint16_t crc2 = crc16(data2, 1, CRC16_INIT);
	assert(crc1 != crc2);
}

static void test_crc16_different_init(void)
{
	uint8_t data[] = {0x41};
	uint16_t crc1 = crc16(data, 1, 0x0000);
	uint16_t crc2 = crc16(data, 1, 0xFFFF);
	assert(crc1 != crc2);
}

static void test_crc16_not_mutate_input(void)
{
	uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
	uint8_t copy[] = {0xDE, 0xAD, 0xBE, 0xEF};
	crc16(data, sizeof(data), CRC16_INIT);
	assert(memcmp(data, copy, sizeof(data)) == 0);
}

/* ── firmware.c tests (firmware_read_firmware) ──────────────────── */

static void test_firmware_read_eq3(void)
{
	struct firmware *fw = firmware_read_firmware("testdata/test.eq3",
	                                            ATMEGA_UNKNOWN, 0);
	assert(fw != NULL);
	assert(fw->fw_blocks == 2);

	/* Block 0: 4 bytes of DEADBEEF */
	assert(fw->fw[0][0] == 0x00);  /* block num high */
	assert(fw->fw[0][1] == 0x00);  /* block num low */
	assert(fw->fw[0][2] == 0x00);  /* len high */
	assert(fw->fw[0][3] == 0x04);  /* len low */
	assert(fw->fw[0][4] == 0xDE);
	assert(fw->fw[0][5] == 0xAD);
	assert(fw->fw[0][6] == 0xBE);
	assert(fw->fw[0][7] == 0xEF);

	/* Block 1: 2 bytes of CAFE */
	assert(fw->fw[1][0] == 0x00);  /* block num high */
	assert(fw->fw[1][1] == 0x01);  /* block num low */
	assert(fw->fw[1][2] == 0x00);  /* len high */
	assert(fw->fw[1][3] == 0x02);  /* len low */
	assert(fw->fw[1][4] == 0xCA);
	assert(fw->fw[1][5] == 0xFE);

	firmware_free(fw);
}

static void test_firmware_read_ihex(void)
{
	struct firmware *fw = firmware_read_firmware("testdata/test.ihex",
	                                            ATMEGA_328P, 0);
	assert(fw != NULL);
	/* ATmega328P: image_size=0x7000, block_length=128 → 224 blocks */
	assert(fw->fw_blocks == 224);

	/* First block should contain our data at the start */
	assert(fw->fw[0][0] == 0x00);  /* block 0 high */
	assert(fw->fw[0][1] == 0x00);  /* block 0 low */
	assert(fw->fw[0][2] == 0x00);  /* len 128 high */
	assert(fw->fw[0][3] == 0x80);  /* len 128 low */
	/* Data bytes from ihex record at address 0x0000 */
	assert(fw->fw[0][4] == 0xDE);
	assert(fw->fw[0][5] == 0xAD);
	assert(fw->fw[0][6] == 0xBE);
	assert(fw->fw[0][7] == 0xEF);
	/* Rest of first block should be 0xFF (uninitialized image) */
	assert(fw->fw[0][8] == 0xFF);

	firmware_free(fw);
}

static void test_firmware_free_null_blocks(void)
{
	/* Allocate a firmware struct with 0 blocks, then free it */
	struct firmware *fw = malloc(sizeof(struct firmware));
	assert(fw != NULL);
	fw->fw = NULL;
	fw->fw_blocks = 0;
	/* Should not crash */
	free(fw->fw);
	free(fw);
}

/* ── main ───────────────────────────────────────────────────────── */

int main(void)
{
	printf("Running unit tests...\n\n");

	printf("util.c:\n");
	TEST(test_ascii_to_nibble_digits);
	TEST(test_ascii_to_nibble_upper);
	TEST(test_ascii_to_nibble_lower);
	TEST(test_ascii_to_nibble_invalid);
	TEST(test_nibble_to_ascii_all);
	TEST(test_nibble_to_ascii_masked);
	TEST(test_validate_nibble_valid);
	TEST(test_validate_nibble_invalid);
	TEST(test_validate_nibble_boundaries);
	TEST(test_ascii_nibble_roundtrip);

	printf("\nhm.h macros:\n");
	TEST(test_src_dst_roundtrip);
	TEST(test_src_dst_zero);
	TEST(test_src_dst_max);
	TEST(test_src_dst_independent);
	TEST(test_payloadlen_roundtrip);
	TEST(test_hm_field_offsets);

	printf("\nfirmware.c (crc16):\n");
	TEST(test_crc16_empty);
	TEST(test_crc16_deterministic);
	TEST(test_crc16_different_data);
	TEST(test_crc16_different_init);
	TEST(test_crc16_not_mutate_input);

	printf("\nfirmware.c (firmware_read):\n");
	TEST(test_firmware_read_eq3);
	TEST(test_firmware_read_ihex);
	TEST(test_firmware_free_null_blocks);

	printf("\n%d/%d tests passed.\n", tests_passed, tests_run);

	return (tests_passed == tests_run) ? 0 : 1;
}
