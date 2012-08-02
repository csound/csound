#ifndef POOLS_H
#define POOLS_H

#define POOL_SIZE 256

typedef struct myflt_pool {
    MYFLT* values;
    int max;
    int count;
} MYFLT_POOL;

typedef struct string_val {
    char* value;
    int size;
    struct string_val* next;
} STRING_VAL;

typedef struct string_pool {
    STRING_VAL* values;
    STRING_VAL* end;
    int count;
} STRING_POOL;

MYFLT_POOL* myflt_pool_create(CSOUND* csound);
int myflt_pool_indexof(MYFLT_POOL* pool, MYFLT value);
int myflt_pool_find_or_add(CSOUND* csound, MYFLT_POOL* pool, MYFLT value);
int myflt_pool_find_or_addc(CSOUND* csound, MYFLT_POOL* pool, char* s);

STRING_POOL* string_pool_create(CSOUND* csound);
STRING_VAL* string_pool_indexof(STRING_POOL* pool, char* value);
STRING_VAL* string_pool_find_or_add(CSOUND* csound, STRING_POOL* pool, char* value);
char* string_pool_save_string(CSOUND* csound, STRING_POOL* pool, char* value);

#endif

