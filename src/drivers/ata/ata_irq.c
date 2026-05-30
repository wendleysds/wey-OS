#include <wey/sched.h>
#include <wey/interrupt.h>
#include <drivers/ata.h>
#include <io/ports.h>
#include <def/err.h>

#include "ata_internal.h"

static void ata_irq_handler(struct irq_info* info){
	struct ATAChannel* channel = (struct ATAChannel*)info->device;

	// Clear drive irq
	(void)inb_p(ATA_IO(channel, ATA_REG_STATUS));

	spin_lock(&channel->spinlock);

	struct ATADevice* dev = channel->active;

	if(!dev){
		goto out;
	}

	dev->irqTriggered = 1;

	struct task* task;
    list_for_each_entry(task, &dev->sleepQueue, queue) {
        task->state = TASK_READY;
        list_remove(&task->queue);
        scheduler_add(task);
    }

	channel->active = NULL;

out:
	spin_unlock(&channel->spinlock);
}

static int8_t ata_polling(struct ATADevice* atadev){
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

	irq_register(irq, ata_irq_handler, atachannel);
	irq_unmask(irq);
	outb(ATA_IO(atachannel, ATA_REG_CONTROL), 0x00);
}

int ata_wait_irq(struct ATADevice* atadev){
	struct task* t = current;
	struct ATAChannel* channel = atadev->channel;

	if (!t || t->pid == 0)
		return ata_polling(atadev);

	spin_lock(&channel->spinlock);

	uint8_t status = ata_status(atadev);
	if (status & ATA_SR_DRQ) {
		spin_unlock(&channel->spinlock);
		return SUCCESS;
	}

	channel->active = atadev;

	scheduler_remove(t);
	t->state = TASK_BLOCKED;
	list_add(&t->queue, &atadev->sleepQueue);

	spin_unlock(&channel->spinlock);

	while (1) {
		schedule();

		spin_lock(&channel->spinlock);
		if (atadev->irqTriggered) {
			atadev->irqTriggered = 0;
			spin_unlock(&channel->spinlock);
			break;
		}
		spin_unlock(&channel->spinlock);
	}

	return OK;
}
