
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

    MYFLT val = (MYFLT) strtod_l(s, NULL, csound->c_locale);
    return myflt_pool_find_or_add(csound, pool, val);
}
