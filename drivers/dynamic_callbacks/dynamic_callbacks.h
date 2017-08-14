//
// Created by Ran Bao on 14/08/17.
//

#ifndef ZSTACK_DYNAMIC_CALLBACKS_H
#define ZSTACK_DYNAMIC_CALLBACKS_H

#include <stdbool.h>

#define DYNAMIC_CALLBACK_TABLE_ENTRIES (128UL)
#define MAX_KEY_LEN 32

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

void dynamic_callback_table_init(void);
bool dynamic_callback_table_set(char * key, void * callback);
bool dynamic_callback_table_get(char * key, void ** callback);
bool dynamic_callback_table_del(char * key);


#ifdef __cplusplus
}
#endif

#endif //ZSTACK_DYNAMIC_CALLBACKS_H
