
#include "csoundCore.h"
#include "pools.h"

/* MYFLT POOL */

MYFLT_POOL* myflt_pool_create(CSOUND* csound) {
    MYFLT_POOL* pool = csound->Malloc(csound, sizeof(MYFLT_POOL));
    pool->count = 0;
    pool->max = POOL_SIZE;
    pool->values = csound->Calloc(csound, sizeof(MYFLT) * POOL_SIZE);
    
    return pool;
}

int myflt_pool_indexof(MYFLT_POOL* pool, MYFLT value) {
    int retVal = -1;
    int i;
    
    for (i = 0; i < pool->count; i++) {
        if(pool->values[i] == value) {
            retVal = i;
            break;
        }
    }
    
    return retVal;
}

int myflt_pool_find_or_add(CSOUND* csound, MYFLT_POOL* pool, MYFLT value) {
    int index = myflt_pool_indexof(pool, value);
    
    if(index == -1) {
        if (pool->count > 0 && pool->count % POOL_SIZE == 0) {
            pool->max += POOL_SIZE;
            pool->values = csound->ReAlloc(csound, pool->values, 
                                           pool->max * sizeof
                                           (MYFLT));    
        }
        index = pool->count;
        pool->values[index] = value;

        pool->count++;
    }
    
    return index;
}

int myflt_pool_find_or_addc(CSOUND* csound, MYFLT_POOL* pool, char* s) {
    
    MYFLT val = (MYFLT) strtod(s, NULL);
    return myflt_pool_find_or_add(csound, pool, val);
}


/* STRING POOL */

STRING_POOL* string_pool_create(CSOUND* csound) {
    STRING_POOL* pool = csound->Malloc(csound, sizeof(STRING_POOL));
    pool->values = NULL;
    pool->end = NULL;
    pool->count = 0;
    
    return pool;
}

STRING_VAL* string_pool_find(STRING_POOL* pool, char* value) {
    STRING_VAL* retVal = NULL;
    
    STRING_VAL* current = pool->values;

    while(current != NULL) {
        if(strcmp(value, current->value) == 0) {
            retVal = current;
            break;
        }
        current = current->next;
    }
    
    return retVal;

}
//
//STRING_VAL* string_pool_item_at(STRING_POOL* pool, int index) {
//    
//    STRING_VAL* current = pool->values;
//    
//    int i;
//    for(i = 0; i < index && current != NULL; i++) {
//        current = current->next;
//    }
//    
//    if(i == index) {
//        return current;
//    }
//    
//    return NULL;
//}

int string_pool_append_string(STRING_POOL* pool, STRING_VAL* val) {
    int index; 
    
    if(pool->values == NULL) {
        pool->values = val;
        pool->end = val;
        pool->count = 1;
        index = 0;
    } else {
        pool->end->next = val;
        pool->end = val;
        index = pool->count;
        pool->count += 1;
    }
    val->next = NULL;
    return index;
}

char* string_pool_save_string(CSOUND* csound, STRING_POOL* pool, char* value) {
    
    char* retVal = NULL;
    STRING_VAL* current = pool->values;
    
    while(current != NULL) {
        if(strcmp(value, current->value) == 0) {
            retVal = current->value;
            break;
        }
        current = current->next;
    }

    if(retVal == NULL) {
        STRING_VAL* val = csound->Malloc(csound, sizeof(STRING_VAL));
        val->value = (char*) csound->Malloc(csound, strlen(value) + 1);
        val->next = NULL;
        strcpy(val->value, value);
        //val->size = strlen(value);
        
        string_pool_append_string(pool, val);
        retVal = val->value;
    }
    
    return retVal;
}

STRING_VAL* string_pool_find_or_add(CSOUND* csound, STRING_POOL* pool, char* value) {
    
    STRING_VAL* retVal = string_pool_find(pool, value);
    
    if(retVal == NULL) {
        STRING_VAL* val = csound->Malloc(csound, sizeof(STRING_VAL));
        val->value = (char*) csound->Malloc(csound, strlen(value) + 1);
        strcpy(val->value, value);

        string_pool_append_string(pool, val);
        retVal = val;
    }
    
    return retVal;
}
