/* Typed p-field storage. NaN payloads exist only in the legacy adapter. */
#include "pfield.h"
#include "insds_private.h"
#include "udo.h"

cs_float csoundPFieldStringCode(uint32_t index)
{
  cs_float value;
  /* Callers reject an unrepresentable ordinal before encoding it. */
#ifdef USE_DOUBLE
  uint64_t bits = UINT64_C(0x7ff8000000000000) | ((uint64_t)index << 32);
#else
  uint32_t bits = UINT32_C(0x7fc00000) | index;
#endif
  memcpy(&value, &bits, sizeof(value));
  return value;
}

int32_t csoundPFieldStringIndex(cs_float value, uint32_t *index)
{
#ifdef USE_DOUBLE
  uint64_t bits;
  memcpy(&bits, &value, sizeof(bits));
  if ((bits & UINT64_C(0x7fff0000ffffffff)) !=
      UINT64_C(0x7ff8000000000000)) return 0;
  *index = (uint32_t)((bits >> 32) & 0xffffU);
#else
  uint32_t bits;
  memcpy(&bits, &value, sizeof(bits));
  if ((bits & UINT32_C(0x7fff0000)) != UINT32_C(0x7fc00000)) return 0;
  *index = bits & 0xffffU;
#endif
  return 1;
}

static int32_t event_string_count(const EVTBLK *event)
{
  if (event == NULL || event->strarg == NULL) return 0;
  if (event->scnt < 0 || (uint32_t)event->scnt > CS_PFIELD_STRING_LIMIT) return -1;
  return event->scnt == 0 ? 1 : event->scnt;
}

size_t csoundEventStringBytes(const EVTBLK *event)
{
  int32_t count = event_string_count(event);
  const char *next;
  if (count <= 0) return 0;
  next = event->strarg;
  while (count--) next += strlen(next) + 1;
  return (size_t)(next - event->strarg);
}

const char *csoundEventStringArg(const EVTBLK *event, cs_float code)
{
  uint32_t index;
  int32_t count = event_string_count(event);
  const char *next;
  if (count <= 0 || !csoundPFieldStringIndex(code, &index) ||
      index >= (uint32_t)count) return NULL;
  next = event->strarg;
  while (index--) next += strlen(next) + 1;
  return next;
}

const char *csoundEventPFieldString(const EVTBLK *event, int32_t index)
{
  if (event == NULL || event->p == NULL || index < 1 || index > event->pcnt)
    return NULL;
  return csoundEventStringArg(event, event->p[index]);
}

CS_PFIELDS *csoundPFieldsCreate(CSOUND *csound, size_t count)
{
  CS_PFIELDS *set;
  if (count == 0 || count > INT32_MAX || count > SIZE_MAX / sizeof(CS_PFIELD))
    return NULL;
  set = csound->Calloc(csound, sizeof(*set));
  if (set == NULL) return NULL;
  set->fields = csound->Calloc(csound, count * sizeof(*set->fields));
  if (set->fields == NULL) {
    csound->Free(csound, set);
    return NULL;
  }
  set->count = count;
  return set;
}

void csoundPFieldsDestroy(CSOUND *csound, CS_PFIELDS *set)
{
  if (set == NULL) return;
  csound->Free(csound, set->fields);
  csound->Free(csound, set->offsets);
  csound->Free(csound, set->strings);
  csound->Free(csound, set);
}

int32_t csoundPFieldsSetString(CSOUND *csound, CS_PFIELDS *set, size_t field,
                               const char *text)
{
  size_t length, bytes, string_capacity, offset_capacity, *offsets;
  char *strings;
  if (set == NULL || field >= set->count || text == NULL ||
      set->string_count >= CS_PFIELD_STRING_LIMIT) return CSOUND_ERROR;
  length = strlen(text) + 1;
  if (length > SIZE_MAX - set->string_bytes) return CSOUND_MEMORY;
  bytes = set->string_bytes + length;
  string_capacity = set->string_capacity;
  offset_capacity = set->offset_capacity;
  strings = set->strings;
  offsets = set->offsets;
  /* Grow in batches. Keep old storage until text has been copied: it may
     refer to a string already in this set. */
  if (bytes > string_capacity) {
    string_capacity = bytes <= SIZE_MAX / 2 ? bytes * 2 : bytes;
    strings = csound->Malloc(csound, string_capacity);
    if (strings == NULL) return CSOUND_MEMORY;
    if (set->string_bytes != 0)
      memcpy(strings, set->strings, set->string_bytes);
  }
  if (set->string_count == offset_capacity) {
    offset_capacity = offset_capacity == 0 ? 8 : offset_capacity * 2;
    offsets = csound->Malloc(csound, offset_capacity * sizeof(*offsets));
    if (offsets == NULL) {
      if (strings != set->strings) csound->Free(csound, strings);
      return CSOUND_MEMORY;
    }
    if (set->string_count != 0)
      memcpy(offsets, set->offsets, set->string_count * sizeof(*offsets));
  }
  memcpy(strings + set->string_bytes, text, length);
  offsets[set->string_count] = set->string_bytes;
  if (strings != set->strings) csound->Free(csound, set->strings);
  if (offsets != set->offsets) csound->Free(csound, set->offsets);
  set->strings = strings;
  set->offsets = offsets;
  set->string_capacity = string_capacity;
  set->offset_capacity = offset_capacity;
  set->fields[field].kind = CS_PFIELD_STRING;
  set->fields[field].value.string_index = set->string_count++;
  set->string_bytes += length;
  return CSOUND_SUCCESS;
}

static const char *string_at(const CS_PFIELDS *set, uint32_t index)
{
  size_t offset, end;
  if (set == NULL || index >= set->string_count) return NULL;
  offset = set->offsets[index];
  end = index + 1 < set->string_count ? set->offsets[index + 1] : set->string_bytes;
  if (offset >= end || end > set->string_bytes ||
      memchr(set->strings + offset, '\0', end - offset) == NULL) return NULL;
  return set->strings + offset;
}

const char *csoundPFieldsString(const CS_PFIELDS *set, size_t field)
{
  if (set == NULL || field >= set->count ||
      set->fields[field].kind != CS_PFIELD_STRING) return NULL;
  return string_at(set, set->fields[field].value.string_index);
}

CS_PFIELDS *csoundPFieldsImport(CSOUND *csound, const EVTBLK *event)
{
  CS_PFIELDS *set;
  int32_t count = event_string_count(event);
  const char *next;
  size_t field;
  if (event == NULL || event->p == NULL || event->pcnt < 0 || count < 0)
    return NULL;
  set = csoundPFieldsCreate(csound, (size_t)event->pcnt + 1);
  if (set == NULL) return NULL;
  if (count != 0) {
    set->string_bytes = csoundEventStringBytes(event);
    set->strings = csound->Malloc(csound, set->string_bytes);
    set->offsets = csound->Malloc(csound, (size_t)count * sizeof(size_t));
    if (set->strings == NULL || set->offsets == NULL) goto error;
    memcpy(set->strings, event->strarg, set->string_bytes);
    next = set->strings;
    for (int32_t i = 0; i < count; ++i) {
      set->offsets[i] = (size_t)(next - set->strings);
      next += strlen(next) + 1;
    }
    set->string_count = count;
    set->string_capacity = set->string_bytes;
    set->offset_capacity = count;
  }
  /* p0 is not an event argument and some legacy callers leave it unset. */
  for (field = 1; field < set->count; ++field) {
    cs_float value = event->p[field];
    uint32_t index;
    if (count != 0 && IsStringCode(value)) {
      if (!csoundPFieldStringIndex(value, &index) || index >= (uint32_t)count)
        goto error;
      set->fields[field].kind = CS_PFIELD_STRING;
      set->fields[field].value.string_index = index;
    }
    else set->fields[field].value.number = value;
  }
  return set;
error:
  csoundPFieldsDestroy(csound, set);
  return NULL;
}

int32_t csoundPFieldsExport(CSOUND *csound, const CS_PFIELDS *set, EVTBLK *event)
{
  cs_float *values;
  char *strings = NULL;
  size_t field;
  if (set == NULL || event == NULL || event->p != NULL || event->strarg != NULL)
    return CSOUND_ERROR;
  for (field = 1; field < set->count; ++field) {
    if (set->fields[field].kind == CS_PFIELD_STRING) {
      if (csoundPFieldsString(set, field) == NULL) return CSOUND_ERROR;
    }
    else if (set->fields[field].kind != CS_PFIELD_NUMBER ||
             (set->string_count && IsStringCode(set->fields[field].value.number)))
      return CSOUND_ERROR;
  }
  values = csound->Calloc(csound, set->count * sizeof(*values));
  if (values == NULL) return CSOUND_MEMORY;
  if (set->string_bytes != 0) {
    strings = csound->Malloc(csound, set->string_bytes);
    if (strings == NULL) {
      csound->Free(csound, values);
      return CSOUND_MEMORY;
    }
    memcpy(strings, set->strings, set->string_bytes);
  }
  for (field = 1; field < set->count; ++field)
    values[field] = set->fields[field].kind == CS_PFIELD_STRING ?
      csoundPFieldStringCode(set->fields[field].value.string_index) :
      set->fields[field].value.number;
  event->p = values;
  event->pcnt = (int32_t)set->count - 1;
  event->strarg = strings;
  event->scnt = set->string_count;
  return CSOUND_SUCCESS;
}

void csoundPFieldsFreeEvent(CSOUND *csound, EVTBLK *event)
{
  csound->Free(csound, event->p);
  csound->Free(csound, event->strarg);
  event->p = NULL;
  event->strarg = NULL;
  event->scnt = 0;
}

void csoundResetInstancePFields(CSOUND *csound, INSDS *owner)
{
  if (owner->private_data == NULL) return;
  if (owner->private_data->pfields != NULL &&
      owner->strarg == owner->private_data->pfields->strings)
    owner->strarg = NULL;
  csoundPFieldsDestroy(csound, owner->private_data->pfields);
  owner->private_data->pfields = NULL;
}

void csoundFreeInstancePrivate(CSOUND *csound, INSDS *owner)
{
  csoundResetInstancePFields(csound, owner);
  csound->Free(csound, owner->private_data);
  owner->private_data = NULL;
}

int32_t csoundInstancePFieldsTake(CSOUND *csound, INSDS *owner, CS_PFIELDS *set)
{
  if (owner->private_data != NULL && owner->private_data->pfields == set)
    return CSOUND_SUCCESS;
  if (owner->private_data == NULL) {
    owner->private_data = csound->Calloc(csound, sizeof(*owner->private_data));
    if (owner->private_data == NULL) return CSOUND_MEMORY;
  }
  csoundResetInstancePFields(csound, owner);
  owner->private_data->pfields = set;
  owner->strarg = set != NULL ? set->strings : NULL;
  return CSOUND_SUCCESS;
}

int32_t csoundInstancePFields(CSOUND *csound, INSDS *owner, const EVTBLK *event)
{
  CS_PFIELDS *set;
  int32_t result;
  if (event->strarg == NULL) {
    csoundResetInstancePFields(csound, owner);
    owner->strarg = NULL;
    return CSOUND_SUCCESS;
  }
  set = csoundPFieldsImport(csound, event);
  if (set == NULL) return CSOUND_ERROR;
  result = csoundInstancePFieldsTake(csound, owner, set);
  if (result != CSOUND_SUCCESS) csoundPFieldsDestroy(csound, set);
  return result;
}

const char *csoundInstanceStringArg(const INSDS *owner, cs_float code)
{
  uint32_t index;
  if (!csoundPFieldStringIndex(code, &index)) return NULL;
  while (owner != NULL && owner->opcod_iobufs != NULL)
    owner = ((OPCOD_IOBUFS *)owner->opcod_iobufs)->parent_ip;
  if (owner == NULL || owner->private_data == NULL) return NULL;
  return string_at(owner->private_data->pfields, index);
}
