/*
 csound_data_structures.c:

 Copyright (C) 2013 Steven Yi

 This file is part of Csound.

 The Csound Library is free software; you can redistribute it
 and/or modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 Csound is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with Csound; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA
 */
#include "csoundCore.h"
#include "csound_data_structures.h"

#ifdef __cplusplus
extern "C" {
#endif

/* FUNCTIONS FOR CONS_CELL */

PUBLIC CONS_CELL* cs_cons(CSOUND* csound, void* val, CONS_CELL* cons) {
    CONS_CELL* cell = csound->Malloc(csound, sizeof(CONS_CELL));

    cell->value = val;
    cell->next = cons;

    return cell;
}

PUBLIC CONS_CELL* cs_cons_append(CONS_CELL* cons1, CONS_CELL* cons2) {
    if(cons1 == NULL) return cons2;
    if(cons2 == NULL) return cons1;

    CONS_CELL* c = cons1;

    while (c->next != NULL) c = c->next;

    c->next = cons2;

    return cons1;
}

PUBLIC int cs_cons_length(CONS_CELL* head) {
    CONS_CELL* current = head;
    int count = 0;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count;
}

PUBLIC void cs_cons_free(CSOUND* csound, CONS_CELL* head) {
    CONS_CELL *current, *next;

    if (head == NULL) return;

    current = head;

    while(current != NULL) {
        next = current->next;
        csound->Free(csound, current);
        current = next;
    }
}


PUBLIC void cs_cons_free_complete(CSOUND* csound, CONS_CELL* head) {

    CONS_CELL *current, *next;

    if (head == NULL) return;

    current = head;

    while(current != NULL) {
        next = current->next;
        csound->Free(csound, current->value);
        csound->Free(csound, current);
        current = next;
    }
}

/* FUNCTION FOR HASH SET */

PUBLIC CS_HASH_TABLE* cs_hash_table_create(CSOUND* csound) {
    return (CS_HASH_TABLE*) csound->Calloc(csound, sizeof(CS_HASH_TABLE));
}

static unsigned int cs_name_hash(char *s)
{
    unsigned int h = 0;
    while (*s != '\0') {
        h = (h<<4) ^ *s++;
    }
    return (h%HASH_SIZE);
}

PUBLIC void* cs_hash_table_get(CSOUND* csound,
                               CS_HASH_TABLE* hashTable, char* key) {
    unsigned int index;
    CS_HASH_TABLE_ITEM* item;

    if (key == NULL) {
        return NULL;
    }

    index = cs_name_hash(key);
    item = hashTable->buckets[index];

    while (item != NULL) {
        if (strcmp(key, item->key) == 0) {
            return item->value;
        }
        item = item->next;
    }

    return NULL;
}

PUBLIC char* cs_hash_table_get_key(CSOUND* csound,
                                   CS_HASH_TABLE* hashTable, char* key) {
    unsigned int index;
    CS_HASH_TABLE_ITEM* item;

    if (key == NULL) {
        return NULL;
    }

    index = cs_name_hash(key);
    item = hashTable->buckets[index];

    while (item != NULL) {
        if (strcmp(key, item->key) == 0) {
            return item->key;
        }
        item = item->next;
    }

    return NULL;
}

char* cs_hash_table_put_no_key_copy(CSOUND* csound,
                                   CS_HASH_TABLE* hashTable,
                                    char* key, void* value) {
    if (key == NULL) {
        return NULL;
    }

    unsigned int index = cs_name_hash(key);

    CS_HASH_TABLE_ITEM* item = hashTable->buckets[index];

    if (item == NULL) {
        CS_HASH_TABLE_ITEM* newItem = csound->Malloc(csound,
                                                     sizeof(CS_HASH_TABLE_ITEM));
        newItem->key = key;
        newItem->value = value;
        newItem->next = NULL;
        hashTable->buckets[index] = newItem;
        return newItem->key;
    } else {
        while (item != NULL) {
            if (strcmp(key, item->key) == 0) {
                item->value = value;
                return item->key;
            } else if(item->next == NULL) {
                CS_HASH_TABLE_ITEM* newItem = csound->Malloc(csound,
                                                      sizeof(CS_HASH_TABLE_ITEM));
                newItem->key = key;
                newItem->value = value;
                newItem->next = NULL;
                item->next = newItem;
                return newItem->key;
            }
            item = item->next;
        }
    }
    return NULL;
}

PUBLIC void cs_hash_table_put(CSOUND* csound,
                              CS_HASH_TABLE* hashTable, char* key, void* value) {
    cs_hash_table_put_no_key_copy(csound, hashTable,
                                  cs_strdup(csound, key), value);
}

PUBLIC char* cs_hash_table_put_key(CSOUND* csound,
                                   CS_HASH_TABLE* hashTable, char* key) {
    return cs_hash_table_put_no_key_copy(csound, hashTable,
                                         cs_strdup(csound, key), NULL);
}

PUBLIC void cs_hash_table_remove(CSOUND* csound,
                                 CS_HASH_TABLE* hashTable, char* key) {
    CS_HASH_TABLE_ITEM *previous, *item;
    unsigned int index;

    if (key == NULL) {
        return;
    }

    index = cs_name_hash(key);

    previous = NULL;
    item = hashTable->buckets[index];

    while (item != NULL) {
        if (strcmp(key, item->key) == 0) {
            if (previous == NULL) {
                hashTable->buckets[index] = NULL;
                csound->Free(csound, item);
                return;
            } else {
                previous->next = item->next;
                csound->Free(csound, item);
            }
        }
        previous = item;
        item = item->next;
    }
}

PUBLIC CONS_CELL* cs_hash_table_keys(CSOUND* csound, CS_HASH_TABLE* hashTable) {
    CONS_CELL* head = NULL;

    int i = 0;

    for (i = 0; i < HASH_SIZE; i++) {
        CS_HASH_TABLE_ITEM* item = hashTable->buckets[i];

        while (item != NULL) {
            head = cs_cons(csound, item->key, head);
            item = item->next;
        }
    }
    return head;
}

PUBLIC CONS_CELL* cs_hash_table_values(CSOUND* csound, CS_HASH_TABLE* hashTable) {
    CONS_CELL* head = NULL;

    int i = 0;

    for (i = 0; i < HASH_SIZE; i++) {
        CS_HASH_TABLE_ITEM* item = hashTable->buckets[i];

        while (item != NULL) {
            head = cs_cons(csound, item->value, head);
            item = item->next;
        }
    }
    return head;
}

PUBLIC void cs_hash_table_merge(CSOUND* csound,
                                CS_HASH_TABLE* target, CS_HASH_TABLE* source) {
    // TODO - check if this is the best strategy for merging
    int i = 0;

    for (i = 0; i < HASH_SIZE; i++) {
        CS_HASH_TABLE_ITEM* item = source->buckets[i];

        while (item != NULL) {
            cs_hash_table_put_no_key_copy(csound, target, item->key, item->value);
            item = item->next;
        }
    }

}

PUBLIC void cs_hash_table_free(CSOUND* csound, CS_HASH_TABLE* hashTable) {
    int i;

    for (i = 0; i < HASH_SIZE; i++) {
        CS_HASH_TABLE_ITEM* item = hashTable->buckets[i];

        while(item != NULL) {
            CS_HASH_TABLE_ITEM* next = item->next;
            csound->Free(csound, item->key);
            csound->Free(csound, item);
            item = next;
        }
    }
    csound->Free(csound, hashTable);
}

PUBLIC void cs_hash_table_mfree_complete(CSOUND* csound, CS_HASH_TABLE* hashTable) {

    int i;

    for (i = 0; i < HASH_SIZE; i++) {
        CS_HASH_TABLE_ITEM* item = hashTable->buckets[i];

        while(item != NULL) {
            CS_HASH_TABLE_ITEM* next = item->next;
            csound->Free(csound, item->key);
            csound->Free(csound, item->value);
            csound->Free(csound, item);
            item = next;
        }
    }
    csound->Free(csound, hashTable);
}

PUBLIC void cs_hash_table_free_complete(CSOUND* csound, CS_HASH_TABLE* hashTable) {

    int i;

    for (i = 0; i < HASH_SIZE; i++) {
        CS_HASH_TABLE_ITEM* item = hashTable->buckets[i];

        while(item != NULL) {
            CS_HASH_TABLE_ITEM* next = item->next;
            csound->Free(csound, item->key);
            
            /* NOTE: This needs to be free, not csound->Free.
               To use mfree on keys, use cs_hash_table_mfree_complete 
               TODO: Check if this is even necessary anymore... */
            free(item->value);
            csound->Free(csound, item);
            item = next;
        }
    }
    csound->Free(csound, hashTable);
}



#ifdef __cplusplus
extern "C" {
#endif
