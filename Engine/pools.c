
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
        if (pool->count % POOL_SIZE == 0) {
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


/* STRING POOL */

STRING_POOL* string_pool_create(CSOUND* csound) {
    STRING_POOL* pool = csound->Malloc(csound, sizeof(STRING_POOL));
    pool->values = NULL;
    pool->end = NULL;
    pool->count = 0;
    
    return pool;
}

int string_pool_indexof(STRING_POOL* pool, char* value) {
    int retVal = -1;
    
    STRING_VAL* current = pool->values;

    int index = 0;
    while(current != NULL) {
        if(strcmp(value, current->value) == 0) {
            retVal = index;
            break;
        }
        index++;
    }
    
    return retVal;

}

int string_pool_find_or_add(CSOUND* csound, STRING_POOL* pool, char* value) {
    
    int index = string_pool_indexof(pool, value);
    
    if(index == -1) {
        STRING_VAL* val = csound->Malloc(csound, sizeof(STRING_VAL));
        val->value = value; // should this copy?
        val->size = strlen(value);

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
    }
    
    return index;
}
