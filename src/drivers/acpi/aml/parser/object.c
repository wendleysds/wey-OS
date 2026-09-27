#include "../internal/stream.h"
#include "../internal/parser.h"
#include <lib/string.h>
#include <mm/kheap.h>

bool aml_parse_object(aml_stream_t *s, aml_object_t *obj) {
	enum aml_opcode op = aml_stream_read_opcode(s);

	if (opcode_is_digit(op)) {
		uint64_t value;
		if (!aml_parse_integer(s, &value)) return false;
		obj->type = AML_OBJ_TYPE_INTEGER;
		obj->integer = value;
		return true;
	} else if (op == StringPrefix) {
		aml_stream_consume_opcode(s); 
		const uint8_t *tmp = s->curr;
		while (s->curr < s->end && *s->curr != '\0') s->curr++;
		if (s->curr >= s->end) return false;

		size_t str_size = s->curr - tmp;
		char *str = (char *)kmalloc(str_size + 1);
		if (!str) return false;

		memcpy(str, tmp, str_size);
		str[str_size] = '\0';
		
		s->curr++;
		obj->type = AML_OBJ_TYPE_STRING;
		obj->string = str;
		return true;
	} else if (op == BufferOp || op == PackageOp || op == VarPackageOp) {
		aml_stream_consume_opcode(s); 

		const uint8_t *pkg_start = s->curr;
		uint32_t pkg_len;
		if (!aml_parse_pkg_length(s, &pkg_len)) return false;

		const uint8_t *pkg_end = pkg_start + pkg_len;
		if (pkg_end > s->end) return false;

		if (op == BufferOp) {
			if (!aml_parse_integer(s, NULL)) return false;
			obj->type = AML_OBJ_TYPE_BUFFER;
			obj->buffer.data = s->curr;
			obj->buffer.length = pkg_end - s->curr;
			s->curr = pkg_end;
			return true;
		} else {
			obj->type = AML_OBJ_TYPE_PACKAGE;
			
			if (op == PackageOp) {
				uint8_t declared_count;
				aml_stream_consume_u8(s, &declared_count);
			} else { // VarPackageOp
				uint64_t var_count;
				aml_parse_integer(s, &var_count);
			}

			INIT_LIST_HEAD(&obj->package.list);
			obj->package.count = 0;

			while (s->curr < pkg_end) {
				aml_object_t *elem = kzalloc(sizeof(aml_object_t));
				if(!elem) break;
				
				INIT_LIST_HEAD(&elem->node);
				
				if (!aml_parse_object(s, elem)) {
					kfree(elem);
					break; 
				}

				list_add_tail(&elem->node, &obj->package.list);
				obj->package.count++;
			}

			s->curr = pkg_end; 
			return true;
		}
	} else if (opcode_is_char(op)) {
		char ref_name[ACPI_NODE_NAME_MAX] = {0};
		if (!aml_parse_name_string(s, ref_name, ACPI_NODE_NAME_MAX)) return false;
		
		obj->type = AML_OBJ_TYPE_STRING; 
		char *str = (char *)kmalloc(strlen(ref_name) + 1);
		if (str) {
			strcpy(str, ref_name);
			obj->string = str;
		}
		return true;
	}
	
	return false; 
}
