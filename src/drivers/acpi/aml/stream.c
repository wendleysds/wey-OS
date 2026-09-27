#include "internal/stream.h"

enum aml_opcode aml_stream_consume_opcode(aml_stream_t *s) {
	if (!s || !s->curr) {
		return DecodeFailed;
	}

	uint8_t first_byte;
	if (!aml_stream_consume_u8(s, &first_byte)) {
		return DecodeFailed;
	}

	if (first_byte == ExtOpPrefix) {
		uint8_t second_byte;
		if (!aml_stream_consume_u8(s, &second_byte)) {
			return DecodeFailed;
		}

		uint16_t extended_opcode = (first_byte << 8) | second_byte;
		return (enum aml_opcode)extended_opcode;
	}

	if (first_byte == 0x92) {
		uint8_t second_byte;
		if (!aml_stream_consume_u8(s, &second_byte)) {
			return DecodeFailed;
		}

		if (second_byte >= 0x93 && second_byte <= 0x95) {
			uint16_t extended_opcode = (first_byte << 8) | second_byte;
			return (enum aml_opcode)extended_opcode;
		}
	}

	return (enum aml_opcode)first_byte;
}

enum aml_opcode aml_stream_read_opcode(aml_stream_t *s) {
	const uint8_t *tmp = s->curr;
	enum aml_opcode opcode = aml_stream_consume_opcode(s);
	s->curr = tmp;
	return opcode;
}
