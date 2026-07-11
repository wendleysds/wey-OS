#include <kernel/input.h>
#include <sync/spinlock.h>
#include <kernel/init.h>
#include <def/errno.h>

static LIST_HEAD(handlers);
static spinlock_t lock;

int input_register_handler(struct input_handler *handler){
	spin_lock(&lock);
	INIT_LIST_HEAD(&handler->list);
	list_add(&handler->list, &handlers);
	spin_unlock(&lock);
	return SUCCESS;
}

void input_unregister_handler(struct input_handler *handler){
	spin_lock(&lock);
	list_remove(&handler->list);
	INIT_LIST_HEAD(&handler->list);
	spin_unlock(&lock);
}

void input_report(const struct input_event *event){
	spin_lock(&lock);

	struct input_handler* pos;
	list_for_each_entry(pos, &handlers, list){
		pos->event(pos, event);
	}

	spin_unlock(&lock);
}

static __init int input_init(void){
	INIT_LIST_HEAD(&handlers);
	spinlock_init(&lock);
	return OK;
}

subsys_initcall(input_init);
