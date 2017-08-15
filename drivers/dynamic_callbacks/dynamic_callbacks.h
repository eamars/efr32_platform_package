//
// Created by Ran Bao on 14/08/17.
//

#ifndef ZSTACK_DYNAMIC_CALLBACKS_H
#define ZSTACK_DYNAMIC_CALLBACKS_H

#include <stdbool.h>

#define DYNAMIC_CALLBACK_TABLE_ENTRIES (128UL)
#define MAX_KEY_LEN 64

/**
 * @brief Structure for callback stacks
 */
typedef struct
{
	char key[MAX_KEY_LEN];
	void * callback;
} dynamic_callback_entry_t;


#ifdef __cplusplus
extern "C" {
#endif

// Using macro is evil
#define GEN_DYN_FUNC(fname, return_type, ...) \
    typedef return_type (*fname##_handler_t)(__VA_ARGS__); \
	static fname##_handler_t fname##_handler = (void *) 0;

#define CALL_DYN_FUNC_WITH_CACHE(fname, ...) \
    if (fname##_handler == 0) { \
        dynamic_callback_table_get(#fname, (void *) &fname##_handler); \
    }\
    if (fname##_handler) {\
        (*fname##_handler)(__VA_ARGS__);\
    }


void dynamic_callback_table_init(void);
bool dynamic_callback_table_set(char * key, void * callback);
bool dynamic_callback_table_get(char * key, void ** callback);
bool dynamic_callback_table_del(char * key);


#ifdef __cplusplus
}
#endif

#endif //ZSTACK_DYNAMIC_CALLBACKS_H
