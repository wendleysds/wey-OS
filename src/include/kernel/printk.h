#ifndef _KERNEL_PRINTK_H
#define _KERNEL_PRINTK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define KLOG_ESCAPE          "\001"
#define KLOG_ESCAPE_ASCII    '\001'

#define KLOG_EMERG   KLOG_ESCAPE "0"
#define KLOG_ALERT   KLOG_ESCAPE "1"
#define KLOG_CRIT    KLOG_ESCAPE "2"
#define KLOG_ERR     KLOG_ESCAPE "3"
#define KLOG_WARNING KLOG_ESCAPE "4"
#define KLOG_NOTICE  KLOG_ESCAPE "5"
#define KLOG_INFO    KLOG_ESCAPE "6"
#define KLOG_DEBUG   KLOG_ESCAPE "7"
#define KLOG_CONT    KLOG_ESCAPE "c"

#define LOG_LVL_EMERG   0
#define LOG_LVL_ALERT   1
#define LOG_LVL_CRIT    2
#define LOG_LVL_ERR     3
#define LOG_LVL_WARNING 4
#define LOG_LVL_NOTICE  5
#define LOG_LVL_INFO    6
#define LOG_LVL_DEBUG   7

#define LOG_FLAG_CONT      (1u << 0)
#define LOG_FLAG_TRUNCATED (1u << 1)
#define LOG_FLAG_SEALED    (1u << 2)

typedef void (*printk_echo_function)(const char *s, int length);

struct printk_record {
    uint64_t sequence;
    uint64_t timestamp_ms;
    uint64_t text_cursor;
    uint32_t text_len;
    uint8_t level;
    uint8_t flags;
};

struct printk_reader {
    uint64_t next_sequence;
    uint32_t partial_offset;
    uint64_t dropped;
};

int printk(const char *restrict fmt, ...);
void printk_set_echo(printk_echo_function f);
void printk_console_flush(void);
void printk_show_buffer(void);

void printk_reader_init(struct printk_reader *reader, uint64_t start_sequence);
size_t printk_reader_next(
    struct printk_reader *reader,
    struct printk_record *out_record,
    char *out_buffer,
    size_t out_buffer_size,
    bool *is_continuation
);

#define pr_emerg(fmt, ...)   printk(KLOG_EMERG fmt, ##__VA_ARGS__)
#define pr_alert(fmt, ...)   printk(KLOG_ALERT fmt, ##__VA_ARGS__)
#define pr_crit(fmt, ...)    printk(KLOG_CRIT fmt, ##__VA_ARGS__)
#define pr_err(fmt, ...)     printk(KLOG_ERR fmt, ##__VA_ARGS__)
#define pr_warning(fmt, ...) printk(KLOG_WARNING fmt, ##__VA_ARGS__)
#define pr_notice(fmt, ...)  printk(KLOG_NOTICE fmt, ##__VA_ARGS__)
#define pr_info(fmt, ...)    printk(KLOG_INFO fmt, ##__VA_ARGS__)
#define pr_debug(fmt, ...)   printk(KLOG_DEBUG fmt, ##__VA_ARGS__)
#define pr_cont(fmt, ...)    printk(KLOG_CONT fmt, ##__VA_ARGS__)

#endif /* _KERNEL_PRINTK_H */
