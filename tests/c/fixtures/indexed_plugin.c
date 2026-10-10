/* Example plugin: a type that provides its own [] read and write.
   SPDX-License-Identifier: LGPL-2.1-or-later */
#include "csdl.h"
#include <string.h>

/* The variable memory holds the vector itself; Csound frees the payload
   through the type's freeVariableMemory hook. */
typedef struct {
  cs_float *data;
  size_t size;
  size_t capacity;
} INDEXED_VEC;

static int32_t vec_reserve(CSOUND *csound, INDEXED_VEC *v, size_t n)
{
  if (n > v->capacity) {
    cs_float *data = csound->ReAlloc(csound, v->data, n * sizeof(cs_float));
    if (data == NULL) return NOTOK;
    v->data = data;
    v->capacity = n;
  }
  v->size = n;
  return OK;
}

static void vec_init_memory(CSOUND *csound, CS_VARIABLE *var, cs_float *mem)
{
  (void)csound;
  memset(mem, 0, var->memBlockSize);
}

static CS_VARIABLE *vec_create(void *cs, const CS_TYPE *type, const void *typeArg, INSDS *ctx)
{
  CSOUND *csound = (CSOUND *)cs;
  CS_VARIABLE *var;
  (void)type; (void)typeArg;
  var = csound->Calloc(csound, sizeof(CS_VARIABLE));
  if (var == NULL) return NULL;
  var->memBlockSize = CS_FLOAT_ALIGN(sizeof(INDEXED_VEC));
  var->initializeVariableMemory = vec_init_memory;
  var->ctx = ctx;
  return var;
}

static void vec_copy_value(CSOUND *csound, const CS_TYPE *type, void *dest, const void *src, INSDS *ctx)
{
  INDEXED_VEC *d = (INDEXED_VEC *)dest;
  const INDEXED_VEC *s = (const INDEXED_VEC *)src;
  (void)type; (void)ctx;
  if (d == s || vec_reserve(csound, d, s->size) != OK) return;
  if (s->size) memcpy(d->data, s->data, s->size * sizeof(cs_float));
}

static void vec_free(void *cs, void *mem)
{
  CSOUND *csound = (CSOUND *)cs;
  INDEXED_VEC *v = (INDEXED_VEC *)mem;
  if (v->data != NULL) csound->Free(csound, v->data);
  memset(v, 0, sizeof(INDEXED_VEC));
}

static CS_TYPE INDEXED_VEC_TYPE = {
  "IndexedVec", "plugin vector with its own []", CS_ARG_TYPE_BOTH,
  vec_create, vec_copy_value, vec_free, NULL, 0
};

/* Same layout and callbacks, but no [] entries: indexing stays an error. */
static CS_TYPE OPAQUE_VEC_TYPE = {
  "OpaqueVec", "plugin vector without []", CS_ARG_TYPE_BOTH,
  vec_create, vec_copy_value, vec_free, NULL, 0
};

/* The IndexedVec entries registered in reverse order: the order must not
   change which entry an access uses. */
static CS_TYPE REVERSED_VEC_TYPE = {
  "ReversedVec", "IndexedVec with its [] entries reversed", CS_ARG_TYPE_BOTH,
  vec_create, vec_copy_value, vec_free, NULL, 0
};

/* Two [] reads and two [] writes with the same signature: ambiguous. */
static CS_TYPE AMBIGUOUS_VEC_TYPE = {
  "AmbiguousVec", "vector with ambiguous [] entries", CS_ARG_TYPE_BOTH,
  vec_create, vec_copy_value, vec_free, NULL, 0
};

typedef struct {
  OPDS h;
  INDEXED_VEC *out;
  cs_float *size;
} VEC_NEW;

typedef struct {
  OPDS h;
  cs_float *out;
  INDEXED_VEC *vec;
} VEC_LEN;

typedef struct {
  OPDS h;
  cs_float *out;
  INDEXED_VEC *vec;
  cs_float *index;
} VEC_GET;

typedef struct {
  OPDS h;
  INDEXED_VEC *out;
  INDEXED_VEC *vec;
  cs_float *from;
  cs_float *to;
} VEC_SLICE;

typedef struct {
  OPDS h;
  INDEXED_VEC *vec;
  cs_float *value;
  cs_float *index;
} VEC_SET;

/* Element i is 10 * i, so a read shows which index it reached. */
static int32_t vec_new(CSOUND *csound, VEC_NEW *p)
{
  size_t i, n = (size_t)*p->size;
  if (vec_reserve(csound, p->out, n) != OK) return NOTOK;
  for (i = 0; i < n; i++) p->out->data[i] = 10 * (cs_float)i;
  return OK;
}

static int32_t vec_len(CSOUND *csound, VEC_LEN *p)
{
  (void)csound;
  *p->out = (cs_float)p->vec->size;
  return OK;
}

/* The index policy is the plugin's: an index is truncated toward zero, so
   v[1.7] is v[1], and a negative, NaN or out-of-range one is an error. */
static int32_t vec_index(const INDEXED_VEC *v, cs_float x, size_t *i)
{
  if (!(x >= 0) || x >= (cs_float)v->size) return NOTOK;
  *i = (size_t)x;
  return OK;
}

static int32_t vec_get(CSOUND *csound, VEC_GET *p)
{
  size_t i;
  if (vec_index(p->vec, *p->index, &i) != OK)
    return csound->InitError(csound, "IndexedVec read out of range\n");
  *p->out = p->vec->data[i];
  return OK;
}

static int32_t vec_get_perf(CSOUND *csound, VEC_GET *p)
{
  size_t i;
  if (vec_index(p->vec, *p->index, &i) != OK)
    return csound->PerfError(csound, &p->h, "IndexedVec read out of range\n");
  *p->out = p->vec->data[i];
  return OK;
}

/* v[from][to] is the slice [from, to), copied: writing to the slice leaves
   v as it was. */
static int32_t vec_slice_bounds(const INDEXED_VEC *v, cs_float from, cs_float to,
                                size_t *f, size_t *t)
{
  if (!(from >= 0) || !(to >= from) || to > (cs_float)v->size) return NOTOK;
  *f = (size_t)from;
  *t = (size_t)to;
  return OK;
}

static int32_t vec_slice_copy(INDEXED_VEC *out, const INDEXED_VEC *vec, size_t from, size_t to)
{
  out->size = to - from;
  if (out->size)
    memcpy(out->data, vec->data + from, out->size * sizeof(cs_float));
  return OK;
}

static int32_t vec_slice(CSOUND *csound, VEC_SLICE *p)
{
  size_t from, to;
  if (vec_slice_bounds(p->vec, *p->from, *p->to, &from, &to) != OK)
    return csound->InitError(csound, "IndexedVec slice out of range\n");
  if (vec_reserve(csound, p->out, to - from) != OK) return NOTOK;
  return vec_slice_copy(p->out, p->vec, from, to);
}

static int32_t vec_slice_perf(CSOUND *csound, VEC_SLICE *p)
{
  size_t from, to;
  if (vec_slice_bounds(p->vec, *p->from, *p->to, &from, &to) != OK ||
      to - from > p->out->capacity)
    return csound->PerfError(csound, &p->h, "IndexedVec slice out of range\n");
  return vec_slice_copy(p->out, p->vec, from, to);
}

static int32_t vec_set(CSOUND *csound, VEC_SET *p)
{
  size_t i;
  if (vec_index(p->vec, *p->index, &i) != OK)
    return csound->InitError(csound, "IndexedVec write out of range\n");
  p->vec->data[i] = *p->value;
  return OK;
}

static int32_t vec_set_perf(CSOUND *csound, VEC_SET *p)
{
  size_t i;
  if (vec_index(p->vec, *p->index, &i) != OK)
    return csound->PerfError(csound, &p->h, "IndexedVec write out of range\n");
  p->vec->data[i] = *p->value;
  return OK;
}

typedef struct {
  OPDS h;
  INDEXED_VEC *out;
  INDEXED_VEC *a;
  INDEXED_VEC *b;
} VEC_ADD;

/* v + w through the "##add" entry: element-wise over the shorter vector. */
static int32_t vec_add(CSOUND *csound, VEC_ADD *p)
{
  size_t i, n = p->a->size < p->b->size ? p->a->size : p->b->size;
  if (vec_reserve(csound, p->out, n) != OK) return NOTOK;
  for (i = 0; i < n; i++) p->out->data[i] = p->a->data[i] + p->b->data[i];
  return OK;
}

/* A struct type registered with RegisterStruct: [i] reads and writes the
   i-th member, so p[0] is p.x and p[1] is p.y. */
typedef struct {
  OPDS h;
  cs_float *out;
  CS_STRUCT_VAR *pair;
  cs_float *index;
} PAIR_GET;

typedef struct {
  OPDS h;
  CS_STRUCT_VAR *pair;
  cs_float *value;
  cs_float *index;
} PAIR_SET;

static int32_t pair_get(CSOUND *csound, PAIR_GET *p)
{
  cs_float x = *p->index;
  int32_t i = x >= 0 ? (int32_t)x : -1;
  if (i < 0 || i >= p->pair->memberCount)
    return csound->InitError(csound, "IndexedPair read out of range\n");
  *p->out = p->pair->members[i]->value;
  return OK;
}

static int32_t pair_set(CSOUND *csound, PAIR_SET *p)
{
  cs_float x = *p->index;
  int32_t i = x >= 0 ? (int32_t)x : -1;
  if (i < 0 || i >= p->pair->memberCount)
    return csound->InitError(csound, "IndexedPair write out of range\n");
  p->pair->members[i]->value = *p->value;
  return OK;
}

/* p[i][j] is the pair (p[i], p[j]): a read that returns the struct type. */
typedef struct {
  OPDS h;
  CS_STRUCT_VAR *out;
  CS_STRUCT_VAR *pair;
  cs_float *first;
  cs_float *second;
} PAIR_PICK;

static int32_t pair_pick(CSOUND *csound, PAIR_PICK *p)
{
  cs_float *at[2] = {p->first, p->second};
  int32_t n;
  for (n = 0; n < 2; n++) {
    int32_t i = *at[n] >= 0 ? (int32_t)*at[n] : -1;
    if (i < 0 || i >= p->pair->memberCount)
      return csound->InitError(csound, "IndexedPair read out of range\n");
    p->out->members[n]->value = p->pair->members[i]->value;
  }
  return OK;
}

PUBLIC int32_t csoundModuleCreate(CSOUND *csound)
{
  (void)csound;
  return OK;
}

/* The command line initialises a plugin once while parsing options, before
   the type pool exists, and again when compiling: register on the pass that
   has a pool, once. */
static int32_t register_types(CSOUND *csound, TYPE_POOL *pool)
{
  const CSOUND_STRUCT_MEMBER pair[] = {{"x", "i"}, {"y", "i"}};
  if (csound->GetType(csound, "IndexedVec") != NULL) return OK;
  return csound->AddVariableType(csound, pool, &INDEXED_VEC_TYPE) &&
    csound->AddVariableType(csound, pool, &OPAQUE_VEC_TYPE) &&
    csound->AddVariableType(csound, pool, &REVERSED_VEC_TYPE) &&
    csound->AddVariableType(csound, pool, &AMBIGUOUS_VEC_TYPE) &&
    csound->RegisterStruct(csound, "IndexedPair", pair, 2) ? OK : NOTOK;
}

PUBLIC int32_t csoundModuleInit(CSOUND *csound)
{
  TYPE_POOL *pool = csound->GetTypePool(csound);
  if (pool == NULL) return OK;
  if (register_types(csound, pool) != OK) return NOTOK;

  static OENTRY opcodes[] = {
    {"indexed_vec",     sizeof(VEC_NEW),   0, ":IndexedVec;",   "i",              (SUBR) vec_new,   NULL,                  NULL},
    {"opaque_vec",      sizeof(VEC_NEW),   0, ":OpaqueVec;",    "i",              (SUBR) vec_new,   NULL,                  NULL},
    {"reversed_vec",    sizeof(VEC_NEW),   0, ":ReversedVec;",  "i",              (SUBR) vec_new,   NULL,                  NULL},
    {"ambiguous_vec",   sizeof(VEC_NEW),   0, ":AmbiguousVec;", "i",              (SUBR) vec_new,   NULL,                  NULL},
    {"indexed_vec_len", sizeof(VEC_LEN),   0, "i",              ":IndexedVec;",   (SUBR) vec_len,   NULL,                  NULL},

    /* One index: an i read runs at init time only, a k read at init and
       perf time. Csound picks the k read when the value is used at perf
       time, so it follows changes, and the i read for an init-only use. */
    {"##array_get.IVi", sizeof(VEC_GET),   0, "i",            ":IndexedVec;i",   (SUBR) vec_get,   NULL,                  NULL},
    {"##array_get.IVk", sizeof(VEC_GET),   0, "k",            ":IndexedVec;k",   (SUBR) vec_get,   (SUBR) vec_get_perf,   NULL},

    /* Two indices are one access, not two: the slice [from, to), copied. */
    {"##array_get.IVs", sizeof(VEC_SLICE), 0, ":IndexedVec;", ":IndexedVec;kk",  (SUBR) vec_slice, (SUBR) vec_slice_perf, NULL},

    /* "=" writes an i value at init time and a k value at perf time only,
       with exactly one index. */
    {"##array_set.IVi", sizeof(VEC_SET),   0, "",              ":IndexedVec;ii", (SUBR) vec_set,   NULL,                  NULL},
    {"##array_set.IVk", sizeof(VEC_SET),   0, "",              ":IndexedVec;kk", NULL,             (SUBR) vec_set_perf,   NULL},

    /* init writes at init time only, whatever the rate of its arguments. */
    {"##array_init.IVi", sizeof(VEC_SET),  0, "",              ":IndexedVec;ik", (SUBR) vec_set,   NULL,                  NULL},
    {"##array_init.IVk", sizeof(VEC_SET),  0, "",              ":IndexedVec;kk", (SUBR) vec_set,   NULL,                  NULL},

    /* + on the type is an ordinary "##add" entry, as for Complex. */
    {"##add.IV",        sizeof(VEC_ADD),   0, ":IndexedVec;",  ":IndexedVec;:IndexedVec;", (SUBR) vec_add, NULL,           NULL},

    {"##array_init.RVk", sizeof(VEC_SET),  0, "",               ":ReversedVec;kk", (SUBR) vec_set,  NULL,                  NULL},
    {"##array_init.RVi", sizeof(VEC_SET),  0, "",               ":ReversedVec;ik", (SUBR) vec_set,  NULL,                  NULL},
    {"##array_set.RVk", sizeof(VEC_SET),   0, "",               ":ReversedVec;kk", NULL,            (SUBR) vec_set_perf,   NULL},
    {"##array_set.RVi", sizeof(VEC_SET),   0, "",               ":ReversedVec;ii", (SUBR) vec_set,  NULL,                  NULL},
    {"##array_get.RVk", sizeof(VEC_GET),   0, "k",              ":ReversedVec;k",  (SUBR) vec_get,  (SUBR) vec_get_perf,   NULL},
    {"##array_get.RVi", sizeof(VEC_GET),   0, "i",              ":ReversedVec;i",  (SUBR) vec_get,  NULL,                  NULL},

    /* A k read is unique; the two i reads tie for an init-time read, and
       the two k writes tie for any write. */
    {"##array_get.AVk", sizeof(VEC_GET),   0, "k",              ":AmbiguousVec;k",  (SUBR) vec_get, (SUBR) vec_get_perf,   NULL},
    {"##array_get.AV1", sizeof(VEC_GET),   0, "i",              ":AmbiguousVec;i",  (SUBR) vec_get, NULL,                  NULL},
    {"##array_get.AV2", sizeof(VEC_GET),   0, "i",              ":AmbiguousVec;i",  (SUBR) vec_get, NULL,                  NULL},
    {"##array_set.AV1", sizeof(VEC_SET),   0, "",               ":AmbiguousVec;kk", NULL,           (SUBR) vec_set_perf,   NULL},
    {"##array_set.AV2", sizeof(VEC_SET),   0, "",               ":AmbiguousVec;kk", NULL,           (SUBR) vec_set_perf,   NULL},

    {"##array_get.IPi", sizeof(PAIR_GET),  0, "i",             ":IndexedPair;i", (SUBR) pair_get,  NULL,                  NULL},
    {"##array_get.IPp", sizeof(PAIR_PICK), 0, ":IndexedPair;", ":IndexedPair;ii",(SUBR) pair_pick, NULL,                  NULL},
    {"##array_set.IPi", sizeof(PAIR_SET),  0, "",              ":IndexedPair;ii",(SUBR) pair_set,  NULL,                  NULL}
  };

  return csound->AppendOpcodes(csound, opcodes, (int32_t)(sizeof(opcodes) / sizeof(opcodes[0])));
}

PUBLIC int32_t csoundModuleInfo(void) { return CSOUND_MODULE_INFO; }
