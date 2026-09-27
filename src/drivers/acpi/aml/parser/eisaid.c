#include <stdint.h>

void aml_parse_eisa_id(uint32_t eisa_id, char out_str[8]) {
	const uint8_t *b = (const uint8_t *)&eisa_id;
	uint32_t id = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
				((uint32_t)b[2] << 8) | (uint32_t)b[3];
	char c1 = '@' + ((id >> 26) & 0x1F);
	char c2 = '@' + ((id >> 21) & 0x1F);
	char c3 = '@' + ((id >> 16) & 0x1F);
	uint16_t hex_num = id & 0xFFFF;
	const char hex_chars[] = "0123456789ABCDEF";
	out_str[0] = c1;
	out_str[1] = c2;
	out_str[2] = c3;
	out_str[3] = hex_chars[(hex_num >> 12) & 0xF];
	out_str[4] = hex_chars[(hex_num >> 8) & 0xF];
	out_str[5] = hex_chars[(hex_num >> 4) & 0xF];
	out_str[6] = hex_chars[hex_num & 0xF];
	out_str[7] = '\0';
}