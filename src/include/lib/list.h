#ifndef _LIST_H
#define _LIST_H

#define container_of(ptr, type, member) ({          \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_entry(pos, head, member)              \
	for (pos = list_entry((head)->next, typeof(*pos), member);    \
		&pos->member != (head);                    \
		pos = list_entry(pos->member.next, typeof(*pos), member))

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) \
    do { (ptr)->next = (ptr); (ptr)->prev = (ptr); } while (0)

struct list_head {
	struct list_head *next, *prev;
};

void list_add(struct list_head *new, struct list_head *head);
void list_remove(struct list_head *entry);

#endif