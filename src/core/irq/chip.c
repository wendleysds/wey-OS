#include <kernel/interrupt.h>
#include <kernel/init.h>

#include <lib/list.h>

static LIST_HEAD(irqchips);

