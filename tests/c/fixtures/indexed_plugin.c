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
  cs_float *index[VARGMAX];
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

static int32_t vec_get(CSOUND *csound, VEC_GET *p)
{
  size_t i = (size_t)*p->index;
  if (i >= p->vec->size)
    return csound->InitError(csound, "IndexedVec read out of range");
  *p->out = p->vec->data[i];
  return OK;
}

static int32_t vec_get_perf(CSOUND *csound, VEC_GET *p)
{
  size_t i = (size_t)*p->index;
  if (i >= p->vec->size)
    return csound->PerfError(csound, &p->h, "IndexedVec read out of range");
  *p->out = p->vec->data[i];
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
  size_t from = (size_t)*p->from, to = (size_t)*p->to;
  if (from > to || to > p->vec->size)
    return csound->InitError(csound, "IndexedVec slice out of range");
  if (vec_reserve(csound, p->out, to - from) != OK) return NOTOK;
  return vec_slice_copy(p->out, p->vec, from, to);
}

static int32_t vec_slice_perf(CSOUND *csound, VEC_SLICE *p)
{
  size_t from = (size_t)*p->from, to = (size_t)*p->to;
  if (from > to || to > p->vec->size || to - from > p->out->capacity)
    return csound->PerfError(csound, &p->h, "IndexedVec slice out of range");
  return vec_slice_copy(p->out, p->vec, from, to);
}

static int32_t vec_set(CSOUND *csound, VEC_SET *p)
{
  size_t i = (size_t)*p->index[0];
  if (i >= p->vec->size)
    return csound->InitError(csound, "IndexedVec write out of range");
  p->vec->data[i] = *p->value;
  return OK;
}

static int32_t vec_set_perf(CSOUND *csound, VEC_SET *p)
{
  size_t i = (size_t)*p->index[0];
  if (i >= p->vec->size)
    return csound->PerfError(csound, &p->h, "IndexedVec write out of range");
  p->vec->data[i] = *p->value;
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
  cs_float *index[VARGMAX];
} PAIR_SET;

static int32_t pair_get(CSOUND *csound, PAIR_GET *p)
{
  int32_t i = (int32_t)*p->index;
  if (i < 0 || i >= p->pair->memberCount)
    return csound->InitError(csound, "IndexedPair read out of range");
  *p->out = p->pair->members[i]->value;
  return OK;
}

static int32_t pair_set(CSOUND *csound, PAIR_SET *p)
{
  int32_t i = (int32_t)*p->index[0];
  if (i < 0 || i >= p->pair->memberCount)
    return csound->InitError(csound, "IndexedPair write out of range");
  p->pair->members[i]->value = *p->value;
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
    csound->RegisterStruct(csound, "IndexedPair", pair, 2) ? OK : NOTOK;
}

PUBLIC int32_t csoundModuleInit(CSOUND *csound)
{
  TYPE_POOL *pool = csound->GetTypePool(csound);
  if (pool == NULL) return OK;
  if (register_types(csound, pool) != OK) return NOTOK;

  static OENTRY opcodes[] = {
    {"indexed_vec",     sizeof(VEC_NEW),   0, ":IndexedVec;", "i",               (SUBR) vec_new,   NULL,                  NULL},
    {"opaque_vec",      sizeof(VEC_NEW),   0, ":OpaqueVec;",  "i",               (SUBR) vec_new,   NULL,                  NULL},
    {"indexed_vec_len", sizeof(VEC_LEN),   0, "i",            ":IndexedVec;",    (SUBR) vec_len,   NULL,                  NULL},

    /* One index reads an element at the index's rate. */
    {"##array_get.IVi", sizeof(VEC_GET),   0, "i",            ":IndexedVec;i",   (SUBR) vec_get,   NULL,                  NULL},
    {"##array_get.IVk", sizeof(VEC_GET),   0, "k",            ":IndexedVec;k",   (SUBR) vec_get,   (SUBR)vec_get_perf,    NULL},

    /* Two indices read the slice [from, to) as another IndexedVec. */
    {"##array_get.IVs", sizeof(VEC_SLICE), 0, ":IndexedVec;", ":IndexedVec;kk",  (SUBR) vec_slice, (SUBR) vec_slice_perf, NULL},

    /* The first matching value type wins: i values are written once. */
    {"##array_set.IVi", sizeof(VEC_SET),   0, "",              ":IndexedVec;im", (SUBR) vec_set,   NULL,                  NULL},
    {"##array_set.IVk", sizeof(VEC_SET),   0, "",              ":IndexedVec;kz", (SUBR) vec_set,   (SUBR) vec_set_perf,   NULL},

    {"##array_get.IPi", sizeof(PAIR_GET),  0, "i",             ":IndexedPair;i", (SUBR) pair_get,  NULL,                  NULL},
    {"##array_set.IPi", sizeof(PAIR_SET),  0, "",              ":IndexedPair;im",(SUBR) pair_set,  NULL,                  NULL}
  };

  return csound->AppendOpcodes(csound, opcodes, (int32_t)(sizeof(opcodes) / sizeof(opcodes[0])));
}

PUBLIC int32_t csoundModuleInfo(void) { return CSOUND_MODULE_INFO; }
