#include <kernel/wait.h>
#include <lib/assert.h>

int __wake_up(struct wait_queue_head *head, int count) {
	unsigned long flags;
	int start = count;

	spin_lock_irqsave(&head->lock, &flags);
	if (list_empty(&head->entries)){
		goto out;
		return count;
	}

	struct wait_queue_entry *curr, *next;
	curr = list_first_entry(&head->entries, struct wait_queue_entry, nodes);
	list_for_each_entry_safe_from(curr, next, &head->entries, nodes) {
		BUG_ON(!curr->callback);

		int ret = curr->callback(curr);
		if (ret < 0) break;
		if (ret && !--count) break;
	}

out:
	spin_unlock_irqrestore(&head->lock, &flags);
	return start - count; // remaining
}

void wait_queue_add(struct wait_queue_head *head, struct wait_queue_entry *new_entry) {
	unsigned long flags;

	spin_lock_irqsave(&head->lock, &flags);
	list_add_tail(&new_entry->nodes, &head->entries);
	spin_unlock_irqrestore(&head->lock, &flags);
}

void wait_queue_remove(struct wait_queue_head *head, struct wait_queue_entry *entry){
	unsigned long flags;

	spin_lock_irqsave(&head->lock, &flags);
	list_remove(&entry->nodes);
	spin_unlock_irqrestore(&head->lock, &flags);
}
