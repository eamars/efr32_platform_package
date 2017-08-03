/**
 * @file io_device.c
 * @brief Abstract interface for serial devices.
 * @author Ran Bao (ran.bao@wirelessguard.co.nz)
 * @date March, 2017
 */

#include "io_device.h"

void io_default_handler(void)
{
	// do nothing
}

// safely initialize io_device
void io_device_init(io_device * handler)
{
	handler->device = NULL;
	handler->init = (void *) io_default_handler;

	handler->put = (void *) io_default_handler;
	handler->get = (void *) io_default_handler;

	handler->read = (void *) io_default_handler;
	handler->write = (void *) io_default_handler;

	handler->read_ready = (void *) io_default_handler;
	handler->flush = (void *) io_default_handler;

	handler->rx_window = (void *) io_default_handler;
	handler->tx_window = (void *) io_default_handler;
}
