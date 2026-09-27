#ifndef __AML_INTERNAL_HELPERS_H__
#define __AML_INTERNAL_HELPERS_H__

#include "types.h"

void aml_parse_eisa_id(uint32_t eisa_id, char out_str[8]);
bool aml_parse_pkg_length(aml_stream_t *s, uint32_t *out_len);
bool aml_parse_crs_buffer(aml_stream_t *s, aml_object_t *obj);
bool aml_parse_object(aml_stream_t *s, aml_object_t *obj);
bool aml_parse_integer(aml_stream_t *s, uint64_t *out_val);
bool aml_parse_name_segment(aml_stream_t *s, char *out_name);
bool aml_parse_name_string(aml_stream_t *s, char *out_name, size_t max_len);

#endif