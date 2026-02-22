#include <wey/sched.h>
#include <wey/interrupt.h>
#include <drivers/ata.h>
#include <io/ports.h>
#include <def/err.h>

#include "ata_internal.h"

static void _ata_irq_handler(struct irq_info* info){
	struct ATAChannel* channel = (struct ATAChannel*)info->device;

	// Clear drive irq
	(void)inb_p(ATA_IO(channel, ATA_REG_STATUS));

	if(channel->active){
		channel->active->irqTriggered = 1;
		struct task* t = 0x0;

		if(list_empty(&channel->active->sleepQueue)){
			return;
		}

		while((t = list_entry(channel->active->sleepQueue.next, struct task, queue))){
			t->state = TASK_READY;
			list_remove(&t->queue);
			scheduler_add(t);
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
	struct ATAChannel* atachannel = (channel == 0)
		? &_ata_primary
		: &_ata_secondary;

	if(atachannel->irqRegistered){
		return;
	}

	atachannel->irqRegistered = 1;

	uint8_t irq = (channel == 0 ? IRQ_ATA_PRIMARY : IRQ_ATA_SECONDARY);

	irq_register(irq, _ata_irq_handler, atachannel);
	irq_unmask(irq);
}

int ata_wait_irq(struct ATADevice* atadev){
	struct task* t = current;

	if(t && t->pid != 0){
		t->state = TASK_SLEEPING;
		scheduler_remove(t);
		list_add(&t->queue, &atadev->sleepQueue);

		while(!atadev->irqTriggered){
			schedule();
		}

		return OK;
	}

	return _ata_polling(atadev);
}
