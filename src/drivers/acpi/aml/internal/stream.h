#ifndef __AML_INTERNAL_STREAM_H__
#define __AML_INTERNAL_STREAM_H__

#include <stdint.h>
#include <stdbool.h>
#include "opcodes.h"
#include "types.h"

static inline uint8_t aml_read_u8(const uint8_t *p) {
	return p[0];
}

static inline uint16_t aml_read_u16(const uint8_t *p) {
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static inline uint32_t aml_read_u32(const uint8_t *p) {
	uint32_t value = 0;
	for (int i = 0; i < 4; i++)
		value |= (uint32_t)p[i] << (i * 8);
	return value;
}

static inline uint64_t aml_read_u64(const uint8_t *p) {
	uint64_t value = 0;
	for (int i = 0; i < 8; i++)
		value |= (uint64_t)p[i] << (i * 8);
	return value;
}

static inline bool aml_stream_has_bytes(const aml_stream_t *s, size_t bytes) {
	return (s->curr + bytes <= s->end);
}

static inline bool aml_stream_consume_u8(aml_stream_t *s, uint8_t *out) {
	if (!aml_stream_has_bytes(s, 1)) return false;
	if(out) *out = aml_read_u8(s->curr);
	s->curr++;
	return true;
}

static inline bool aml_stream_consume_u16(aml_stream_t *s, uint16_t *out) {
	if (!aml_stream_has_bytes(s, 2)) return false;
	if(out) *out = aml_read_u16(s->curr);
	s->curr += 2;
	return true;
}

static inline bool aml_stream_consume_u32(aml_stream_t *s, uint32_t *out) {
	if (!aml_stream_has_bytes(s, 4)) return false;
	if(out) *out = aml_read_u32(s->curr);
	s->curr += 4;
	return true;
}

static inline bool aml_stream_consume_u64(aml_stream_t *s, uint64_t *out) {
	if (!aml_stream_has_bytes(s, 8)) return false;
	if(out) *out = aml_read_u64(s->curr);
	s->curr += 8;
	return true;
}

static inline uint8_t aml_stream_read_u8(aml_stream_t *s) {
	return aml_read_u8(s->curr);
}

static inline uint16_t aml_stream_read_u16(aml_stream_t *s) {
	return aml_read_u16(s->curr);
}

static inline uint32_t aml_stream_read_u32(aml_stream_t *s) {
	return aml_read_u32(s->curr);
}

static inline uint64_t aml_stream_read_u64(aml_stream_t *s) {
	return aml_read_u64(s->curr);
}

enum aml_opcode aml_stream_consume_opcode(aml_stream_t *s);
enum aml_opcode aml_stream_read_opcode(aml_stream_t *s);

#endif