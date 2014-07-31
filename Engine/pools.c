
#include "csoundCore.h"
#include "pools.h"
#include "csound_standard_types.h"

/* MYFLT POOL */

MYFLT_POOL* myflt_pool_create(CSOUND* csound) {
    MYFLT_POOL* pool = csound->Malloc(csound, sizeof(MYFLT_POOL));
    pool->count = 0;
    pool->max = POOL_SIZE;
    pool->values = csound->Calloc(csound, sizeof(CS_VAR_MEM) * POOL_SIZE);

    return pool;
}

void myflt_pool_free(CSOUND *csound, MYFLT_POOL *pool){
  if(pool != NULL) {
  csound->Free(csound, pool->values);
  csound->Free(csound, pool);
  }
}

int myflt_pool_indexof(MYFLT_POOL* pool, MYFLT value) {
    int retVal = -1;
    int i;

    for (i = 0; i < pool->count; i++) {
        if(pool->values[i].value == value) {
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
                                           (CS_VAR_MEM));
        }
        index = pool->count;
        pool->values[index].varType = (CS_TYPE*)&CS_VAR_TYPE_C;
        pool->values[index].value = value;

        pool->count++;
    }

    return index;
}

int myflt_pool_find_or_addc(CSOUND* csound, MYFLT_POOL* pool, char* s) {

    MYFLT val = (MYFLT) cs_strtod(s, NULL);
    return myflt_pool_find_or_add(csound, pool, val);
}
