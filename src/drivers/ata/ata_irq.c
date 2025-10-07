#include <wey/sched.h>
#include <drivers/ata.h>
#include <arch/i386/idt.h>
#include <arch/i386/pic.h>
#include <io/ports.h>
#include <def/err.h>

#include "ata_internal.h"

static void _ata_irq_handler(struct InterruptFrame* frame){
	struct ATAChannel* channel = (frame->int_no == ATA_PRIMARY_IRQ)
		? &_ata_primary
		: &_ata_secondary;

	// Clear drive irq
	(void)inb_p(ATA_IO(channel, ATA_REG_STATUS));

	if(channel->active){
		channel->active->irqTriggered = 1;

		struct Task* t = 0x0;
		while((t = task_dequeue(&channel->active->sleepQueue))){
			t->state = TASK_READY;
			scheduler_add_task(t);
		}
	}
}

static int8_t _ata_polling(struct ATADevice* atadev){
	struct ATAChannel* ch = atadev->channel;
	
	int i;
	for (i = 0; i < TRIES; i++)
	{
		uint8_t status = ata_status(atadev);
		if (status & ATA_SR_ERR) return -inb_p(ATA_IO(ch, ATA_REG_ERROR));
		if (status & ATA_SR_DRQ) return OK;
	}

	return TIMEOUT;
}

void ata_register_irq(char channel){
	uint8_t irq = (channel == 0 ? ATA_PRIMARY_IRQ : ATA_SECONDARY_IRQ);

	idt_register_callback(irq, _ata_irq_handler);
	IRQ_clear_mask(irq);
}

int ata_wait_irq(struct ATADevice* atadev){
	struct Task* t = pcb_current();

	if(t && t->tid != 0){
		t->state = TASK_WAITING;
		task_enqueue(&atadev->sleepQueue, t);

		while(!atadev->irqTriggered){
			schedule();
		}

		return OK;
	}

	return _ata_polling(atadev);
}
