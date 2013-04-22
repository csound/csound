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

#include "csound_data_structures.h"


/* FUNCTIONS FOR CONS_CELL */

PUBLIC CONS_CELL* cs_cons(CSOUND* csound, void* val, CONS_CELL* cons) {
    CONS_CELL* cell = mmalloc(csound, sizeof(CONS_CELL));
    
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



/* FUNCTION FOR HASH SET */

const char* CS_HASH_SET = "hash_set_holder";

const unsigned char cs_strhash_tabl_8[256] = {
    230,  69,  87,  14,  21, 133, 233, 229,  54, 111, 196,  53, 128,  23,
    66, 225,  67,  79, 173, 110, 116,  56,  48, 129,  89, 188,  29, 251,
    186, 159, 102, 162, 227,  57, 220, 244, 165, 243, 215, 216, 214,  33,
    172, 254, 247, 241, 121, 197,  83,  22, 142,  61, 199,  50, 140, 192,
    6, 237, 183,  46, 206,  81,  18, 105, 147, 253,  15,  97, 179, 163,
    108, 123,  59, 198,  19, 141,   9,  95,  25, 219, 222,   1,   5,  52,
    90, 138,  11, 234,  55,  60, 209,  39,  80, 203, 120,   4,  64, 146,
    153, 157, 194, 134, 174, 100, 107, 125, 236, 160, 150,  41,  12, 223,
    135, 189, 122, 171,  10, 221,  71,  68, 106,  73, 218, 115,   2, 152,
    132, 190, 185, 113, 139, 104, 151, 154, 248, 117, 193, 118, 136, 204,
    17, 239, 158,  77, 103, 182, 250, 191, 170,  13,  75,  85,  62,   0,
    164,   8, 178,  93,  47,  42, 177,   3, 212, 255,  35, 137,  31, 224,
    242,  88, 161, 145,  49, 119, 143, 245, 201,  38, 211,  96, 169,  98,
    78, 195,  58, 109,  40, 238, 114,  20,  99,  24, 175, 200, 148, 112,
    45,   7,  28, 168,  27, 249,  94, 205, 156,  44,  37,  82, 217,  36,
    30,  16, 101,  72,  43, 149, 144, 187,  65, 131, 184, 166,  51,  32,
    226, 202, 231, 213, 126, 210, 235,  74, 208, 252, 181, 155, 246,  92,
    63, 228, 180, 176,  76, 167, 232,  91, 130,  84, 124,  86,  34,  26,
    207, 240, 127,  70
};



PUBLIC CS_HASH_TABLE* cs_create_hash_table(CSOUND* csound) {
    return (CS_HASH_TABLE*) mcalloc(csound, sizeof(CS_HASH_TABLE));
}

static inline unsigned char cs_name_hash(CSOUND *csound, const char *s)
{
    const unsigned char *c = (const unsigned char*) &(s[0]);
    unsigned int  h = 0U;
#ifdef LINUX
    for ( ; *c != (unsigned char) 0; c++)
        h = csound->strhash_tabl_8[h ^ *c];
#else
    (void) csound;
    for ( ; *c != (unsigned char) 0; c++)
        h = cs_strhash_tabl_8[h ^ *c];
#endif
    return (unsigned char) h;
}

PUBLIC void* cs_hash_table_get(CSOUND* csound, CS_HASH_TABLE* set, char* key) {
    unsigned char index;
    CS_HASH_TABLE_ITEM* item;
    
    if (key == NULL || strcmp(key, "") == 0){
        return NULL;
    }
    
    index = cs_name_hash(csound, key);
    item = set->buckets[index];
    
    if(item == NULL) return NULL;
    
    while (item != NULL) {
        if (strcmp(key, item->key) == 0) {
            return item->value;
        }
        item = item->next;
    }
    
    return NULL;
}

PUBLIC void cs_hash_table_put(CSOUND* csound, CS_HASH_TABLE* set, char* key, void* value) {
    if (key == NULL || strcmp(key, "") == 0){
        return;
    }
    
    unsigned char index = cs_name_hash(csound, key);
    
    CS_HASH_TABLE_ITEM* item = set->buckets[index];
    
    if (item == NULL) {
        CS_HASH_TABLE_ITEM* newItem = mmalloc(csound, sizeof(CS_HASH_TABLE_ITEM));
        newItem->key = cs_strdup(csound, key);
        newItem->value = value;
        set->buckets[index] = newItem;
    } else {
        while (item != NULL) {
            if (strcmp(key, item->key) == 0) {
                item->value = value;
                return;
            } else if(item->next == NULL) {
                CS_HASH_TABLE_ITEM* newItem = mmalloc(csound, sizeof(CS_HASH_TABLE_ITEM));
                newItem->key = cs_strdup(csound, key);
                newItem->value = value;
                item->next = newItem;
                return;
            }
            item = item->next;
        }
    }
    
    

}

PUBLIC void cs_hash_table_remove(CSOUND* csound, CS_HASH_TABLE* set, char* key) {
    CS_HASH_TABLE_ITEM *previous, *item;
    unsigned char index;
    
    if (key == NULL || strcmp(key, "") == 0){
        return;
    }
    
    index = cs_name_hash(csound, key);
    
    previous = NULL;
    item = set->buckets[index];
    
    while (item != NULL) {
        if (strcmp(key, item->key) == 0) {
            if (previous == NULL) {
                set->buckets[index] = NULL;
                return;
            } else {
                previous->next = item->next;
                mfree(csound, item);
            }
        }
        previous = item;
        item = item->next;
    }
}

PUBLIC void cs_hash_table_delete(CSOUND* csound, CS_HASH_TABLE* set) {
    int i;
    
    for (i = 0; i < 256; i++) {
        CS_HASH_TABLE_ITEM* item = set->buckets[i];
        
        while(item != NULL) {
            CS_HASH_TABLE_ITEM* next = item->next;
            mfree(csound, item);  // should this also free the value of the item
            item = next;
        }
    }
    mfree(csound, set);
}