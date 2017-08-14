//
// Created by Ran Bao on 14/08/17.
//

#include <string.h>
#include <stdint.h>
#include "dynamic_callbacks.h"


/**
 * @brief The dynamic callback table is declared somewhere by host
 */
extern dynamic_callback_entry_t callback_table_internal[DYNAMIC_CALLBACK_TABLE_ENTRIES];


void dynamic_callback_table_init(void)
{
	memset(callback_table_internal, 0x0, sizeof(callback_table_internal));
}


bool dynamic_callback_table_set(char * key, void * callback)
{
	uint32_t next_empty = DYNAMIC_CALLBACK_TABLE_ENTRIES;

	// look for the function with same key first
	for (uint32_t i = 0; i < DYNAMIC_CALLBACK_TABLE_ENTRIES; i++)
	{
		dynamic_callback_entry_t * node = &callback_table_internal[i];

		// compare the key
		if (!strncmp(key, node->key, MAX_KEY_LEN))
		{
			// found, then update function
			node->callback = callback;
			return true;
		}


		// otherwise find the next empty slot
		if (next_empty == DYNAMIC_CALLBACK_TABLE_ENTRIES &&     // cond1: empty slot is not found
				node->key == 0)                                 // cond2: empty key
		{
			// found next empty slot
			next_empty = i;
		}
	}

	// insert into newly found empty slot
	if (next_empty != DYNAMIC_CALLBACK_TABLE_ENTRIES)
	{
		dynamic_callback_entry_t * node = &callback_table_internal[next_empty];

		strncpy(node->key, key, MAX_KEY_LEN);
		node->callback = callback;
		return true;
	}

	// no available slot found
	return false;
}


bool dynamic_callback_table_get(char * key, void ** callback)
{
	for (uint32_t i = 0; i < DYNAMIC_CALLBACK_TABLE_ENTRIES; i++)
	{
		dynamic_callback_entry_t * node = &callback_table_internal[i];

		// compare the key
		if (!strncmp(key, node->key, MAX_KEY_LEN))
		{
			// found
			*callback = node->callback;
			return true;
		}
	}

	// not found
	return false;
}

bool dynamic_callback_table_del(char * key)
{
	for (uint32_t i = 0; i < DYNAMIC_CALLBACK_TABLE_ENTRIES; i++)
	{
		dynamic_callback_entry_t * node = &callback_table_internal[i];

		// compare the key
		if (!strncmp(key, node->key, MAX_KEY_LEN))
		{
			// found
			node->key[0] = 0;
			return true;
		}
	}

	// not found
	return false;
}
