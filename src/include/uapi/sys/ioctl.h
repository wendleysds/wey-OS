#ifndef _UAPI_SYS_IOCTL_H
#define _UAPI_SYS_IOCTL_H

#define _IOC_NONE  0U
#define _IOC_WRITE 1U
#define _IOC_READ  2U

/**
 * @brief Create an ioctl command code.
 * @param dir Direction of the data transfer (none, write, or read).
 * @param type Type of the device (application-specific).
 * @param nr Command number (application-specific).
 * @param size Size of the data to be transferred.
 * @return The ioctl command code.
 *
 * 31              30 29            16 15       8 7        0
 * +----------------+----------------+----------+----------+
 * |      dir       |      size      |   type   |    nr    |
 * +----------------+----------------+----------+----------+
 */
#define _IOC(dir, type, nr, size) \
    (((dir)  << 30) | \
     ((size) << 16) | \
     ((type) << 8)  | \
     (nr))

#define _IO(type, nr) \
    _IOC(_IOC_NONE, (type), (nr), 0)

#define _IOW(type, nr, data_type) \
    _IOC(_IOC_WRITE, (type), (nr), sizeof(data_type))

#define _IOR(type, nr, data_type) \
    _IOC(_IOC_READ, (type), (nr), sizeof(data_type))

#define _IOWR(type, nr, data_type) \
    _IOC(_IOC_READ | _IOC_WRITE, (type), (nr), sizeof(data_type))

// Decoding
#define _IO_DIR(cmd)  (((cmd) >> 30) & 0x3U)
#define _IO_TYPE(cmd) (((cmd) >> 8) & 0xffU)
#define _IO_NR(cmd)   ((cmd) & 0xffU)
#define _IO_SIZE(cmd) (((cmd) >> 16) & 0x3fffU)

#endif /* _UAPI_SYS_IOCTL_H */