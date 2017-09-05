/**
 * @brief Generic stack structure
 * @file stack_private.h
 * @author Ran Bao
 * @date Aug, 2017
 */

#ifndef STACK_PRIVATE_H_
#define STACK_PRIVATE_H_

#include <stddef.h>

typedef struct
{
	void * array;
	int32_t stack_pointer;
	int32_t size;
} stack_t;


#endif
