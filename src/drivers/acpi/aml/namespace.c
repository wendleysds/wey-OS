#include <kernel/printk.h>
#include <lib/string.h>
#include <mm/kheap.h>

#include "internal/stream.h"
#include "internal/parser.h"
#include "lib/list.h"

static aml_node_t *root_node = NULL;

static int aml_parse_namespace_scope(const uint8_t *aml, size_t length, aml_node_t *scope);

static void aml_clean_object_contents(aml_object_t *obj) {
	if (!obj) return;

	acpi_resource_t *cur, *next;
	aml_object_t *pos, *n;

	switch (obj->type) {
		case AML_OBJ_TYPE_STRING:
			if (obj->string) {
				kfree((void*)obj->string);
			}
			break;

		case AML_OBJ_TYPE_PACKAGE:
			list_for_each_entry_safe(pos, n, &obj->package.list, node){
				list_remove(&pos->node);
				aml_clean_object_contents(pos);
				kfree(pos);
			}
			break;

		case AML_OBJ_TYPE_DEVICE: 
			cur = obj->device.resources;
			while (cur) {
				next = cur->next;
				kfree(cur);
				cur = next;
			}
			break;

		default:
			break;
	}
}

void aml_namespace_insert_node(aml_node_t *node, aml_node_t *parent){
	node->parent = parent;
	if (parent) {
		if (!parent->child) {
			parent->child = node;
		} else {
			aml_node_t *curr = parent->child;
			while (curr->peer)
				curr = curr->peer;
			curr->peer = node;
		}
	}
}

void aml_namespace_delete_node(aml_node_t *node){
    if(!node) return;

    if(node->child)
        aml_namespace_delete_node(node->child);

    aml_node_t* peer = node->peer;
    if(peer)
        aml_namespace_delete_node(peer);

    aml_clean_object_contents(&node->object);
    
    kfree(node);
}

aml_node_t *aml_namespace_create_node(const char *name) {
	aml_node_t *node = (aml_node_t *)kzalloc(sizeof(aml_node_t));
	if (!node) {
		return NULL;
	}

	if (name) {
		strncpy(node->name, name, ACPI_NODE_NAME_MAX);
		node->name[ACPI_NODE_NAME_MAX - 1] = '\0';
		node->object.name = node->name;
	}
	
	return node;
}

static bool aml_skip_package(aml_stream_t *s) {
	const uint8_t *pkg_start = s->curr;
	uint32_t pkg_len;
	if (!aml_parse_pkg_length(s, &pkg_len)) return false;

	const uint8_t *pkg_end = pkg_start + pkg_len;
	if (pkg_end > s->end) return false;

	s->curr = pkg_end;
	return true;
}


static bool acpi_populate_device(aml_node_t* dev){
	if(dev->object.type != AML_OBJ_TYPE_DEVICE) return false;

	aml_node_t *child = dev->child;
	while(child){
		if (strcmp(child->name, "_HID") == 0) {
			if (child->object.type == AML_OBJ_TYPE_INTEGER)
				aml_parse_eisa_id(child->object.integer, dev->object.device.hid);
			else if (child->object.type == AML_OBJ_TYPE_STRING)
				strncpy(dev->object.device.hid, child->object.string, 9);
		} else if (strcmp(child->name, "_CID") == 0) {
			if (child->object.type == AML_OBJ_TYPE_INTEGER)
				aml_parse_eisa_id(child->object.integer, dev->object.device.cid);
			else if (child->object.type == AML_OBJ_TYPE_STRING)
				strncpy(dev->object.device.cid, child->object.string, 9);
		} else if (strcmp(child->name, "_CRS") == 0){
			if (child->object.type == AML_OBJ_TYPE_BUFFER) {
				aml_stream_t s = {
					.curr = child->object.buffer.data,
					.end = child->object.buffer.data + child->object.buffer.length,
					.start = child->object.buffer.data
				};

				if (!aml_parse_crs_buffer(&s, &dev->object)) {
					printk("Failed to parse CRS buffer for device %s\n", dev->name);
				}
			} else if (child->object.type == AML_OBJ_TYPE_METHOD) {
				// Not supported
			}
		}

		child = child->peer;
	}

	return true;
}

static bool aml_parse_device(aml_stream_t *s, aml_node_t *scope) {
	const uint8_t *pkg_start = s->curr;
	uint32_t pkg_len;
	if (!aml_parse_pkg_length(s, &pkg_len)) return false;
	if (pkg_len == 0) return true;

	const uint8_t *pkg_end = pkg_start + pkg_len;
	if (pkg_end > s->end) return false;

	char name[ACPI_NODE_NAME_MAX] = {0};
	if (!aml_parse_name_string(s, name, ACPI_NODE_NAME_MAX)) return false;

	aml_node_t *node = aml_namespace_create_node(name);
	if (!node) return false;

	node->object.type = AML_OBJ_TYPE_DEVICE;

	aml_namespace_insert_node(node, scope);

	if (s->curr < pkg_end) {
		aml_parse_namespace_scope(s->curr, pkg_end - s->curr, node);
	}

	acpi_populate_device(node);

	s->curr = pkg_end;
	return true;
}

static bool aml_parse_method(aml_stream_t *s, aml_node_t *scope) {
	const uint8_t *pkg_start = s->curr;
	uint32_t pkg_len;
	if (!aml_parse_pkg_length(s, &pkg_len)) return false;
	if (pkg_len == 0) return true;

	const uint8_t *method_end = pkg_start + pkg_len;
	if (method_end > s->end) return false;

	char name[ACPI_NODE_NAME_MAX] = {0};
	if (!aml_parse_name_string(s, name, ACPI_NODE_NAME_MAX)) return false;

	uint8_t flags;
	if (!aml_stream_consume_u8(s, &flags)) return false;

	aml_node_t *node = aml_namespace_create_node(name);
	if (!node) return false;

	node->object.type = AML_OBJ_TYPE_METHOD;
	node->object.method.arg_count = flags & 0x07;
	node->object.method.aml_start = s->curr;
	node->object.method.aml_length = method_end - s->curr;

	s->curr = method_end;
	aml_namespace_insert_node(node, scope);
	return true;
}

static bool aml_parse_name(aml_stream_t *s, aml_node_t *scope) {
	char name[ACPI_NODE_NAME_MAX] = {0};

	if (!aml_parse_name_string(s, name, ACPI_NODE_NAME_MAX)) {
	return false;
	}

	aml_node_t *node = aml_namespace_create_node(name);
	if (!node) return false;

	if (!aml_parse_object(s, &node->object)) {
		goto out_clean;
	}

	aml_namespace_insert_node(node, scope);
	return true;

out_clean:
	if (node) aml_namespace_delete_node(node);
	return false;
}

static bool aml_parse_alias(aml_stream_t *s, aml_node_t *scope) {
	(void)scope;
	if (!aml_parse_name_string(s, NULL, 0)) return false;
	if (!aml_parse_name_string(s, NULL, 0)) return false;
	return true;
}

static bool aml_parse_regionop(aml_stream_t *s, aml_node_t *scope) {
	char regionName[ACPI_NODE_NAME_MAX] = {0};
    if (!aml_parse_name_string(s, regionName, ACPI_NODE_NAME_MAX)) return false;

    uint8_t space_byte;
    if (!aml_stream_consume_u8(s, &space_byte)) return false;

    aml_object_t offset_obj = {0};
    aml_object_t length_obj = {0};

    if (!aml_parse_object(s, &offset_obj)) return false;
    if (!aml_parse_object(s, &length_obj)) return false;

    uint64_t offset = (offset_obj.type == AML_OBJ_TYPE_INTEGER) ? offset_obj.integer : 0;
    uint64_t length = (length_obj.type == AML_OBJ_TYPE_INTEGER) ? length_obj.integer : 0;

    if (offset_obj.type == AML_OBJ_TYPE_STRING && offset_obj.string) {
        kfree((void*)offset_obj.string);
    }
    if (length_obj.type == AML_OBJ_TYPE_STRING && length_obj.string) {
        kfree((void*)length_obj.string);
    }

    aml_node_t *region_node = aml_namespace_create_node(regionName);
    if (!region_node) return false;

    region_node->object.type = AML_OBJ_TYPE_REGION;
    region_node->object.region.space_byte = space_byte;
    region_node->object.region.offset = offset;
    region_node->object.region.length = length;

    aml_namespace_insert_node(region_node, scope);
    return true;
}

static int aml_parse_namespace_scope(const uint8_t *aml, size_t length, aml_node_t *scope) {
	aml_stream_t stream = { .curr = aml, .end = aml + length, .start = aml };
	aml_stream_t *s = &stream;

	while (s->curr < s->end) {
		enum aml_opcode opcode = aml_stream_consume_opcode(s);
		bool result = false;

		switch (opcode) {
			case DeviceOpList: 
				result = aml_parse_device(s, scope); 
				break;
			case MethodOp: 
				result = aml_parse_method(s, scope); 
				break;
			case ScopeOp: {
				const uint8_t *pkg_start = s->curr;
				uint32_t pkg_len;
				if (!aml_parse_pkg_length(s, &pkg_len)){
					result = false;
					break;
				}

				const uint8_t *scope_end = pkg_start + pkg_len;
				if (scope_end > s->end){
					result = false;
					break;
				}

				char name[ACPI_NODE_NAME_MAX] = {0};
				if (!aml_parse_name_string(s, name, ACPI_NODE_NAME_MAX)){
					result = false;
					break;
				}

				aml_node_t *node = aml_namespace_create_node(name);
				if (!node) {
					result = false;
					break;
				}

				aml_namespace_insert_node(node, scope);
				node->object.type = AML_OBJ_TYPE_SCOPE;

				if (s->curr < scope_end) {
					aml_parse_namespace_scope(s->curr, scope_end - s->curr, node);
				}

				s->curr = scope_end;
				result = true;
				break;
			}
			case NameOp: 
				result = aml_parse_name(s, scope); 
				break;
			case AliasOp: 
				result = aml_parse_alias(s, scope); 
				break;
			case OpRegionOp: 
				result = aml_parse_regionop(s, scope); 
				break;
			case MutexOp: {
				result = aml_parse_name_string(s, NULL, 0);
				if (!result) break;
				result = aml_stream_consume_u8(s, NULL);
			}
			break;

			default:
				switch (opcode) {
					case FieldOp:
					case ThermalZoneOpList:
					case BankFieldOp:
					case IndexFieldOp:
					case ProcessorOp:
					case PowerResOp:
					case IfOp:
					case ElseOp:
					case WhileOp:
					case BufferOp:
					case PackageOp:
					case VarPackageOp:
						if (!aml_skip_package(s)) return -1;
						result = true;
						break;
					default:
						printk("Unknown opcode: %#x\n", opcode);
						return 1;
				}
				break;
		}

		if (!result){
			printk("Failed to parse opcode: %#x\n", opcode);
			return 1;
		}
	}

	return 0;
}

int acpi_load_namespace(const uint8_t *aml, size_t length) {
	root_node = aml_namespace_create_node("\\");
	if (!root_node) return 1;

	aml_parse_namespace_scope(aml, length, root_node);
	return 0;
}

void acpi_unload_namespace(void) {
	aml_namespace_delete_node(root_node);
	root_node = NULL;
}

static void acpi_namespace_walk_recursive(aml_node_t *node, void (*callback)(aml_object_t*)){
	if(!node) return;

	callback(&node->object);

	acpi_namespace_walk_recursive(node->peer, callback);
	acpi_namespace_walk_recursive(node->child, callback);
}

void acpi_namespace_walk(void (*callback)(aml_object_t*)){
	acpi_namespace_walk_recursive(root_node, callback);
}