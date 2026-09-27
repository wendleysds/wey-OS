#include "../internal/stream.h"
#include <lib/string.h>

bool aml_parse_pkg_length(aml_stream_t *s, uint32_t *out_len) {
	uint8_t lead_byte;
	if (!aml_stream_consume_u8(s, &lead_byte)) {
		return false;
	}

	uint8_t byte_count = (lead_byte >> 6) & 0x03;
	if (!aml_stream_has_bytes(s, byte_count))
		return false;

	uint32_t length = 0;
	if (byte_count == 0) {
		length = lead_byte & 0x3F;
	} else {
		length = lead_byte & 0x0F;
		for (uint8_t i = 0; i < byte_count; i++) {
			length |= ((uint32_t)(*s->curr)) << (4 + (i * 8));
			s->curr++;
		}
	}

	if (out_len) *out_len = length;
	return true;
}

bool aml_parse_integer(aml_stream_t *s, uint64_t *out_val) {
	enum aml_opcode op = aml_stream_consume_opcode(s);

	if(op != ZeroOp && op != OneOp && op != OnesOp && op != BytePrefix && op != WordPrefix && op != DWordPrefix && op != QWordPrefix) {
		return false;
	}

	uint64_t val = 0;
	if (op == ZeroOp || op == OneOp || op == OnesOp) {
		if (op == ZeroOp) {
			val = 0;
		} else if (op == OneOp) {
			val = 1;
		} else {
			val = 0xFFFFFFFFFFFFFFFFULL;
		}
	} else if (op == BytePrefix) {
		uint8_t u8;
		if (!aml_stream_consume_u8(s, &u8)) {
			return false;
		}
		val = u8;
	} else if (op == WordPrefix) {
		uint16_t u16;
		if (!aml_stream_consume_u16(s, &u16)) {
			return false;
		}
		val = u16;
	} else if (op == DWordPrefix) {
		uint32_t u32;
		if (!aml_stream_consume_u32(s, &u32)) {
			return false;
		}
		val = u32;
	} else if (op == QWordPrefix) {
		uint64_t u64;
		if (!aml_stream_consume_u64(s, &u64)) {
			return false;
		}
		val = u64;
	}

	if (out_val) *out_val = val;
	return true;
}

bool aml_parse_name_segment(aml_stream_t *s, char *out_name) {
	if (!aml_stream_has_bytes(s, 4)) {
		return false;
	}

	if (out_name) {
		memcpy(out_name, s->curr, 4);
		out_name[4] = '\0';
	}

	s->curr += 4;
	return true;
}

bool aml_parse_name_string(aml_stream_t *s, char *out_name, size_t max_len) {
	size_t w = 0;

	while (s->curr < s->end && (*s->curr == '\\' || *s->curr == '^')) {
		if (out_name && w + 1 < max_len) {
			out_name[w++] = *s->curr;
		}
		s->curr++;
	}

	if (s->curr >= s->end) return false;

	const uint8_t *before_op = s->curr;
	enum aml_opcode op = aml_stream_consume_opcode(s);
	if (op == DecodeFailed) return false;

	if (op == ZeroOp || op == 0x00) {
		if (out_name && max_len > 0) out_name[w] = '\0';
		return true;
	}

	if (op == DualNamePrefix) {
		for (int i = 0; i < 2; i++) {
			if (!aml_stream_has_bytes(s, 4)) return false;
			
			if (out_name && w + 4 < max_len) {
				memcpy(out_name + w, s->curr, 4);
				w += 4;
			}
			s->curr += 4;
			
			if (i == 0 && out_name && w + 1 < max_len) {
				out_name[w++] = '.';
			}
		}
		if (out_name && w < max_len) out_name[w] = '\0';
		return true;
	}

	if (op == MultiNamePrefix) {
		uint8_t seg_count;
		if (!aml_stream_consume_u8(s, &seg_count)) return false;

		if (seg_count == 0) {
			if (out_name && max_len > 0) out_name[w] = '\0';
			return true;
		}

		for (uint8_t i = 0; i < seg_count; i++) {
			if (!aml_stream_has_bytes(s, 4)) return false;
			
			if (out_name && w + 4 < max_len) {
				memcpy(out_name + w, s->curr, 4);
				w += 4;
			}
			s->curr += 4;

			if (i < seg_count - 1 && out_name && w + 1 < max_len) {
				out_name[w++] = '.';
			}
		}
		if (out_name && w < max_len) out_name[w] = '\0';
		return true;
	}

	s->curr = before_op;
	if (!aml_stream_has_bytes(s, 4)) return false;
	
	if (out_name && w + 4 < max_len) {
		memcpy(out_name + w, s->curr, 4);
		w += 4;
	}
	s->curr += 4;
	
	if (out_name && w < max_len) out_name[w] = '\0';
	return true;
}