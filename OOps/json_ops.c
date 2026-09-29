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

static JSON_CONTEXT *new_context(CSOUND *csound, INSDS *instance, uint32_t depth)
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

static int integer_option(cs_float value, unsigned maximum)
{
  return isfinite(value) && value >= 0 && value <= maximum &&
         FLOOR(value) == value;
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
    /* yyjson uses double regardless of the Csound precision mode. */
    double number = yyjson_get_num(json);
    if (!yyjson_is_num(json) || !isfinite(number) ||
        !isfinite((cs_float)number)) {
      ctx->error = "expected a finite number in cs_float range";
      return NOTOK;
    }
    *(cs_float *)value = (cs_float)number;
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
    if (!isfinite(*(const cs_float *)value)) {
      ctx->error = "expected a finite number";
      return NULL;
    }
    return yyjson_mut_real(doc, (double)*(const cs_float *)value);
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

static int32_t decode_document(CSOUND *csound, INSDS *instance, void *output,
                                yyjson_doc *doc, uint32_t maxdepth,
                                const char *opcode)
{
  const CS_TYPE *type = output != NULL ? GetTypeForArg(output) : NULL;
  if (type == NULL || maxdepth > JSON_MAX_DEPTH)
    return csound->InitError(csound, "%s: invalid destination or maximum depth", opcode);
  yyjson_val *object = yyjson_doc_get_root(doc);
  CS_VARIABLE *variable = NULL;
  void *value = NULL;
  const char *error = "expected a declared UDT or typed array";
  JSON_CONTEXT *context = new_context(csound, instance, maxdepth);
  int32_t result = NOTOK;
  if (!type->userDefinedType && type != &CS_VAR_TYPE_ARRAY)
    goto done;
  if (check_depth(context, object) != OK) {
    error = context->error;
    goto done;
  }
  const CS_TYPE *element_type = type == &CS_VAR_TYPE_ARRAY
    ? ((ARRAYDAT *)output)->arrayType : NULL;
  if (check_type(context, type, element_type, 1) != OK) {
    error = context->error;
    goto done;
  }
  ARRAY_VAR_INIT array_init;
  if (type == &CS_VAR_TYPE_ARRAY) {
    const ARRAYDAT *destination = output;
    array_init.type = destination->arrayType;
    array_init.dimensions = destination->dimensions;
  }
  variable = csoundCreateVariableForType(
    csound, type, type == &CS_VAR_TYPE_ARRAY ? &array_init : NULL,
    instance);
  if (variable == NULL) {
    error = "could not create destination value";
    goto done;
  }
  value = csound->Calloc(csound, variable->memBlockSize);
  variable->initializeVariableMemory(csound, variable, (cs_float *)value);
  if (decode_value(context, type, value, object, 1) != OK) {
    error = context->error;
    goto done;
  }
  move_value(csound, type, output, value, variable->memBlockSize);
  result = OK;
done:
  if (value != NULL) {
    type->freeVariableMemory(csound, value);
    csound->Free(csound, value);
  }
  csound->Free(csound, variable);
  if (result != OK)
    result = csound->InitError(csound, "%s: %s at %s", opcode, error,
                               context->path);
  free_context(context);
  return result;
}

static int32_t unmarshal(CSOUND *csound, JSON_UNMARSHAL *p, int from_file)
{
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
  int32_t result = decode_document(csound, p->h.insdshead, p->out, doc,
                                    (uint32_t)*p->maxdepth, opcode);
  yyjson_doc_free(doc);
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
  JSON_CONTEXT *context = new_context(csound, p->h.insdshead, (uint32_t)*p->maxdepth);
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

/* The plugin interface keeps yyjson's layout and allocator private. */
static void json_error(CSOUND_JSON_ERROR *error, const char *message, size_t pos)
{
  if (error == NULL) return;
  snprintf(error->message, sizeof(error->message), "%s", message);
  error->position = pos;
}

static yyjson_read_flag json_read_flags(uint32_t flags)
{
  return ((flags & 1) ? YYJSON_READ_ALLOW_COMMENTS : 0) |
         ((flags & 2) ? YYJSON_READ_ALLOW_TRAILING_COMMAS : 0);
}

static CSOUND_JSON_DOCUMENT *json_editable(CSOUND *csound, yyjson_doc *doc,
                                          CSOUND_JSON_ERROR *error)
{
  JSON_CONTEXT *context = new_context(csound, NULL, 0);
  int result = check_depth(context, yyjson_doc_get_root(doc));
  if (result != OK) json_error(error, context->error, 0);
  free_context(context);
  if (result != OK) {
    yyjson_doc_free(doc);
    return NULL;
  }
  yyjson_mut_doc *mutable_doc = yyjson_doc_mut_copy(doc, NULL);
  yyjson_doc_free(doc);
  if (mutable_doc == NULL) json_error(error, "could not allocate JSON document", 0);
  return (CSOUND_JSON_DOCUMENT *)mutable_doc;
}

static CSOUND_JSON_DOCUMENT *plugin_json_parse(CSOUND *csound, const char *source, size_t length,
                                               uint32_t flags,
                                               CSOUND_JSON_ERROR *error)
{
  if (error != NULL) memset(error, 0, sizeof(*error));
  if (csound == NULL || source == NULL || flags > 3) {
    json_error(error, "invalid JSON source or flags", 0);
    return NULL;
  }
  yyjson_read_err read_error;
  yyjson_doc *doc = yyjson_read_opts((char *)source, length,
                                    json_read_flags(flags), NULL, &read_error);
  if (doc == NULL) {
    json_error(error, read_error.msg, read_error.pos);
    return NULL;
  }
  return json_editable(csound, doc, error);
}

static CSOUND_JSON_DOCUMENT *plugin_json_parse_file(CSOUND *csound,
                         const char *path, uint32_t flags, CSOUND_JSON_ERROR *error)
{
  if (error != NULL) memset(error, 0, sizeof(*error));
  if (csound == NULL || path == NULL || flags > 3) {
    json_error(error, "invalid JSON path or flags", 0);
    return NULL;
  }
  FILE *file;
  void *handle = csound->FileOpen(csound, &file, CSFILE_STD, path, "rb",
                                  "INCDIR;SSDIR;SFDIR", CSFTYPE_OTHER_TEXT, 0);
  if (handle == NULL) {
    json_error(error, "cannot open JSON file", 0);
    return NULL;
  }
  yyjson_read_err read_error;
  yyjson_doc *doc = yyjson_read_fp(file, json_read_flags(flags), NULL, &read_error);
  csound->FileClose(csound, handle, CSFILE_CLOSE_SYNC);
  if (doc == NULL) {
    json_error(error, read_error.msg, read_error.pos);
    return NULL;
  }
  return json_editable(csound, doc, error);
}

static void plugin_json_free(CSOUND_JSON_DOCUMENT *doc)
{ yyjson_mut_doc_free((yyjson_mut_doc *)doc); }

static CSOUND_JSON_VALUE *plugin_json_root(CSOUND_JSON_DOCUMENT *doc)
{ return (CSOUND_JSON_VALUE *)yyjson_mut_doc_get_root((yyjson_mut_doc *)doc); }

static CSOUND_JSON_KIND plugin_json_kind(const CSOUND_JSON_VALUE *value)
{
  yyjson_mut_val *v = (yyjson_mut_val *)value;
  if (yyjson_mut_is_null(v)) return CSOUND_JSON_NULL;
  if (yyjson_mut_is_bool(v)) return CSOUND_JSON_BOOLEAN;
  if (yyjson_mut_is_num(v)) return CSOUND_JSON_NUMBER;
  if (yyjson_mut_is_str(v)) return CSOUND_JSON_STRING;
  if (yyjson_mut_is_arr(v)) return CSOUND_JSON_ARRAY;
  if (yyjson_mut_is_obj(v)) return CSOUND_JSON_OBJECT;
  return CSOUND_JSON_INVALID;
}

static size_t plugin_json_size(const CSOUND_JSON_VALUE *value)
{ return yyjson_mut_get_len((yyjson_mut_val *)value); }

static CSOUND_JSON_VALUE *plugin_json_member(const CSOUND_JSON_VALUE *value,
                                             const char *key)
{ return (CSOUND_JSON_VALUE *)yyjson_mut_obj_get((yyjson_mut_val *)value, key); }

static CSOUND_JSON_VALUE *plugin_json_element(const CSOUND_JSON_VALUE *value,
                                              size_t index)
{ return (CSOUND_JSON_VALUE *)yyjson_mut_arr_get((yyjson_mut_val *)value, index); }

static double plugin_json_number(const CSOUND_JSON_VALUE *value)
{ return yyjson_mut_get_num((yyjson_mut_val *)value); }

static const char *plugin_json_string(const CSOUND_JSON_VALUE *value)
{ return yyjson_mut_get_str((yyjson_mut_val *)value); }

static int32_t plugin_json_boolean(const CSOUND_JSON_VALUE *value)
{ return yyjson_mut_get_bool((yyjson_mut_val *)value); }

static int32_t plugin_json_rename(CSOUND_JSON_DOCUMENT *doc,
                    CSOUND_JSON_VALUE *object, const char *old, const char *name)
{
  yyjson_mut_val *value = (yyjson_mut_val *)object;
  if (doc == NULL || old == NULL || name == NULL ||
      yyjson_mut_obj_get(value, name) != NULL) return NOTOK;
  return yyjson_mut_obj_rename_key((yyjson_mut_doc *)doc, value, old, name)
    ? OK : NOTOK;
}

static int32_t plugin_json_set_number(CSOUND_JSON_VALUE *value, double number)
{
  if (!isfinite(number) || plugin_json_kind(value) != CSOUND_JSON_NUMBER)
    return NOTOK;
  return yyjson_mut_set_real((yyjson_mut_val *)value, number) ? OK : NOTOK;
}

static int32_t plugin_json_set_string(CSOUND_JSON_DOCUMENT *doc,
                                      CSOUND_JSON_VALUE *value, const char *text)
{
  if (doc == NULL || text == NULL || plugin_json_kind(value) != CSOUND_JSON_STRING)
    return NOTOK;
  yyjson_mut_val *copy = yyjson_mut_strcpy((yyjson_mut_doc *)doc, text);
  return copy != NULL && yyjson_mut_set_strn((yyjson_mut_val *)value,
                yyjson_mut_get_str(copy), yyjson_mut_get_len(copy)) ? OK : NOTOK;
}

static int32_t plugin_json_remove(CSOUND_JSON_VALUE *value, const char *key)
{ return yyjson_mut_obj_remove_key((yyjson_mut_val *)value, key) != NULL ? OK : NOTOK; }

static int32_t plugin_json_write(CSOUND *csound, CSOUND_JSON_DOCUMENT *doc,
                                  STRINGDAT *out, uint32_t pretty)
{
  if (doc == NULL || out == NULL || pretty > 1)
    return csound->InitError(csound, "JSON: invalid document, output or pretty option");
  size_t length;
  char *text = yyjson_mut_write((yyjson_mut_doc *)doc,
                                pretty ? YYJSON_WRITE_PRETTY : 0, &length);
  if (text == NULL || length >= MAX_STRINGDAT_SIZE) {
    free(text);
    return csound->InitError(csound, "JSON: could not write document");
  }
  STRINGDAT source = {text, length + 1, 0};
  CS_VAR_TYPE_S.copyValue(csound, &CS_VAR_TYPE_S, out, &source, NULL);
  free(text);
  return OK;
}

static int32_t plugin_json_decode(CSOUND *csound, INSDS *instance, void *out,
                                   CSOUND_JSON_DOCUMENT *doc, uint32_t depth)
{
  yyjson_doc *input = yyjson_mut_doc_imut_copy((yyjson_mut_doc *)doc, NULL);
  if (input == NULL)
    return csound->InitError(csound, "JSON: invalid document or allocation failed");
  int32_t result = decode_document(csound, instance, out, input, depth, "JSON");
  yyjson_doc_free(input);
  return result;
}

static int32_t plugin_json_unmarshal_common(CSOUND *csound, INSDS *instance,
                 void *out, const char *source, uint32_t flags, uint32_t depth,
                 int from_file)
{
  if (flags > 3 || depth > JSON_MAX_DEPTH)
    return csound->InitError(csound, "JSON: invalid flags or maximum depth");
  cs_float options = (cs_float)flags, maxdepth = (cs_float)depth;
  STRINGDAT text = {(char *)source, 0, 0};
  JSON_UNMARSHAL opcode = {0};
  opcode.h.insdshead = instance;
  opcode.out = out;
  opcode.source = &text;
  opcode.flags = &options;
  opcode.maxdepth = &maxdepth;
  return unmarshal(csound, &opcode, from_file);
}

static int32_t plugin_json_unmarshal(CSOUND *csound, INSDS *instance, void *out,
                          const char *source, uint32_t flags, uint32_t depth)
{ return plugin_json_unmarshal_common(csound, instance, out, source, flags, depth, 0); }

static int32_t plugin_json_unmarshal_file(CSOUND *csound, INSDS *instance, void *out,
                          const char *source, uint32_t flags, uint32_t depth)
{ return plugin_json_unmarshal_common(csound, instance, out, source, flags, depth, 1); }

static int32_t plugin_json_marshal(CSOUND *csound, INSDS *instance, STRINGDAT *out,
                                   void *value, uint32_t pretty, uint32_t depth)
{
  if (out == NULL || value == NULL || pretty > 1 || depth > JSON_MAX_DEPTH)
    return csound->InitError(csound, "JSON: invalid value, pretty or maximum depth");
  cs_float options = (cs_float)pretty, maxdepth = (cs_float)depth;
  JSON_MARSHAL opcode = {0};
  opcode.h.insdshead = instance;
  opcode.out = out;
  opcode.value = value;
  opcode.pretty = &options;
  opcode.maxdepth = &maxdepth;
  return json_marshal(csound, &opcode);
}

const CSOUND_JSON_API *csoundGetJsonAPI(uint32_t version)
{
  static const CSOUND_JSON_API api = {
    CSOUND_JSON_API_VERSION, sizeof(CSOUND_JSON_API),
    plugin_json_parse, plugin_json_parse_file, plugin_json_free, plugin_json_root,
    plugin_json_kind, plugin_json_size, plugin_json_member, plugin_json_element,
    plugin_json_number, plugin_json_string, plugin_json_boolean,
    plugin_json_rename, plugin_json_set_number, plugin_json_set_string,
    plugin_json_remove, plugin_json_write, plugin_json_decode,
    plugin_json_unmarshal, plugin_json_unmarshal_file, plugin_json_marshal
  };
  return version == CSOUND_JSON_API_VERSION ? &api : NULL;
}
