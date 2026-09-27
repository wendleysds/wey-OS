#ifndef __AML_NAMESPACE_H__
#define __AML_NAMESPACE_H__

#include "types.h"

int acpi_load_namespace(const uint8_t *aml, size_t length);
void acpi_unload_namespace(void);

void acpi_namespace_walk(void (*callback)(aml_object_t*));

#endif