#include "../internal/stream.h"
#include <mm/kheap.h>

static void add_device_resource(aml_object_t *obj, acpi_resource_t *res) {
	if (!obj || !res || obj->type != AML_OBJ_TYPE_DEVICE) {
		return;
	}

	res->next = NULL;
	if (obj->device.resources == NULL) {
		obj->device.resources = res;
	} else {
		acpi_resource_t *curr = obj->device.resources;
		while (curr->next != NULL) {
			curr = curr->next;
		}
		curr->next = res;
	}
}

bool aml_parse_crs_buffer(aml_stream_t *s, aml_object_t *obj) {
	while (aml_stream_has_bytes(s, 1)) {
		uint8_t tag;
		if (!aml_stream_consume_u8(s, &tag)){
			return false;
		}

		const uint8_t *ptr = s->curr;
		if ((tag & 0x80) == 0) {
			uint8_t item_type = (tag >> 3) & 0x0F;
			uint8_t item_len = tag & 0x07;
			if (item_type == 0x04 && item_len >= 2) {
				acpi_resource_t *res = kzalloc(sizeof(*res));
				if (res) {
					res->type = ACPI_RESOURCE_TYPE_IRQ;
					res->irq_mask = aml_read_u16(ptr);
					add_device_resource(obj, res);
				}
			} else if (item_type == 0x08 && item_len >= 7) {
				acpi_resource_t *res = kzalloc(sizeof(*res));
				if (res) {
					res->type = ACPI_RESOURCE_TYPE_IO;
					res->io.base = aml_read_u16(ptr + 1);
					res->io.length = aml_read_u8(ptr + 6);
					res->io.alignment = aml_read_u8(ptr + 5);
					add_device_resource(obj, res);
				}
			} else if (item_type == 0x09 && item_len >= 3) {
				acpi_resource_t *res = kzalloc(sizeof(*res));
				if (res) {
					res->type = ACPI_RESOURCE_TYPE_IO;
					res->io.base = aml_read_u16(ptr);
					res->io.length = aml_read_u8(ptr + 2);
					res->io.alignment = 1;
					add_device_resource(obj, res);
				}
			} else if (item_type == 0x0F) {
				s->curr = ptr;
				break;
			}

			s->curr += item_len;
		} else {
			uint8_t item_type = tag & 0x7F;
			uint16_t item_len;
			if (!aml_stream_consume_u16(s, &item_len))
				break;

			ptr = s->curr;

			if (!aml_stream_has_bytes(s, item_len)) {
                break; 
            }

			if (item_type == 0x06 && item_len >= 9) {
				acpi_resource_t *res = kzalloc(sizeof(*res));
					if (res) {
					res->type = ACPI_RESOURCE_TYPE_MMIO;
					res->mmio.base = aml_read_u32(ptr + 1);
					res->mmio.length = aml_read_u32(ptr + 5);
					res->mmio.writeable = true;
					add_device_resource(obj, res);
				}
			} else if (item_type == 0x08 && item_len >= 13) {
				acpi_resource_t *res = kzalloc(sizeof(*res));
				if (res) {
					if (ptr[0] == 1) {
						res->type = ACPI_RESOURCE_TYPE_IO;
						res->io.base = aml_read_u16(ptr + 6);
						res->io.length = aml_read_u16(ptr + 12);
						res->io.alignment = 1;
					} else {
						res->type = ACPI_RESOURCE_TYPE_MMIO;
						res->mmio.base = aml_read_u16(ptr + 6);
						res->mmio.length = aml_read_u16(ptr + 12);
						res->mmio.writeable = true;
					}
					add_device_resource(obj, res);
				}
			} else if (item_type == 0x07 && item_len >= 23) {
				acpi_resource_t *res = kzalloc(sizeof(*res));
				if (res) {
					if (ptr[0] == 1) {
						res->type = ACPI_RESOURCE_TYPE_IO;
						res->io.base = (uint16_t)aml_read_u32(ptr + 10);
						res->io.length = (uint16_t)aml_read_u32(ptr + 22);
						res->io.alignment = 1;
					} else {
						res->type = ACPI_RESOURCE_TYPE_MMIO;
						res->mmio.base = aml_read_u32(ptr + 10);
						res->mmio.length = aml_read_u32(ptr + 22);
						res->mmio.writeable = true;
					}
					add_device_resource(obj, res);
				}
			} else if (item_type == 0x0A && item_len >= 43) {
				acpi_resource_t *res = kzalloc(sizeof(*res));
				if (res) {
					if (ptr[0] == 1) {
						res->type = ACPI_RESOURCE_TYPE_IO;
						res->io.base = (uint16_t)aml_read_u64(ptr + 14);
						res->io.length = (uint16_t)aml_read_u64(ptr + 38);
						res->io.alignment = 1;
					} else {
						res->type = ACPI_RESOURCE_TYPE_MMIO;
						res->mmio.base = aml_read_u64(ptr + 14);
						res->mmio.length = aml_read_u64(ptr + 38);
						res->mmio.writeable = true;
					}
					add_device_resource(obj, res);
				}
			}
			s->curr += item_len;
		}
	}

	return true;
}
