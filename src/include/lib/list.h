#ifndef _LIST_H
#define _LIST_H

#include <stddef.h>

#define container_of(ptr, type, member) ({                  \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

#define list_next_entry(pos, member) \
	list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_entry_is_head(pos, head, member)\
	list_is_head(&pos->member, (head))

#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_entry(pos, head, member)                    \
	for (pos = list_entry((head)->next, typeof(*pos), member);    \
		&pos->member != (head);                                   \
		pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)       \
	for (pos = list_first_entry(head, typeof(*pos), member), \
		n = list_next_entry(pos, member);                    \
		!list_entry_is_head(pos, head, member);              \
		pos = n, n = list_next_entry(n, member))

#define list_for_each_entry_safe_from(pos, n, head, member) \
	for (n = list_next_entry(pos, member);                  \
		!list_entry_is_head(pos, head, member);             \
		pos = n, n = list_next_entry(n, member))

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

static inline void list_add_head(struct list_head *new, struct list_head *head){
	list_add(new, head);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head){
	list_add(new, head->prev);
}

static inline int list_is_head(const struct list_head *list, const struct list_head *head) {
	return list == head;
}

static inline int list_empty(struct list_head *head){
	return head->next == head;
}

#endif