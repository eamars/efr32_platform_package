/**
 * @file stack32.c
 * @brief First in late out data structure for holding 32bit integer
 * @author Ran Bao
 * @date Aug, 2017
 */

#include <stdint.h>
#include <stdlib.h>

#include "stack32.h"
#include "stack_private.h"
#include "drv_debug.h"

/**
 * @brief Initialize stack structure
 * @param stack pointer to the stack
 * @param size the maximum number of item stored in stack
 */
void stack32_init(stack_t *stack, size_t size)
{
	DRV_ASSERT(stack);

	// dynamically allocate memory for the stack
	stack->array = malloc(sizeof(uint32_t) * size);

	DRV_ASSERT(stack->array);

	// initialize stack pointer
	stack->stack_pointer = 0;

	stack->size = size;
}

/**
 * @brief Destory stack structure
 * @param stack pointer to the stack
 */
void stack32_del(stack_t *stack)
{
	DRV_ASSERT(stack);
	DRV_ASSERT(stack->array);

	free(stack->array);
}

/**
 * @brief Push one data to the top of stack
 * @param stack pointer to the stack
 * @param data data that would like to push
 */
void stack32_push(stack_t *stack, uint32_t data)
{
	DRV_ASSERT(stack);

	uint32_t * ptr = (uint32_t *) stack->array;

	DRV_ASSERT(stack->stack_pointer < stack->size);

	ptr[stack->stack_pointer++] = data;
}

/**
 * @brief Return the top element from stack structure and delete it
 * @param stack pointer to the struct
 * @return data
 */
uint32_t stack32_pop(stack_t *stack)
{
	DRV_ASSERT(stack);

	uint32_t * ptr = (uint32_t *) stack->array;

	DRV_ASSERT((stack->stack_pointer - 1) >= 0);

	return ptr[--stack->stack_pointer];
}

/**
 * @brief Return the top element from stack structure
 * @param stack pointer to the struct
 * @return data
 */
uint32_t stack32_top(stack_t *stack)
{
	DRV_ASSERT(stack);

	uint32_t * ptr = (uint32_t *) stack->array;

	DRV_ASSERT((stack->stack_pointer - 1) >= 0);

	return ptr[stack->stack_pointer - 1];
}
