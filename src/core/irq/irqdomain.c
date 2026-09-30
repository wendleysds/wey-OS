#include <kernel/interrupt.h>
#include <lib/string.h>
#include <lib/list.h>
#include <mm/kheap.h>
#include <def/errno.h>

static LIST_HEAD(irq_domain_list);
static struct irq_domain *default_irq_domain = NULL;

int irq_domain_register(struct irq_domain *domain) {
	if (!domain) {
		return -EINVAL;
	}

	INIT_LIST_HEAD(&domain->list);
	list_add_tail(&domain->list, &irq_domain_list);

	if (!default_irq_domain) {
		default_irq_domain = domain;
	}

	return 0;
}

void irq_domain_unregister(struct irq_domain *domain) {
	if (!domain) {
		return;
	}

	list_remove(&domain->list);

	if (default_irq_domain == domain) {
		if (!list_empty(&irq_domain_list)) {
			default_irq_domain = list_first_entry(&irq_domain_list, struct irq_domain, list);
		} else {
			default_irq_domain = NULL;
		}
	}
}

struct irq_domain *irq_domain_get_default(void) {
	return default_irq_domain;
}

void irq_domain_set_default(struct irq_domain *domain) {
	default_irq_domain = domain;
}

struct irq_domain *irq_domain_find_by_name(const char *name) {
	if (!name) {
		return NULL;
	}

	struct irq_domain *dom;
	list_for_each_entry(dom, &irq_domain_list, list) {
		if (dom->name && strcmp(dom->name, name) == 0) {
			return dom;
		}
	}

	return NULL;
}

static int legacy_domain_map(struct irq_domain *d, unsigned int hwirq, unsigned int *irq) {
	if (hwirq < d->hwirq_base || hwirq >= d->hwirq_base + d->nr_irqs) {
		return -EINVAL;
	}

	*irq = d->irq_base + (hwirq - d->hwirq_base);
	return 0;
}

static void legacy_domain_unmap(struct irq_domain *d, unsigned int hwirq) {
	// No dynamic unmapping needed for legacy domains
}

struct irq_domain *irq_domain_create_legacy(
	const char *name,
	unsigned int nr_irqs,
	unsigned int first_hwirq,
	unsigned int first_irq,
	const struct irq_chip *chip,
	const struct irq_controller *controller
) {
	// Check if already registered
	struct irq_domain *existing = irq_domain_find_by_name(name);
	if (existing) {
		irq_domain_set_chip(existing, chip, controller);
		return existing;
	}

	struct irq_domain *domain = kmalloc(sizeof(struct irq_domain));
	if (!domain) {
		return NULL;
	}

	memset(domain, 0, sizeof(struct irq_domain));
	domain->name = name;
	domain->nr_irqs = nr_irqs;
	domain->hwirq_base = first_hwirq;
	domain->irq_base = first_irq;
	domain->chip = chip;
	domain->controller = controller;
	domain->map = legacy_domain_map;
	domain->unmap = legacy_domain_unmap;

	irq_domain_register(domain);

	// Initialize descriptors in domain range
	for (unsigned int i = 0; i < nr_irqs; i++) {
		unsigned int hwirq = first_hwirq + i;
		unsigned int irq = first_irq + i;
		struct irq_desc *desc = irq_desc_get_by_hwirq(hwirq);
		if (desc) {
			desc->irq = irq;
			desc->hwirq = hwirq;
			desc->domain = domain;
			desc->chip = chip;
			desc->controller = controller;
		}
	}

	return domain;
}

struct irq_domain *irq_domain_create_linear(
	const char *name,
	unsigned int nr_irqs,
	const struct irq_chip *chip,
	const struct irq_controller *controller
) {
	return irq_domain_create_legacy(name, nr_irqs, 0x20, 0, chip, controller);
}

int irq_domain_set_chip(
	struct irq_domain *domain,
	const struct irq_chip *chip,
	const struct irq_controller *controller
) {
	if (!domain) {
		return -EINVAL;
	}

	domain->chip = chip;
	domain->controller = controller;

	for (unsigned int i = 0; i < domain->nr_irqs; i++) {
		unsigned int hwirq = domain->hwirq_base + i;
		struct irq_desc *desc = irq_desc_get_by_hwirq(hwirq);
		if (desc) {
			desc->chip = chip;
			desc->controller = controller;
		}
	}

	return 0;
}

int irq_domain_map(struct irq_domain *domain, unsigned int hwirq, unsigned int *irq) {
	if (!domain) {
		domain = default_irq_domain;
	}
	if (!domain || !domain->map) {
		return -ENODEV;
	}
	return domain->map(domain, hwirq, irq);
}

void irq_domain_unmap(struct irq_domain *domain, unsigned int hwirq) {
	if (!domain) {
		domain = default_irq_domain;
	}
	if (domain && domain->unmap) {
		domain->unmap(domain, hwirq);
	}
}

int irq_find_mapping(struct irq_domain *domain, unsigned int hwirq) {
	unsigned int irq = 0;
	if (irq_domain_map(domain, hwirq, &irq) == 0) {
		return (int)irq;
	}
	return -1;
}

int irq_create_mapping(struct irq_domain *domain, unsigned int hwirq) {
	return irq_find_mapping(domain, hwirq);
}
