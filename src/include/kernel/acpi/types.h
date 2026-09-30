#ifndef __ACPI_TYPES_H__
#define __ACPI_TYPES_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <lib/list.h>

#define ACPI_NODE_NAME_MAX 5

typedef enum {
	AML_OBJ_TYPE_UNINITIALIZED,
	AML_OBJ_TYPE_INTEGER,
	AML_OBJ_TYPE_STRING,
	AML_OBJ_TYPE_BUFFER,
	AML_OBJ_TYPE_PACKAGE,
	AML_OBJ_TYPE_DEVICE,
	AML_OBJ_TYPE_METHOD,
	AML_OBJ_TYPE_SCOPE,
	AML_OBJ_TYPE_REGION,
} aml_object_type_t;

typedef enum {
	ACPI_RESOURCE_TYPE_IO,
	ACPI_RESOURCE_TYPE_MMIO,
	ACPI_RESOURCE_TYPE_IRQ,
} acpi_resource_type_t;

typedef struct {
	uint16_t base;
	uint16_t length;
	uint8_t alignment;
} acpi_resource_io_t;

typedef struct {
	uint64_t base;
	uint64_t length;
	bool writeable;
} acpi_resource_mmio_t;

typedef struct acpi_resource {
	acpi_resource_type_t type;
	union {
		acpi_resource_io_t io;
		acpi_resource_mmio_t mmio;
		uint32_t irq_mask;
	};
	struct acpi_resource *next;
} acpi_resource_t;

typedef struct {
	const uint8_t *curr;
	const uint8_t *end;
	const uint8_t *start;
} aml_stream_t;

typedef struct aml_object {
	aml_object_type_t type;
	const char *name;
	struct list_head node;
	union {
		uint64_t integer;
		const char* string;
		struct {
			const uint8_t *data;
			size_t length;
		} buffer;
		struct {
			struct list_head list;
			size_t count;
		} package;
		struct {
			uint8_t arg_count;
			const uint8_t *aml_start;
			uint32_t aml_length;
		} method;
		struct {
			uint8_t space_byte;
			uint64_t offset;
			uint64_t length;
		} region;
		struct {
			char name[5];
  			char hid[9];
  			char cid[9];
  			acpi_resource_t *resources;
		} device;
	};
} aml_object_t;

typedef struct aml_node {
	char name[ACPI_NODE_NAME_MAX];
	struct aml_node *parent;
	struct aml_node *child;
	struct aml_node *peer;
	aml_object_t object;
} aml_node_t;

#endif