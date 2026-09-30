#include <kernel/interrupt.h>
#include <def/errno.h>

void irq_set_chip(int irq, const struct irq_chip *chip) {
	struct irq_desc *desc = irq_to_desc(irq);
	if (desc) {
		desc->chip = chip;
	}
}

void irq_set_controller(int irq, const struct irq_controller *controller) {
	struct irq_desc *desc = irq_to_desc(irq);
	if (desc) {
		desc->controller = controller;
	}
}

void irq_set_chip_and_controller(int irq, const struct irq_chip *chip, const struct irq_controller *controller) {
	struct irq_desc *desc = irq_to_desc(irq);
	if (desc) {
		desc->chip = chip;
		desc->controller = controller;
	}
}

void interrupt_set_chip(int interrupt, const struct irq_chip* chip) {
	irq_set_chip(interrupt, chip);
}

void interrupt_set_controller(int interrupt, const struct irq_controller* controller) {
	irq_set_controller(interrupt, controller);
}

const struct irq_chip* irq_get_chip(int irq) {
	struct irq_desc *desc = irq_to_desc(irq);
	return desc ? desc->chip : NULL;
}

const struct irq_controller* irq_get_controller(int irq) {
	struct irq_desc *desc = irq_to_desc(irq);
	return desc ? desc->controller : NULL;
}

const struct irq_chip* interrupt_get_chip(int interrupt) {
	return irq_get_chip(interrupt);
}

const struct irq_controller* interrupt_get_controller(int interrupt) {
	return irq_get_controller(interrupt);
}

void irq_set_chip_data(int irq, void *data) {
	struct irq_desc *desc = irq_to_desc(irq);
	if (desc) {
		desc->chip_data = data;
	}
}

void *irq_get_chip_data(int irq) {
	struct irq_desc *desc = irq_to_desc(irq);
	return desc ? desc->chip_data : NULL;
}

int irq_set_trigger_type(int irq, enum irq_trigger_type type) {
	struct irq_desc *desc = irq_to_desc(irq);
	if (!desc) {
		return -EINVAL;
	}

	desc->type = type;
	if (desc->chip && desc->chip->set_type) {
		return desc->chip->set_type(desc, type);
	}

	return 0;
}

int irq_set_affinity(int irq, unsigned int cpu) {
	struct irq_desc *desc = irq_to_desc(irq);
	if (!desc) {
		return -EINVAL;
	}

	if (desc->chip && desc->chip->set_affinity) {
		return desc->chip->set_affinity(desc, cpu);
	}

	return -ENOSYS;
}

int irq_switch_chip(int irq, const struct irq_chip *new_chip, const struct irq_controller *new_controller) {
	struct irq_desc *desc = irq_to_desc(irq);
	if (!desc) {
		return -EINVAL;
	}

	bool was_unmasked = !desc->masked;

	// Mask on old chip if active
	if (was_unmasked && desc->chip && desc->chip->mask) {
		desc->chip->mask(desc);
	}

	desc->chip = new_chip;
	desc->controller = new_controller;

	// Unmask on new chip if it was previously unmasked
	if (was_unmasked && desc->chip && desc->chip->unmask) {
		desc->chip->unmask(desc);
	}

	return 0;
}

int irq_switch_all_chips(const struct irq_chip *new_chip, const struct irq_controller *new_controller) {
	for (int i = 0; i < TOTAL_INTERRUPTS; i++) {
		struct irq_desc *desc = irq_desc_get_by_hwirq(i);
		if (desc && (desc->chip || desc->handlers)) {
			bool was_unmasked = !desc->masked;

			if (was_unmasked && desc->chip && desc->chip->mask) {
				desc->chip->mask(desc);
			}

			desc->chip = new_chip;
			desc->controller = new_controller;

			if (was_unmasked && desc->chip && desc->chip->unmask) {
				desc->chip->unmask(desc);
			}
		}
	}

	return 0;
}
