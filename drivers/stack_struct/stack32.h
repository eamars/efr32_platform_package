/**
 * @file stack32.h
 * @brief First in late out data structure for holding 32bit integer
 * @author Ran Bao
 * @date Aug, 2017
 */

#ifndef STACK32_H_
#define STACK32_H_

#include <stack_private.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize stack structure
 * @param stack pointer to the stack
 * @param size the maximum number of item stored in stack
 */
void stack32_init(stack_t *stack, size_t size);

/**
 * @brief Destory stack structure
 * @param stack pointer to the stack
 */
void stack32_del(stack_t *stack);

/**
 * @brief Push one data to the top of stack
 * @param stack pointer to the stack
 * @param data data that would like to push
 */
void stack32_push(stack_t *stack, uint32_t data);

/**
 * @brief Return the top element from stack structure and delete it
 * @param stack pointer to the struct
 * @return data
 */
uint32_t stack32_pop(stack_t *stack);

/**
 * @brief Return the top element from stack structure
 * @param stack pointer to the struct
 * @return data
 */
uint32_t stack32_top(stack_t *stack);

#ifdef __cplusplus
}
#endif

#endif
