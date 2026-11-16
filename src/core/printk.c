#include <kernel/printk.h>
#include <kernel/clock.h>
#include <sync/spinlock.h>
#include <def/config.h>
#include <lib/string.h>
#include <lib/stdio.h>
#include <lib/div64.h>

static printk_echo_function printk_echo;

static char printk_text_buffer[PRINTK_BUFFER_SIZE];
static struct printk_record printk_records[PRINTK_RECORD_COUNT];

static DEFINE_SPINLOCK(printk_lock);
static DEFINE_SPINLOCK(printk_console_lock);

/* Monolithic cursor, not a physical offset of the ring. */
static uint64_t printk_text_cursor;
static uint64_t printk_sequence;
static uint32_t printk_record_head;  /* Next insertion position. */
static uint32_t printk_record_count;
static uint8_t printk_cont_level = LOG_LVL_INFO;

static struct printk_reader console_reader;

static inline uint32_t printk_record_index(uint32_t index){
    return index % PRINTK_RECORD_COUNT;
}

static inline struct printk_record *printk_oldest_record(void){
    if (printk_record_count == 0)
        return NULL;

    return &printk_records[printk_record_index(
        printk_record_head + PRINTK_RECORD_COUNT - printk_record_count
    )];
}

static inline struct printk_record *printk_last_record(void){
    if (printk_record_count == 0)
        return NULL;

    return &printk_records[printk_record_index(
        printk_record_head + PRINTK_RECORD_COUNT - 1
    )];
}

static inline void printk_drop_oldest_record(void){
    if (printk_record_count != 0)
        --printk_record_count;
}

static inline void printk_write_char(char c, void *ctx){
    (void)ctx;
    printk_text_buffer[printk_text_cursor % PRINTK_BUFFER_SIZE] = c;
    ++printk_text_cursor;
}

/*
 * One record is only valid if ALL of its text is still in the ring.
 * We cannot preserve only the suffix: this would make the reader present an
 * apparently intact message, but with the beginning silently corrupted.
 */
static void printk_expire_text_records(uint64_t new_cursor){
    uint64_t oldest_valid_cursor = 0;

    if (new_cursor > PRINTK_BUFFER_SIZE)
        oldest_valid_cursor = new_cursor - PRINTK_BUFFER_SIZE;

    while (printk_record_count != 0) {
        struct printk_record *record = printk_oldest_record();

        if (record->text_cursor >= oldest_valid_cursor)
            break;

        printk_drop_oldest_record();
    }
}

static struct printk_record *printk_alloc_record(void){
    struct printk_record *record;

    if (printk_record_count == PRINTK_RECORD_COUNT)
        printk_drop_oldest_record();

    record = &printk_records[printk_record_head];
    printk_record_head = printk_record_index(printk_record_head + 1);
    printk_record_count++;

    return record;
}

static int printk_parse_prefix(const char *fmt, uint8_t *level, uint8_t *flags){
    if (!fmt || fmt[0] != KLOG_ESCAPE_ASCII || fmt[1] == '\0')
        return 0;

    switch (fmt[1]) {
    case '0': *level = LOG_LVL_EMERG;   return 2;
    case '1': *level = LOG_LVL_ALERT;   return 2;
    case '2': *level = LOG_LVL_CRIT;    return 2;
    case '3': *level = LOG_LVL_ERR;     return 2;
    case '4': *level = LOG_LVL_WARNING; return 2;
    case '5': *level = LOG_LVL_NOTICE;  return 2;
    case '6': *level = LOG_LVL_INFO;    return 2;
    case '7': *level = LOG_LVL_DEBUG;   return 2;
    case 'c': *flags |= LOG_FLAG_CONT;  return 2;
    default:  return 0;
    }
}

static bool printk_can_merge_cont(
    const struct printk_record *last,
    uint64_t new_text_start,
    uint64_t new_text_len
){
    uint64_t last_char_pos;

    if (!last || last->text_len == 0)
        return false;

    /* Readers may be using an incremental copy of this record. */
    if (last->flags & (LOG_FLAG_TRUNCATED | LOG_FLAG_SEALED))
        return false;

    if (last->text_cursor + last->text_len != new_text_start)
        return false;

    if (new_text_len > PRINTK_BUFFER_SIZE ||
        (uint64_t)last->text_len + new_text_len > PRINTK_BUFFER_SIZE)
        return false;

    if (new_text_start == 0)
        return false;

    last_char_pos = (new_text_start - 1) % PRINTK_BUFFER_SIZE;
    if (printk_text_buffer[last_char_pos] == '\n')
        return false;

    return true;
}

static size_t printk_copy_text_slice(
    uint64_t start_cursor,
    size_t length,
    char *dst,
    size_t dst_size
){
    size_t first_len;
    uint64_t offset;

    if (!dst || dst_size == 0 || length == 0)
        return 0;

    if (length > dst_size)
        length = dst_size;

    offset = start_cursor % PRINTK_BUFFER_SIZE;

    if (offset + length <= PRINTK_BUFFER_SIZE) {
        memcpy(dst, &printk_text_buffer[offset], length);
    } else {
        first_len = PRINTK_BUFFER_SIZE - (size_t)offset;
        memcpy(dst, &printk_text_buffer[offset], first_len);
        memcpy(dst + first_len, &printk_text_buffer[0], length - first_len);
    }

    return length;
}

void printk_reader_init(struct printk_reader *reader, uint64_t start_sequence){
    if (!reader)
        return;

    reader->next_sequence = start_sequence;
    reader->partial_offset = 0;
    reader->dropped = 0;
}

/*
 * end_sequence is exclusive. UINT64_MAX means an unbounded reader.
 * The record is sealed on the first read: a subsequent pr_cont will start
 * a new record. The text continues to appear contiguously on the console,
 * but the reader never observes a text_len that changes in the middle
 * of an incremental read.
*/
static size_t printk_reader_next_until(
    struct printk_reader *reader,
    struct printk_record *out_record,
    char *out_buffer,
    size_t out_buffer_size,
    bool *is_continuation,
    uint64_t end_sequence
){
    unsigned long irq_flags;
    struct printk_record *target = NULL;
    uint32_t oldest_index;
    size_t copied = 0;

    if (is_continuation)
        *is_continuation = false;

    if (!reader || !out_buffer || out_buffer_size == 0)
        return 0;

    spin_lock_irqsave(&printk_lock, &irq_flags);

    if (printk_record_count == 0)
        goto out_unlock;

    oldest_index = printk_record_index(
        printk_record_head + PRINTK_RECORD_COUNT - printk_record_count
    );

    for (uint32_t i = 0; i < printk_record_count; ++i) {
        struct printk_record *record = &printk_records[
            printk_record_index(oldest_index + i)
        ];

        if (record->sequence >= end_sequence)
            break;

        if (record->sequence > reader->next_sequence) {
            /* The reader fell behind: the intermediate records are gone. */
            reader->dropped += record->sequence - reader->next_sequence;
            reader->next_sequence = record->sequence;
            reader->partial_offset = 0;
            target = record;
            break;
        }

        if (record->sequence == reader->next_sequence) {
            if (reader->partial_offset < record->text_len)
                target = record;
            break;
        }
    }

    if (!target)
        goto out_unlock;

	/*
	 * From here on, the text/metadata of this record can no longer grow.
	 * This allows copying in blocks smaller than the record, without holding the lock
	 * during the callback or requiring a 16 KiB buffer on the stack.
	*/
    target->flags |= LOG_FLAG_SEALED;

    if (out_record)
        *out_record = *target;

    if (is_continuation) {
        *is_continuation = (reader->partial_offset != 0) ||
                           ((target->flags & LOG_FLAG_CONT) != 0);
    }

    copied = printk_copy_text_slice(
        target->text_cursor + reader->partial_offset,
        target->text_len - reader->partial_offset,
        out_buffer,
        out_buffer_size
    );

    reader->partial_offset += (uint32_t)copied;

    if (reader->partial_offset >= target->text_len) {
        reader->next_sequence = target->sequence + 1;
        reader->partial_offset = 0;
    }

out_unlock:
    spin_unlock_irqrestore(&printk_lock, &irq_flags);
    return copied;
}

size_t printk_reader_next(
    struct printk_reader *reader,
    struct printk_record *out_record,
    char *out_buffer,
    size_t out_buffer_size,
    bool *is_continuation
){
    return printk_reader_next_until(
        reader,
        out_record,
        out_buffer,
        out_buffer_size,
        is_continuation,
        UINT64_MAX
    );
}

static bool printk_console_trylock(unsigned long *irq_flags){
    if (!spin_trylock_irqsave(&printk_console_lock, irq_flags))
        return false;

    local_irq_restore(irq_flags);
    return true;
}

static void printk_console_lock_acquire(unsigned long *irq_flags){
    spin_lock_irqsave(&printk_console_lock, irq_flags);
    local_irq_restore(irq_flags);
}

static void printk_console_unlock(unsigned long irq_flags){
    spin_unlock_irqrestore(&printk_console_lock, &irq_flags);
}

void printk_console_flush(void){
    unsigned long irq_flags;
    char local_buffer[PRINTK_LINE_MAX];
    struct printk_record record;

    if (!printk_console_trylock(&irq_flags))
        return;

    for (;;) {
        printk_echo_function echo;
        unsigned long lock_flags;
        size_t length;

        spin_lock_irqsave(&printk_lock, &lock_flags);
        echo = printk_echo;
        spin_unlock_irqrestore(&printk_lock, &lock_flags);

        if (!echo)
            break;

        length = printk_reader_next(
            &console_reader,
            &record,
            local_buffer,
            sizeof(local_buffer),
            NULL
        );

        if (length == 0)
            break;

        echo(local_buffer, (int)length);
    }

    printk_console_unlock(irq_flags);
}

/*
 * Replacing/removing the callback waits for the current flush to complete. Do not
 * call this function from within the callback itself, nor from a context that
 * cannot sleep while waiting for another owner of the console_lock.
 */
void printk_set_echo(printk_echo_function f){
    unsigned long console_flags;
    unsigned long log_flags;

    printk_console_lock_acquire(&console_flags);

    spin_lock_irqsave(&printk_lock, &log_flags);
    printk_echo = f;
    spin_unlock_irqrestore(&printk_lock, &log_flags);

    printk_console_unlock(console_flags);

    if (f)
        printk_console_flush();
}

int printk(const char *restrict fmt, ...){
    va_list args;
    unsigned long irq_flags;
    uint8_t level = LOG_LVL_INFO;
    uint8_t flags = 0;
    int prefix_length;
    int ret;
    uint64_t text_start;
    uint64_t text_end;
    uint64_t text_length;
    uint64_t timestamp_ms;

    if (!fmt)
        return -1;

    prefix_length = printk_parse_prefix(fmt, &level, &flags);
    if (prefix_length != 0)
        fmt += prefix_length;

    /* Avoid nesting the time lock inside the logger lock. */
    timestamp_ms = clock_get_monotonic_ns();
    do_div(timestamp_ms, 1000000);

    spin_lock_irqsave(&printk_lock, &irq_flags);

	// The CONT level is global, like in the classic printk model. Concurrent calls
	// can interleave fragments; see the note on pr_cont in the integration documentation.
    if (flags & LOG_FLAG_CONT)
        level = printk_cont_level;
    else
        printk_cont_level = level;

    text_start = printk_text_cursor;

    va_start(args, fmt);
    ret = vprintfmt(printk_write_char, NULL, fmt, args);
    va_end(args);

    text_end = printk_text_cursor;
    text_length = text_end - text_start;

    if (text_length == 0) {
        spin_unlock_irqrestore(&printk_lock, &irq_flags);
        return ret;
    }

    if (text_length > PRINTK_BUFFER_SIZE) {
        text_start = text_end - PRINTK_BUFFER_SIZE;
        text_length = PRINTK_BUFFER_SIZE;
        flags |= LOG_FLAG_TRUNCATED;
    }

    printk_expire_text_records(text_end);

    struct printk_record *last = printk_last_record();

    if ((flags & LOG_FLAG_CONT) &&
        printk_can_merge_cont(last, text_start, text_length)) {
        last->text_len += (uint32_t)text_length;
        last->flags |= LOG_FLAG_CONT;
    } else {
        struct printk_record *record = printk_alloc_record();

        record->sequence = printk_sequence++;
        record->timestamp_ms = timestamp_ms;
        record->text_cursor = text_start;
        record->text_len = (uint32_t)text_length;
        record->level = level;
        record->flags = flags;
    }

    spin_unlock_irqrestore(&printk_lock, &irq_flags);

	// flush always outside the lock
    printk_console_flush();

    return ret;
}

void printk_show_buffer(void){
    struct printk_reader dump_reader;
    struct printk_record record;
    char local_buffer[PRINTK_LINE_MAX];
    printk_echo_function echo;
    unsigned long console_flags;
    unsigned long log_flags;
    uint64_t end_sequence;

    printk_console_lock_acquire(&console_flags);

    spin_lock_irqsave(&printk_lock, &log_flags);
    echo = printk_echo;
    end_sequence = printk_sequence;
    spin_unlock_irqrestore(&printk_lock, &log_flags);

    if (!echo) {
        printk_console_unlock(console_flags);
        return;
    }

    printk_reader_init(&dump_reader, 0);

    for (;;) {
        size_t length = printk_reader_next_until(
            &dump_reader,
            &record,
            local_buffer,
            sizeof(local_buffer),
            NULL,
            end_sequence
        );

        if (length == 0)
            break;

        echo(local_buffer, (int)length);
    }

    printk_console_unlock(console_flags);
}
