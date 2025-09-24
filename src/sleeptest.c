#include <drivers/keyboard.h>
#include <drivers/terminal.h>
#include <core/sched/task.h>
#include <core/sched.h>
#include <def/err.h>
#include <syscall.h>

// Blocking IO Test
// Reading from keyboad

#define KEYBOARD_BUFFER_SIZE 128

static struct TaskQueue _queue;

static char kb_buffer[KEYBOARD_BUFFER_SIZE];
static int kb_tail;
static int kb_head;

static void _sleep_current(){
	struct Task* t = pcb_current();
	if(!t) return;

	t->state = TASK_WAITING;
	task_enqueue(&_queue, t);
	
	schedule();
}

static void _wakeup_last(){
	struct Task* t = task_dequeue(&_queue);
	if(!t) return;

	scheduler_add_task(t);
}

static char kb_pop(){
	if (kb_head == kb_tail) {
		return 0; // Buffer is empty
	}

	char c = kb_buffer[kb_tail];
	kb_tail = (kb_tail + 1) % KEYBOARD_BUFFER_SIZE;
	return c;
}

static void kb_handler(char c){
	int next = (kb_head + 1) % KEYBOARD_BUFFER_SIZE;

	if (next != kb_tail) { // Check if buffer is full
		kb_buffer[kb_head] = c;
		kb_head = next;

		_wakeup_last();
	}
}

void sleep_test(){
	memset(&_queue, 0x0, sizeof(struct TaskQueue));	
	memset(kb_buffer, 0x0, KEYBOARD_BUFFER_SIZE);
	kb_tail = 0;
	kb_head = 0;

	keyboard_set_callback(kb_handler);
}

SYSCALL_DEFINE2(kb_read, char*, buffer, int, count){
	if(!buffer || count < 1){
		return INVALID_ARG;
	}

	int read = 0;

	while(read < count){
		char c;
		while((c = kb_pop()) != 0){

			terminal_putchar(c, TERMINAL_DEFAULT_COLOR);

			if(c == '\n'){
				goto out;
			}

			buffer[read++] = c;
		}

		_sleep_current();
	}

out:

	buffer[read] = '\0';

	return count;
}
