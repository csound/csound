/* JSON conversion for declared Csound types.
   SPDX-License-Identifier: LGPL-2.1-or-later */
#include "json_ops.h"
#include "csound_orc_structs.h"
#include "csound_standard_types.h"
#include "arrays.h"
#include "../third_party/yyjson/yyjson.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define JSON_MAX_DEPTH 256

typedef struct {
  const CS_VARIABLE *variable;
  size_t index;
} JSON_FIELD;

typedef struct json_layout {
  const CS_TYPE *type;
  JSON_FIELD *fields;
  size_t count;
  int checked;
  struct json_layout *next;
} JSON_LAYOUT;

typedef struct {
  yyjson_val **values;
  size_t capacity;
} JSON_BINDINGS;

typedef struct {
  CSOUND *csound;
  INSDS *instance;
  const char *error;
  unsigned maxdepth;
  JSON_LAYOUT *layouts;
  JSON_BINDINGS bindings[JSON_MAX_DEPTH + 1];
  char path[8192];
} JSON_CONTEXT;

static JSON_CONTEXT *new_context(CSOUND *csound, INSDS *instance, MYFLT depth)
{
  JSON_CONTEXT *ctx = csound->Calloc(csound, sizeof(*ctx));
  ctx->csound = csound;
  ctx->instance = instance;
  ctx->error = "could not allocate JSON value";
  ctx->maxdepth = depth ? (unsigned)depth : JSON_MAX_DEPTH;
  strcpy(ctx->path, "$");
  return ctx;
}

static void free_context(JSON_CONTEXT *ctx)
{
  CSOUND *csound = ctx->csound;
  while (ctx->layouts != NULL) {
    JSON_LAYOUT *next = ctx->layouts->next;
    csound->Free(csound, ctx->layouts->fields);
    csound->Free(csound, ctx->layouts);
    ctx->layouts = next;
  }
  for (unsigned i = 0; i <= JSON_MAX_DEPTH; ++i)
    csound->Free(csound, ctx->bindings[i].values);
  csound->Free(csound, ctx);
}

static size_t field_path(JSON_CONTEXT *ctx, const char *field)
{
  size_t saved = strlen(ctx->path);
  snprintf(ctx->path + saved, sizeof(ctx->path) - saved, ".%s", field);
  return saved;
}

static size_t index_path(JSON_CONTEXT *ctx, size_t index)
{
  size_t saved = strlen(ctx->path);
  snprintf(ctx->path + saved, sizeof(ctx->path) - saved, "[%zu]", index);
  return saved;
}

static int compare_fields(const void *left, const void *right)
{
  return strcmp(((const JSON_FIELD *)left)->variable->varName,
                ((const JSON_FIELD *)right)->variable->varName);
}

/* Build the name index once per type per call; reuse it across array elements.
   Binding buffers are also reused at each depth, not allocated for each note. */
static JSON_LAYOUT *layout_for(JSON_CONTEXT *ctx, const CS_TYPE *type)
{
  for (JSON_LAYOUT *item = ctx->layouts; item != NULL; item = item->next)
    if (item->type == type)
      return item;
  JSON_LAYOUT *layout = ctx->csound->Calloc(ctx->csound, sizeof(*layout));
  layout->type = type;
  layout->count = (size_t)cs_cons_length(type->members);
  layout->fields = ctx->csound->Calloc(ctx->csound,
                                      layout->count * sizeof(JSON_FIELD));
  CONS_CELL *cell = type->members;
  for (size_t i = 0; i < layout->count; ++i, cell = cell->next) {
    layout->fields[i].variable = cell->value;
    layout->fields[i].index = i;
  }
  qsort(layout->fields, layout->count, sizeof(JSON_FIELD), compare_fields);
  layout->next = ctx->layouts;
  ctx->layouts = layout;
  return layout;
}

static const JSON_FIELD *find_field(const JSON_LAYOUT *layout, const char *name)
{
  size_t begin = 0, end = layout->count;
  while (begin < end) {
    size_t middle = begin + (end - begin) / 2;
    int order = strcmp(name, layout->fields[middle].variable->varName);
    if (order == 0) return &layout->fields[middle];
    if (order < 0) end = middle;
    else begin = middle + 1;
  }
  return NULL;
}

/* Check declarations even when an array has no elements. The visited marker
   permits recursive types through arrays without walking the same type twice. */
static int check_type(JSON_CONTEXT *ctx, const CS_TYPE *type,
                      const CS_TYPE *element_type, unsigned depth)
{
  if (type == &CS_VAR_TYPE_I || type == &CS_VAR_TYPE_K ||
      type == &CS_VAR_TYPE_S || type == &CS_VAR_TYPE_b || type == &CS_VAR_TYPE_B)
    return OK;
  if (type == NULL) {
    ctx->error = "unsupported array element type";
    return NOTOK;
  }
  if (type->userDefinedType) {
    JSON_LAYOUT *layout = layout_for(ctx, type);
    if (layout->checked) return OK;
    if (depth > JSON_MAX_DEPTH) {
      ctx->error = "maximum type nesting depth exceeded";
      return NOTOK;
    }
    layout->checked = 1;
    for (CONS_CELL *cell = type->members; cell != NULL; cell = cell->next) {
      const CS_VARIABLE *field = cell->value;
      size_t saved = field_path(ctx, field->varName);
      if (check_type(ctx, field->varType, field->subType, depth + 1) != OK)
        return NOTOK;
      ctx->path[saved] = '\0';
    }
    return OK;
  }
  if (type == &CS_VAR_TYPE_ARRAY && depth <= JSON_MAX_DEPTH)
    return check_type(ctx, element_type, NULL, depth + 1);
  ctx->error = "unsupported member type";
  return NOTOK;
}

static int integer_option(MYFLT value, unsigned maximum)
{
  return isfinite(value) && value >= 0 && value <= maximum &&
         floor((double)value) == (double)value;
}

/* Inspect the complete input before mapping fields. This also bounds data in
   unknown fields. yyjson parses iteratively; this walk must do so as well. */
static int check_depth(JSON_CONTEXT *ctx, yyjson_val *value)
{
  typedef struct {
    int object;
    union {
      yyjson_obj_iter object;
      yyjson_arr_iter array;
    } iter;
  } FRAME;
  FRAME *stack = ctx->csound->Calloc(ctx->csound,
                                    ctx->maxdepth * sizeof(FRAME));
  unsigned used = 0;
  int result = OK;
  for (;;) {
    if (yyjson_is_ctn(value)) {
      if (used == ctx->maxdepth) {
        ctx->error = "maximum nesting depth exceeded";
        result = NOTOK;
        break;
      }
      FRAME *frame = &stack[used++];
      frame->object = yyjson_is_obj(value);
      if (frame->object)
        yyjson_obj_iter_init(value, &frame->iter.object);
      else
        yyjson_arr_iter_init(value, &frame->iter.array);
    }
    value = NULL;
    while (used > 0 && value == NULL) {
      FRAME *frame = &stack[used - 1];
      if (frame->object) {
        yyjson_val *key = yyjson_obj_iter_next(&frame->iter.object);
        if (key != NULL) value = yyjson_obj_iter_get_val(key);
      }
      else
        value = yyjson_arr_iter_next(&frame->iter.array);
      if (value == NULL) --used;
    }
    if (value == NULL) break;
  }
  ctx->csound->Free(ctx->csound, stack);
  return result;
}

static int decode_value(JSON_CONTEXT *ctx, const CS_TYPE *type, void *value,
                        yyjson_val *json, unsigned depth)
{
  CSOUND *csound = ctx->csound;
  if ((type->userDefinedType || type == &CS_VAR_TYPE_ARRAY) &&
      depth > ctx->maxdepth) {
    ctx->error = "maximum nesting depth exceeded";
    return NOTOK;
  }
  if (type->userDefinedType) {
    CS_STRUCT_VAR *object = value;
    if (!yyjson_is_obj(json)) {
      ctx->error = "expected an object";
      return NOTOK;
    }
    JSON_LAYOUT *layout = layout_for(ctx, type);
    JSON_BINDINGS *bindings = &ctx->bindings[depth];
    if (bindings->capacity < layout->count) {
      bindings->values = csound->ReAlloc(csound, bindings->values,
                                        layout->count * sizeof(yyjson_val *));
      bindings->capacity = layout->count;
    }
    memset(bindings->values, 0, layout->count * sizeof(yyjson_val *));
    yyjson_obj_iter iter = yyjson_obj_iter_with(json);
    yyjson_val *key;
    while ((key = yyjson_obj_iter_next(&iter)) != NULL) {
      const char *name = yyjson_get_str(key);
      if (memchr(name, '\0', yyjson_get_len(key)) != NULL) {
        ctx->error = "NUL in object key";
        return NOTOK;
      }
      size_t saved = field_path(ctx, name);
      const JSON_FIELD *field = find_field(layout, name);
      if (field == NULL) {
        ctx->error = "unknown field";
        return NOTOK;
      }
      if (bindings->values[field->index] != NULL) {
        ctx->error = "duplicate field";
        return NOTOK;
      }
      bindings->values[field->index] = yyjson_obj_iter_get_val(key);
      ctx->path[saved] = '\0';
    }
    CONS_CELL *cell = type->members;
    for (size_t i = 0; i < layout->count; ++i, cell = cell->next) {
      const CS_VARIABLE *field = cell->value;
      size_t saved = field_path(ctx, field->varName);
      if (bindings->values[i] == NULL) {
        ctx->error = "missing field";
        return NOTOK;
      }
      if (decode_value(ctx, field->varType, &object->members[i]->value,
                       bindings->values[i], depth + 1) != OK)
        return NOTOK;
      ctx->path[saved] = '\0';
    }
    return OK;
  }
  if (type == &CS_VAR_TYPE_ARRAY) {
    ARRAYDAT *array = value;
    if (!yyjson_is_arr(json) || array->arrayType == NULL ||
        array->dimensions != 1 || yyjson_arr_size(json) > INT_MAX) {
      ctx->error = "expected a one-dimensional typed array";
      return NOTOK;
    }
    if (tabinit(csound, array, (int32_t)yyjson_arr_size(json),
                ctx->instance) != OK) {
      ctx->error = "could not allocate array";
      return NOTOK;
    }
    size_t i, count;
    yyjson_val *item;
    yyjson_arr_foreach(json, i, count, item) {
      void *element = (char *)array->data + i * array->arrayMemberSize;
      size_t saved = index_path(ctx, i);
      if (decode_value(ctx, array->arrayType, element, item, depth + 1) != OK)
        return NOTOK;
      ctx->path[saved] = '\0';
    }
    return OK;
  }
  if (type == &CS_VAR_TYPE_S) {
    const char *text = yyjson_get_str(json);
    size_t length = yyjson_get_len(json);
    if (text == NULL || memchr(text, '\0', length) != NULL ||
        length >= MAX_STRINGDAT_SIZE) {
      ctx->error = "expected a string without NUL bytes";
      return NOTOK;
    }
    STRINGDAT source = {(char *)text, length + 1, 0};
    type->copyValue(csound, type, value, &source, ctx->instance);
    return OK;
  }
  if (type == &CS_VAR_TYPE_I || type == &CS_VAR_TYPE_K) {
    double number = yyjson_get_num(json);
    if (!yyjson_is_num(json) || !isfinite(number) ||
        !isfinite((MYFLT)number)) {
      ctx->error = "expected a finite number in MYFLT range";
      return NOTOK;
    }
    *(MYFLT *)value = (MYFLT)number;
    return OK;
  }
  if (type == &CS_VAR_TYPE_b || type == &CS_VAR_TYPE_B) {
    if (!yyjson_is_bool(json)) {
      ctx->error = "expected a Boolean";
      return NOTOK;
    }
    *(int32_t *)value = yyjson_get_bool(json) ? 1 : 0;
    return OK;
  }
  ctx->error = "unsupported member type";
  return NOTOK;
}

static yyjson_mut_val *encode_value(JSON_CONTEXT *ctx, yyjson_mut_doc *doc,
                                    const CS_TYPE *type, const void *value,
                                    unsigned depth)
{
  if ((type->userDefinedType || type == &CS_VAR_TYPE_ARRAY) &&
      depth > ctx->maxdepth) {
    ctx->error = "maximum nesting depth exceeded";
    return NULL;
  }
  if (type->userDefinedType) {
    const CS_STRUCT_VAR *object = value;
    yyjson_mut_val *json = yyjson_mut_obj(doc);
    CONS_CELL *cell = type->members;
    if (json == NULL || object->members == NULL)
      return NULL;
    for (int32_t i = 0; i < object->memberCount; ++i, cell = cell->next) {
      const CS_VARIABLE *field = cell->value;
      size_t saved = field_path(ctx, field->varName);
      yyjson_mut_val *item = encode_value(ctx, doc, field->varType,
                                          &object->members[i]->value, depth + 1);
      if (item == NULL || !yyjson_mut_obj_add_val(doc, json,
                                                 field->varName, item))
        return NULL;
      ctx->path[saved] = '\0';
    }
    return json;
  }
  if (type == &CS_VAR_TYPE_ARRAY) {
    const ARRAYDAT *array = value;
    yyjson_mut_val *json = yyjson_mut_arr(doc);
    size_t count;
    if (json == NULL || array->arrayType == NULL || array->dimensions != 1 ||
        csound_array_member_count(array, &count) != OK ||
        (count > 0 && (array->data == NULL || array->arrayMemberSize <= 0))) {
      ctx->error = "expected a one-dimensional typed array";
      return NULL;
    }
    for (size_t i = 0; i < count; ++i) {
      const void *element = (char *)array->data + i * array->arrayMemberSize;
      size_t saved = index_path(ctx, i);
      yyjson_mut_val *item = encode_value(ctx, doc, array->arrayType,
                                          element, depth + 1);
      if (item == NULL || !yyjson_mut_arr_append(json, item))
        return NULL;
      ctx->path[saved] = '\0';
    }
    return json;
  }
  if (type == &CS_VAR_TYPE_S) {
    const STRINGDAT *text = value;
    return yyjson_mut_strcpy(doc, text->data != NULL ? text->data : "");
  }
  if (type == &CS_VAR_TYPE_I || type == &CS_VAR_TYPE_K) {
    if (!isfinite(*(const MYFLT *)value)) {
      ctx->error = "expected a finite number";
      return NULL;
    }
    return yyjson_mut_real(doc, (double)*(const MYFLT *)value);
  }
  if (type == &CS_VAR_TYPE_b || type == &CS_VAR_TYPE_B)
    return yyjson_mut_bool(doc, *(const int32_t *)value != 0);
  ctx->error = "unsupported member type";
  return NULL;
}

/* Compiled member expressions retain their storage addresses. Commit into
   those slots, not by replacing the struct's member table. */
static void move_value(CSOUND *csound, const CS_TYPE *type, void *destination,
                       void *source, size_t size)
{
  if (type->userDefinedType) {
    CS_STRUCT_VAR *dest = destination, *src = source;
    CONS_CELL *cell = type->members;
    for (int32_t i = 0; i < dest->memberCount; ++i, cell = cell->next) {
      const CS_VARIABLE *field = cell->value;
      move_value(csound, field->varType, &dest->members[i]->value,
                 &src->members[i]->value, field->memBlockSize);
    }
  }
  else {
    if (type->freeVariableMemory != NULL)
      type->freeVariableMemory(csound, destination);
    memcpy(destination, source, size);
    memset(source, 0, size);
  }
}

static int32_t unmarshal(CSOUND *csound, JSON_UNMARSHAL *p, int from_file)
{
  const CS_TYPE *type = GetTypeForArg(p->out);
  const char *opcode = from_file ? "jsonunmarshalfile" : "jsonunmarshal";
  yyjson_read_flag flags = 0;
  yyjson_read_err read_error;
  yyjson_doc *doc;
  if (!integer_option(*p->flags, 3) ||
      !integer_option(*p->maxdepth, JSON_MAX_DEPTH))
    return csound->InitError(csound, "%s: invalid flags or maximum depth", opcode);
  if (p->source->data == NULL)
    return csound->InitError(csound, "%s: empty source", opcode);
  if ((unsigned)*p->flags & 1) flags |= YYJSON_READ_ALLOW_COMMENTS;
  if ((unsigned)*p->flags & 2) flags |= YYJSON_READ_ALLOW_TRAILING_COMMAS;
  if (from_file) {
    FILE *file;
    void *handle = csound->FileOpen(csound, &file, CSFILE_STD, p->source->data,
                                    "rb", "INCDIR;SSDIR;SFDIR",
                                    CSFTYPE_OTHER_TEXT, 0);
    if (handle == NULL)
      return csound->InitError(csound, "%s: cannot open file '%s'", opcode,
                               p->source->data);
    doc = yyjson_read_fp(file, flags, NULL, &read_error);
    csound->FileClose(csound, handle, CSFILE_CLOSE_SYNC);
  }
  else
    doc = yyjson_read_opts(p->source->data, strlen(p->source->data),
                           flags, NULL, &read_error);
  if (doc == NULL)
    return csound->InitError(csound, "%s: %s at byte %zu", opcode,
                             read_error.msg, read_error.pos);
  yyjson_val *object = yyjson_doc_get_root(doc);
  CS_VARIABLE *variable = NULL;
  void *value = NULL;
  const char *error = "expected a declared UDT or typed array";
  JSON_CONTEXT *context = new_context(csound, p->h.insdshead, *p->maxdepth);
  int32_t result = NOTOK;
  if (!type->userDefinedType && type != &CS_VAR_TYPE_ARRAY)
    goto done;
  if (check_depth(context, object) != OK) {
    error = context->error;
    goto done;
  }
  const CS_TYPE *element_type = type == &CS_VAR_TYPE_ARRAY
    ? ((ARRAYDAT *)p->out)->arrayType : NULL;
  if (check_type(context, type, element_type, 1) != OK) {
    error = context->error;
    goto done;
  }
  ARRAY_VAR_INIT array_init;
  if (type == &CS_VAR_TYPE_ARRAY) {
    const ARRAYDAT *destination = p->out;
    array_init.type = destination->arrayType;
    array_init.dimensions = destination->dimensions;
  }
  variable = csoundCreateVariableForType(
    csound, type, type == &CS_VAR_TYPE_ARRAY ? &array_init : NULL,
    p->h.insdshead);
  if (variable == NULL) {
    error = "could not create destination value";
    goto done;
  }
  value = csound->Calloc(csound, variable->memBlockSize);
  variable->initializeVariableMemory(csound, variable, (MYFLT *)value);
  if (decode_value(context, type, value, object, 1) != OK) {
    error = context->error;
    goto done;
  }
  move_value(csound, type, p->out, value, variable->memBlockSize);
  result = OK;
done:
  if (value != NULL) {
    type->freeVariableMemory(csound, value);
    csound->Free(csound, value);
  }
  csound->Free(csound, variable);
  yyjson_doc_free(doc);
  if (result != OK)
    result = csound->InitError(csound, "%s: %s at %s", opcode, error,
                               context->path);
  free_context(context);
  return result;
}

int32_t json_unmarshal(CSOUND *csound, JSON_UNMARSHAL *p)
{
  return unmarshal(csound, p, 0);
}

int32_t json_unmarshal_file(CSOUND *csound, JSON_UNMARSHAL *p)
{
  return unmarshal(csound, p, 1);
}

int32_t json_marshal(CSOUND *csound, JSON_MARSHAL *p)
{
  const CS_TYPE *type = GetTypeForArg(p->value);
  if (!integer_option(*p->pretty, 1) ||
      !integer_option(*p->maxdepth, JSON_MAX_DEPTH))
    return csound->InitError(csound,
                            "jsonmarshal: invalid pretty or maximum depth option");
  yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
  JSON_CONTEXT *context = new_context(csound, p->h.insdshead, *p->maxdepth);
  yyjson_mut_val *object = NULL;
  char *encoded = NULL;
  const char *error = "expected a declared UDT or typed array";
  int32_t result = NOTOK;
  if ((!type->userDefinedType && type != &CS_VAR_TYPE_ARRAY) || doc == NULL)
    goto done;
  const CS_TYPE *element_type = type == &CS_VAR_TYPE_ARRAY
    ? ((ARRAYDAT *)p->value)->arrayType : NULL;
  if (check_type(context, type, element_type, 1) != OK) {
    error = context->error;
    goto done;
  }
  object = encode_value(context, doc, type, p->value, 1);
  if (object == NULL) {
    error = context->error;
    goto done;
  }
  yyjson_mut_doc_set_root(doc, object);
  size_t length;
  encoded = yyjson_mut_write(doc, *p->pretty ? YYJSON_WRITE_PRETTY : 0, &length);
  if (encoded == NULL) {
    error = "could not encode JSON";
    goto done;
  }
  if (length >= MAX_STRINGDAT_SIZE) {
    error = "encoded JSON exceeds the Csound string size limit";
    goto done;
  }
  STRINGDAT source = {encoded, length + 1, 0};
  CS_VAR_TYPE_S.copyValue(csound, &CS_VAR_TYPE_S, p->out, &source,
                         p->h.insdshead);
  result = OK;
done:
  free(encoded);
  yyjson_mut_doc_free(doc);
  if (result != OK)
    result = csound->InitError(csound, "jsonmarshal: %s at %s", error,
                               context->path);
  free_context(context);
  return result;
}
