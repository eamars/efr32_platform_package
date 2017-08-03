/**
 * @brief Redirect stdio to specific device
 * @file redirect.h
 * @author Ran Bao
 * @date Aug, 2017
 */

#ifndef REDIRECT_H_
#define REDIRECT_H_

#include "io_device.h"
#include "sys.h"

#ifdef __cplusplus
extern "C" {
#endif

// stdio operations
static inline void stdio_redirect_to_device(io_device *instance)
{
	sys_redirect_stdin ((void *) instance->read, instance->device);
	sys_redirect_stdout ((void *) instance->write, instance->device);
	sys_redirect_stderr ((void *) instance->write, instance->device);
}

static inline void stdout_redirect_to_device(io_device *instance)
{
	sys_redirect_stdout ((void *) instance->write, instance->device);
}

static inline void stdin_redirect_to_device(io_device *instance)
{
	sys_redirect_stdin ((void *) instance->read, instance->device);
}

static inline void stderr_redirect_to_device(io_device *instance)
{
	sys_redirect_stderr ((void *) instance->write, instance->device);
}

#ifdef __cplusplus
}
#endif

#endif
