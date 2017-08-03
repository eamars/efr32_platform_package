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
	size_t stack_pointer;
	size_t size;
} stack_t;


#endif
