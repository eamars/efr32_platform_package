/**
 * @file io_device.h
 * @brief Abstract interface for serial devices.
 * @author Ran Bao (ran.bao@wirelessguard.co.nz)
 * @date March, 2017
 */
#ifndef IO_DEVICE_H_
#define IO_DEVICE_H_

#include <unistd.h>
#include <stdbool.h>

// basic io (required)
typedef int (*io_put) (void * device, int ch);
typedef int (*io_get) (void * device);
typedef bool (*io_read_ready) (void * device);
typedef void (*io_flush) (void * device);

// initialization (optional)
typedef void (*io_init) (void * device);

// streaming io (optional)
typedef ssize_t (*io_read) (void * device, void * buffer, size_t size);
typedef ssize_t (*io_write) (void * device, void * buffer, size_t size);

// flow control (optional)
typedef int (*io_rx_window) (void * device);
typedef int (*io_tx_window) (void * device);

/**
 * Abstract IO device struct
 */
typedef struct
{
	void * device;				///< generic device handler, include low level peripherals
	io_init init;

	io_put put;					///< corresponding function calls to write one character to such device
	io_get get;					///< function calls to read one character from such device (block or non-block)

	io_read read;				///< function that read sequence of bytes from serial port
	io_write write;				///< function that write sequence of bytes to serial port

	io_read_ready read_ready;	///< wait until the device is ready to read (used to implement block read)
	io_flush flush;				///< flush byte buffer immediately

	io_rx_window rx_window;
	io_tx_window tx_window;
} io_device;

#ifdef __cplusplus
extern "C" {
#endif

void io_device_init(io_device * handler);

void io_default_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* IO_DEVICE_H_ */
