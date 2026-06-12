/*
  udo.c: user-defined opcodes and subinstruments

  Copyright (C) 2003-2025 Steven Yi, Victor Lazzarini, Istvan Varga,
                          Matt Ingalls

  This file is part of Csound.

  The Csound Library is free software; you can redistribute it
  and/or modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  Csound is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with Csound; if not, write to the Free Software
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#include "udo.h"
#include "Opcodes/biquad.h"
#include "csound_data_structures.h"
#include "csound_type_system.h"
#include "csound_standard_types.h"
#include "csound_orc_structs.h"
#include "namedins.h"
#include "cs_internal.h"

// count args in string types arg
static int32_t count_args(const char *args) {
  int32_t count = 0;
  while(*args != '\0') {
    if(*args != ':') {
      if(*args == '[') args+=2;
      else {
      args++;
      count++;
      }
    }
    else {
      while(*args != ';') {
        if(*args == '[') args+=2;
        else args++;
      }
      count++;
      args++;
    }
  }
  return count;
}

static inline void rewire_argpp(CSOUND *csound, OPDS *chain, int32_t index,
                                MYFLT *argPtr, const char *structPath);
static MYFLT *pbr_resolve_struct_target(CSOUND *csound, MYFLT *argPtr,
                                        const char *structPath);

typedef struct pbr_alias_entry {
  const char *name;
  int32_t ar_index;
} PBR_ALIAS_ENTRY;

typedef struct pbr_plan_builder {
  PBR_ALIAS_ENTRY *aliases;
  int32_t alias_count;
  int32_t alias_capacity;
  PBR_REWIRE_ENTRY *init_entries;
  int32_t init_count;
  int32_t init_capacity;
  PBR_REWIRE_ENTRY *perf_entries;
  int32_t perf_count;
  int32_t perf_capacity;
  PBR_SEED_ENTRY   *seed_entries;
  int32_t seed_count;
  int32_t seed_capacity;
} PBR_PLAN_BUILDER;

static size_t pbr_name_key_length(const char *var_name) {
  size_t len = 0;

  if (var_name == NULL) {
    return 0;
  }

  while (var_name[len] != '\0' && var_name[len] != '.' &&
         var_name[len] != ':' && var_name[len] != '@' &&
         var_name[len] != '[') {
    len++;
  }

  return len;
}

static int32_t pbr_names_match(const char *lhs, const char *rhs) {
  size_t lhs_len;
  size_t rhs_len;

  if (lhs == NULL || rhs == NULL) {
    return 0;
  }

  lhs_len = pbr_name_key_length(lhs);
  rhs_len = pbr_name_key_length(rhs);

  return lhs_len == rhs_len && strncmp(lhs, rhs, lhs_len) == 0;
}

static const char *pbr_get_arg_name(const ARG *arg, const char *raw_name) {
  if (arg != NULL &&
      (arg->type == ARG_LOCAL || arg->type == ARG_GLOBAL) &&
      arg->argPtr != NULL) {
    const CS_VARIABLE *var = (const CS_VARIABLE *)arg->argPtr;
    if (var->varName != NULL) {
      return var->varName;
    }
  }

  return raw_name;
}

static const char *pbr_get_struct_path(const ARG *arg,
                                       const char *raw_name,
                                       size_t *path_len) {
  const char *struct_path = arg != NULL ? arg->structPath : NULL;
  size_t len = 0;

  if ((struct_path == NULL || *struct_path == '\0') && raw_name != NULL) {
    const char *dot = strchr(raw_name, '.');
    if (dot != NULL && dot[1] != '\0') {
      struct_path = dot + 1;
    }
  }

  if (struct_path == NULL || *struct_path == '\0') {
    if (path_len != NULL) {
      *path_len = 0;
    }
    return NULL;
  }

  while (struct_path[len] != '\0' && struct_path[len] != ':' &&
         struct_path[len] != '@' && struct_path[len] != '[') {
    len++;
  }

  if (path_len != NULL) {
    *path_len = len;
  }

  return len > 0 ? struct_path : NULL;
}

static char *pbr_dup_struct_path(CSOUND *csound,
                                 const ARG *arg,
                                 const char *raw_name) {
  size_t path_len = 0;
  const char *struct_path = pbr_get_struct_path(arg, raw_name, &path_len);

  return struct_path != NULL ? cs_strndup(csound, struct_path, path_len) : NULL;
}

static int32_t pbr_arg_has_struct_path(const ARG *arg, const char *raw_name) {
  size_t path_len = 0;
  return pbr_get_struct_path(arg, raw_name, &path_len) != NULL && path_len > 0;
}

static int32_t pbr_struct_paths_match(const char *stored_path,
                                      const ARG *arg,
                                      const char *raw_name) {
  size_t path_len = 0;
  const char *struct_path = pbr_get_struct_path(arg, raw_name, &path_len);

  if (struct_path == NULL) {
    return stored_path == NULL || *stored_path == '\0';
  }

  if (stored_path == NULL) {
    return 0;
  }

  return strlen(stored_path) == path_len &&
    strncmp(stored_path, struct_path, path_len) == 0;
}

static int32_t pbr_alias_find(const PBR_ALIAS_ENTRY *aliases,
                              int32_t alias_count,
                              const char *name) {
  int32_t i;

  if (name == NULL) {
    return -1;
  }

  for (i = 0; i < alias_count; i++) {
    if (strcmp(aliases[i].name, name) == 0) {
      return i;
    }
  }

  return -1;
}

static int32_t pbr_alias_lookup_exact(const PBR_ALIAS_ENTRY *aliases,
                                      int32_t alias_count,
                                      const char *var_name) {
  int32_t i;

  if (var_name == NULL) {
    return -1;
  }

  for (i = 0; i < alias_count; i++) {
    if (strcmp(aliases[i].name, var_name) == 0) {
      return aliases[i].ar_index;
    }
  }

  return -1;
}

static int32_t pbr_alias_lookup_base(const PBR_ALIAS_ENTRY *aliases,
                                     int32_t alias_count,
                                     const char *var_name) {
  int32_t i;

  if (var_name == NULL) {
    return -1;
  }

  for (i = 0; i < alias_count; i++) {
    if (pbr_names_match(aliases[i].name, var_name)) {
      return aliases[i].ar_index;
    }
  }

  return -1;
}


static int32_t pbr_lookup_arg_alias(const PBR_PLAN_BUILDER *builder,
                                    const char *arg_name,
                                    const char *raw_name) {
  int32_t ar_index = pbr_alias_lookup_exact(builder->aliases,
                                            builder->alias_count,
                                            raw_name);
  if (ar_index >= 0) {
    return ar_index;
  }

  if (arg_name != raw_name) {
    ar_index = pbr_alias_lookup_exact(builder->aliases,
                                      builder->alias_count,
                                      arg_name);
    if (ar_index >= 0) {
      return ar_index;
    }
  }

  ar_index = pbr_alias_lookup_base(builder->aliases,
                                   builder->alias_count,
                                   raw_name);
  if (ar_index >= 0) {
    return ar_index;
  }

  return arg_name != raw_name ?
    pbr_alias_lookup_base(builder->aliases, builder->alias_count, arg_name) :
    -1;
}

static int32_t pbr_ensure_alias_capacity(CSOUND *csound,
                                         PBR_PLAN_BUILDER *builder,
                                         int32_t needed) {
  if (needed > builder->alias_capacity) {
    int32_t new_capacity = builder->alias_capacity > 0 ?
      builder->alias_capacity * 2 : 8;
    PBR_ALIAS_ENTRY *grown;

    while (new_capacity < needed) {
      new_capacity *= 2;
    }

    grown = builder->aliases == NULL ?
      (PBR_ALIAS_ENTRY *)csound->Malloc(csound,
                                        (size_t)new_capacity *
                                        sizeof(PBR_ALIAS_ENTRY)) :
      (PBR_ALIAS_ENTRY *)csound->ReAlloc(csound, builder->aliases,
                                         (size_t)new_capacity *
                                         sizeof(PBR_ALIAS_ENTRY));
    if (grown == NULL) {
      return NOTOK;
    }

    builder->aliases = grown;
    builder->alias_capacity = new_capacity;
  }

  return OK;
}

static int32_t pbr_add_alias(CSOUND *csound,
                             PBR_PLAN_BUILDER *builder,
                             const char *name,
                             int32_t ar_index) {
  int32_t slot;

  if (name == NULL) {
    return OK;
  }

  slot = pbr_alias_find(builder->aliases, builder->alias_count, name);
  if (slot >= 0) {
    builder->aliases[slot].ar_index = ar_index;
    return OK;
  }

  if (pbr_ensure_alias_capacity(csound, builder, builder->alias_count + 1) != OK) {
    return NOTOK;
  }

  builder->aliases[builder->alias_count].name = name;
  builder->aliases[builder->alias_count].ar_index = ar_index;
  builder->alias_count++;

  return OK;
}

static int32_t pbr_ensure_entry_capacity(CSOUND *csound,
                                         PBR_REWIRE_ENTRY **entries,
                                         int32_t *capacity,
                                         int32_t needed) {
  if (needed > *capacity) {
    int32_t new_capacity = *capacity > 0 ? *capacity * 2 : 16;
    PBR_REWIRE_ENTRY *grown;

    while (new_capacity < needed) {
      new_capacity *= 2;
    }

    grown = *entries == NULL ?
      (PBR_REWIRE_ENTRY *)csound->Malloc(csound,
                                         (size_t)new_capacity *
                                         sizeof(PBR_REWIRE_ENTRY)) :
      (PBR_REWIRE_ENTRY *)csound->ReAlloc(csound, *entries,
                                          (size_t)new_capacity *
                                          sizeof(PBR_REWIRE_ENTRY));
    if (grown == NULL) {
      return NOTOK;
    }

    *entries = grown;
    *capacity = new_capacity;
  }

  return OK;
}

static int32_t pbr_append_entry(CSOUND *csound,
                                PBR_REWIRE_ENTRY **entries,
                                int32_t *count,
                                int32_t *capacity,
                                size_t opcode_mem_offset,
                                int32_t arg_index,
                                int32_t ar_index,
                                const ARG *arg,
                                const char *raw_name) {
  PBR_REWIRE_ENTRY *entry;

  if (pbr_ensure_entry_capacity(csound, entries, capacity, *count + 1) != OK) {
    return NOTOK;
  }

  entry = &((*entries)[*count]);
  entry->opcode_mem_offset = opcode_mem_offset;
  entry->arg_index = arg_index;
  entry->ar_index = ar_index;
  entry->structPath = pbr_dup_struct_path(csound, arg, raw_name);
  (*count)++;

  return OK;
}

static int32_t pbr_append_seed(CSOUND *csound,
                               PBR_SEED_ENTRY **entries,
                               int32_t *count,
                               int32_t *capacity,
                               int32_t output_ar_index,
                               int32_t input_ar_index,
                               int32_t work_ar_index,
                               const ARG *input_arg,
                               const char *input_raw_name) {
  PBR_SEED_ENTRY *entry;

  if (*count >= *capacity) {
    int32_t new_capacity = *capacity > 0 ? *capacity * 2 : 8;
    PBR_SEED_ENTRY *grown;
    while (new_capacity <= *count) {
      new_capacity *= 2;
    }
    grown = *entries == NULL ?
      (PBR_SEED_ENTRY *)csound->Malloc(csound,
                                       (size_t)new_capacity *
                                       sizeof(PBR_SEED_ENTRY)) :
      (PBR_SEED_ENTRY *)csound->ReAlloc(csound, *entries,
                                        (size_t)new_capacity *
                                        sizeof(PBR_SEED_ENTRY));
    if (grown == NULL) {
      return NOTOK;
    }
    *entries = grown;
    *capacity = new_capacity;
  }

  entry = &((*entries)[*count]);
  entry->output_ar_index = output_ar_index;
  entry->input_ar_index = input_ar_index;
  entry->work_ar_index = work_ar_index;
  entry->input_struct_path = pbr_dup_struct_path(csound, input_arg,
                                                 input_raw_name);
  (*count)++;

  return OK;
}

static int32_t pbr_find_seed_work_output(const PBR_SEED_ENTRY *entries,
                                         int32_t entry_count,
                                         int32_t input_ar_index,
                                         const ARG *input_arg,
                                         const char *input_raw_name) {
  int32_t i;

  for (i = 0; i < entry_count; i++) {
    const PBR_SEED_ENTRY *entry = &entries[i];
    if (entry->input_ar_index == input_ar_index &&
        pbr_struct_paths_match(entry->input_struct_path,
                               input_arg, input_raw_name)) {
      return entry->work_ar_index;
    }
  }

  return -1;
}

static int32_t pbr_can_rewire_xout_source(const char *raw_name) {
  if (raw_name == NULL) {
    return 0;
  }

  return strchr(raw_name, '[') == NULL && strchr(raw_name, '@') == NULL;
}

/* xout processing rewrites aliases to output slots.  Query the original xin
   declaration so duplicate pass-through detection is not affected by earlier
   xout entries mutating the alias table. */
static int32_t pbr_lookup_xin_alias(OPCODINFO *udoinfo,
                                    OPTXT *xin_optxt,
                                    const char *var_name) {
  int32_t exact_pass;

  if (udoinfo == NULL || xin_optxt == NULL || udoinfo->in_arg_pool == NULL ||
      xin_optxt->t.outlist == NULL || var_name == NULL) {
    return -1;
  }

  for (exact_pass = 1; exact_pass >= 0; exact_pass--) {
    CS_VARIABLE *param = udoinfo->in_arg_pool->head;
    ARG *xin_arg = xin_optxt->t.outArgs;
    ARGLST *xin_outlist = xin_optxt->t.outlist;
    int32_t i;

    for (i = 0; i < udoinfo->inchns && i < xin_outlist->count && param != NULL;
         i++, param = param->next,
         xin_arg = xin_arg != NULL ? xin_arg->next : NULL) {
      int32_t ar_index = udoinfo->outchns + i;
      const char *xin_varname = xin_outlist->arg[i];
      const char *resolved_name = pbr_get_arg_name(xin_arg, xin_varname);
      const char *names[3] = { xin_varname, resolved_name, param->varName };
      int32_t j;

      for (j = 0; j < 3; j++) {
        const char *candidate = names[j];
        if (candidate == NULL) {
          continue;
        }
        if (exact_pass ? strcmp(candidate, var_name) == 0 :
            pbr_names_match(candidate, var_name)) {
          return ar_index;
        }
      }
    }
  }

  return -1;
}

static void pbr_collect_io_aliases(CSOUND *csound,
                                   OPCODINFO *udoinfo,
                                   OPTXT *xin_optxt,
                                   OPTXT *xout_optxt,
                                   size_t xout_opcode_mem_offset,
                                   PBR_PLAN_BUILDER *builder) {
  IGN(xout_opcode_mem_offset);

  if (udoinfo == NULL) {
    return;
  }

  if (xin_optxt != NULL && udoinfo->in_arg_pool != NULL &&
      xin_optxt->t.outlist != NULL) {
    CS_VARIABLE *param = udoinfo->in_arg_pool->head;
    ARG *xin_arg = xin_optxt->t.outArgs;
    ARGLST *xin_outlist = xin_optxt->t.outlist;
    int32_t i;

    for (i = 0; i < udoinfo->inchns && i < xin_outlist->count && param != NULL;
         i++, param = param->next, xin_arg = xin_arg != NULL ? xin_arg->next : NULL) {
      int32_t ar_index = udoinfo->outchns + i;
      const char *xin_varname = xin_outlist->arg[i];
      const char *resolved_name = pbr_get_arg_name(xin_arg, xin_varname);

      pbr_add_alias(csound, builder, xin_varname, ar_index);
      if (resolved_name != xin_varname) {
        pbr_add_alias(csound, builder, resolved_name, ar_index);
      }
      if (param->varName != NULL && xin_varname != NULL &&
          strcmp(param->varName, xin_varname) != 0) {
        pbr_add_alias(csound, builder, param->varName, ar_index);
      }
    }
  }

  if (xout_optxt != NULL && xout_optxt->t.inlist != NULL) {
    ARG *xout_arg = xout_optxt->t.inArgs;
    ARGLST *xout_inlist = xout_optxt->t.inlist;
    int32_t i;

    for (i = 0; i < udoinfo->outchns && i < xout_inlist->count;
         i++, xout_arg = xout_arg != NULL ? xout_arg->next : NULL) {
      const char *raw_name = xout_inlist->arg[i];
      const char *resolved_name = pbr_get_arg_name(xout_arg, raw_name);
      int32_t source_ar_index = -1;

      if (pbr_can_rewire_xout_source(raw_name)) {
        source_ar_index = pbr_lookup_xin_alias(udoinfo, xin_optxt,
                                               resolved_name);
        if (source_ar_index < 0 && resolved_name != raw_name) {
          source_ar_index = pbr_lookup_xin_alias(udoinfo, xin_optxt,
                                                 raw_name);
        }
      }

      if (source_ar_index >= (int32_t)udoinfo->outchns) {
        int32_t work_ar_index =
          pbr_find_seed_work_output(builder->seed_entries, builder->seed_count,
                                    source_ar_index, xout_arg, raw_name);
        /* Pass-through: xout reads from a xin-aliased variable.
           Redirect the alias to the output slot so internal opcodes write to
           the caller's output pointer (like a native opcode's output field),
           and record a seed entry so the output is initialised from the input
           value before the internal chain runs.  Duplicate pass-through xouts
           share the first output slot as their mutable work value. */
        if (work_ar_index < 0) {
          work_ar_index = i;
          if (pbr_arg_has_struct_path(xout_arg, raw_name)) {
            pbr_add_alias(csound, builder, raw_name, i);
            if (resolved_name != raw_name && strchr(resolved_name, '.') != NULL) {
              pbr_add_alias(csound, builder, resolved_name, i);
            }
          } else {
            pbr_add_alias(csound, builder, resolved_name, i);
            if (resolved_name != raw_name) {
              pbr_add_alias(csound, builder, raw_name, i);
            }
          }
        }
        pbr_append_seed(csound, &builder->seed_entries, &builder->seed_count,
                        &builder->seed_capacity, i, source_ar_index,
                        work_ar_index, xout_arg, raw_name);
        continue;
      }

      pbr_add_alias(csound, builder, raw_name, i);
      pbr_add_alias(csound, builder, resolved_name, i);
    }
  }
}

static void pbr_collect_rewire_entries(CSOUND *csound,
                                       OPCODINFO *udoinfo,
                                       PBR_PLAN_BUILDER *builder) {
  OPTXT *optxt = (OPTXT *)udoinfo->ip;
  size_t opcode_mem_offset = 0;

  while ((optxt = optxt->nxtop) != NULL) {
    TEXT *ttp = &optxt->t;
    OENTRY *ep = ttp->oentry;
    const char *opname;
    size_t current_offset = opcode_mem_offset;

    if (ep == NULL || ep->opname == NULL) {
      continue;
    }

    opname = ttp->opcod != NULL ? ttp->opcod : ep->opname;
    opcode_mem_offset += ep->dsblksiz;

    if (strcmp(ep->opname, "endin") == 0 || strcmp(ep->opname, "endop") == 0) {
      break;
    }

    if (strcmp(opname, "xin") == 0) {
      continue;
    }

    if (ttp->outlist != NULL) {
      ARG *arg = ttp->outArgs;
      int32_t i;

      for (i = 0; i < ttp->outlist->count;
           i++, arg = arg != NULL ? arg->next : NULL) {
        const char *arg_name = pbr_get_arg_name(arg, ttp->outlist->arg[i]);
        int32_t ar_index = pbr_lookup_arg_alias(builder, arg_name,
                                                ttp->outlist->arg[i]);
        if (ar_index < 0) {
          continue;
        }

        if (ep->init != NULL) {
          pbr_append_entry(csound, &builder->init_entries, &builder->init_count,
                           &builder->init_capacity, current_offset, i,
                           ar_index, arg, ttp->outlist->arg[i]);
        }
        if (ep->perf != NULL) {
          pbr_append_entry(csound, &builder->perf_entries, &builder->perf_count,
                           &builder->perf_capacity, current_offset, i,
                           ar_index, arg, ttp->outlist->arg[i]);
        }
      }
    }

    if (ttp->inlist != NULL) {
      ARG *arg = ttp->inArgs;
      int32_t actual_outcount = count_args(ep->outypes);
      int32_t i;

      for (i = 0; i < ttp->inlist->count;
           i++, arg = arg != NULL ? arg->next : NULL) {
        const char *arg_name = pbr_get_arg_name(arg, ttp->inlist->arg[i]);
        int32_t ar_index = pbr_lookup_arg_alias(builder, arg_name,
                                                ttp->inlist->arg[i]);
        if (ar_index < 0) {
          continue;
        }

        if (ep->init != NULL) {
          pbr_append_entry(csound, &builder->init_entries, &builder->init_count,
                           &builder->init_capacity, current_offset,
                           actual_outcount + i, ar_index,
                           arg, ttp->inlist->arg[i]);
        }
        if (ep->perf != NULL) {
          pbr_append_entry(csound, &builder->perf_entries, &builder->perf_count,
                           &builder->perf_capacity, current_offset,
                           actual_outcount + i, ar_index,
                           arg, ttp->inlist->arg[i]);
        }
      }
    }
  }
}

static void pbr_free_entries(CSOUND *csound,
                             PBR_REWIRE_ENTRY *entries,
                             int32_t entry_count) {
  int32_t i;

  if (entries == NULL) {
    return;
  }

  for (i = 0; i < entry_count; i++) {
    csound->Free(csound, entries[i].structPath);
  }

  csound->Free(csound, entries);
}

static void pbr_free_seed_entries(CSOUND *csound,
                                  PBR_SEED_ENTRY *entries,
                                  int32_t entry_count) {
  int32_t i;

  if (entries == NULL) {
    return;
  }

  for (i = 0; i < entry_count; i++) {
    csound->Free(csound, entries[i].input_struct_path);
  }

  csound->Free(csound, entries);
}

static void pbr_free_plan(CSOUND *csound, PBR_REWIRE_PLAN *plan) {
  if (plan == NULL) {
    return;
  }

  pbr_free_entries(csound, plan->init_entries, plan->init_count);
  pbr_free_entries(csound, plan->perf_entries, plan->perf_count);
  pbr_free_seed_entries(csound, plan->seed_entries, plan->seed_count);
  csound->Free(csound, plan);
}

static void pbr_free_var_pool(CSOUND *csound, CS_VAR_POOL *pool) {
  CS_VARIABLE *var;

  if (pool == NULL) {
    return;
  }

  for (var = pool->head; var != NULL; var = var->next) {
    csound->Free(csound, var->varName);
  }

  csoundFreeVarPool(csound, pool);
}

static char *pbr_get_opcode_mem_start(INSDS *lcurip) {
  INSTRTXT *tp;

  if (lcurip == NULL || lcurip->instr == NULL || lcurip->lclbas == NULL) {
    return NULL;
  }

  tp = lcurip->instr;
  return (char *)lcurip->lclbas + tp->varPool->poolSize +
    (tp->varPool->varCount * CS_FLOAT_ALIGN(CS_VAR_TYPE_OFFSET));
}

static void pbr_apply_entries(CSOUND *csound,
                              UOPCODE *p,
                              OPDS *chain,
                              char *op_mem_start,
                              const PBR_REWIRE_ENTRY *entries,
                              int32_t entry_count) {
  int32_t max_ar_index;
  int32_t i;

  IGN(chain);

  if (p == NULL || p->buf == NULL || p->buf->opcode_info == NULL ||
      op_mem_start == NULL) {
    return;
  }

  max_ar_index = p->buf->opcode_info->outchns + p->buf->opcode_info->inchns;

  for (i = 0; i < entry_count; i++) {
    const PBR_REWIRE_ENTRY *entry = &entries[i];

    if (entry->ar_index < 0 || entry->ar_index >= max_ar_index) {
      continue;
    }

    rewire_argpp(csound,
                 (OPDS *)(op_mem_start + entry->opcode_mem_offset),
                 entry->arg_index,
                 p->ar[entry->ar_index],
                 entry->structPath);
  }
}

static void pbr_copy_value(CSOUND *csound, MYFLT *dst, MYFLT *src, INSDS *ctx) {
  CS_TYPE *dst_type;
  CS_TYPE *src_type;

  if (dst == NULL || src == NULL || dst == src) {
    return;
  }

  dst_type = csoundGetTypeForArg(dst);
  src_type = csoundGetTypeForArg(src);
  if (dst_type != NULL && dst_type->copyValue != NULL) {
    dst_type->copyValue(csound, dst_type, dst, src, ctx);
  } else if (src_type != NULL && src_type->copyValue != NULL) {
    src_type->copyValue(csound, src_type, dst, src, ctx);
  } else {
    *dst = *src;
  }
}

static int32_t pbr_is_readonly_source(MYFLT *src) {
  CS_TYPE *src_type = src != NULL ? csoundGetTypeForArg(src) : NULL;
  return src_type == &CS_VAR_TYPE_C || src_type == &CS_VAR_TYPE_P;
}

static void pbr_seed_pass_through_outputs(CSOUND *csound,
                                          UOPCODE *p,
                                          INSDS *ctx) {
  OPCODINFO *udoinfo;
  PBR_REWIRE_PLAN *plan;
  int32_t max_ar_index;
  int32_t i;

  if (p == NULL || p->buf == NULL || p->buf->opcode_info == NULL) {
    return;
  }

  udoinfo = p->buf->opcode_info;
  plan = udoinfo->pbr_plan;
  if (plan == NULL || plan->seed_entries == NULL) {
    return;
  }

  max_ar_index = udoinfo->outchns + udoinfo->inchns;
  for (i = 0; i < plan->seed_count; i++) {
    const PBR_SEED_ENTRY *entry = &plan->seed_entries[i];
    MYFLT *dst;
    MYFLT *src;

    if (entry->output_ar_index < 0 || entry->output_ar_index >= max_ar_index ||
        entry->input_ar_index < 0 || entry->input_ar_index >= max_ar_index) {
      continue;
    }

    dst = p->ar[entry->output_ar_index];
    src = pbr_resolve_struct_target(csound, p->ar[entry->input_ar_index],
                                    entry->input_struct_path);
    pbr_copy_value(csound, dst, src, ctx);
  }
}

static void pbr_sync_pass_through_outputs(CSOUND *csound,
                                          UOPCODE *p,
                                          INSDS *ctx) {
  OPCODINFO *udoinfo;
  PBR_REWIRE_PLAN *plan;
  int32_t max_ar_index;
  int32_t i;

  if (p == NULL || p->buf == NULL || p->buf->opcode_info == NULL) {
    return;
  }

  udoinfo = p->buf->opcode_info;
  plan = udoinfo->pbr_plan;
  if (plan == NULL || plan->seed_entries == NULL) {
    return;
  }

  max_ar_index = udoinfo->outchns + udoinfo->inchns;
  for (i = 0; i < plan->seed_count; i++) {
    const PBR_SEED_ENTRY *entry = &plan->seed_entries[i];
    MYFLT *dst;
    MYFLT *src;

    /* Fan the final work value out to duplicate xout positions. */
    if (entry->output_ar_index < 0 || entry->output_ar_index >= max_ar_index ||
        entry->work_ar_index < 0 || entry->work_ar_index >= max_ar_index ||
        entry->output_ar_index == entry->work_ar_index) {
      continue;
    }

    dst = p->ar[entry->output_ar_index];
    src = p->ar[entry->work_ar_index];
    pbr_copy_value(csound, dst, src, ctx);
  }
}

static void pbr_writeback_pass_through_inputs(CSOUND *csound,
                                              UOPCODE *p,
                                              INSDS *ctx) {
  OPCODINFO *udoinfo;
  PBR_REWIRE_PLAN *plan;
  int32_t max_ar_index;
  int32_t i;

  if (p == NULL || p->buf == NULL || p->buf->opcode_info == NULL) {
    return;
  }

  udoinfo = p->buf->opcode_info;
  plan = udoinfo->pbr_plan;
  if (plan == NULL || plan->seed_entries == NULL) {
    return;
  }

  max_ar_index = udoinfo->outchns + udoinfo->inchns;
  for (i = 0; i < plan->seed_count; i++) {
    const PBR_SEED_ENTRY *entry = &plan->seed_entries[i];
    MYFLT *dst;
    MYFLT *src;
    MYFLT *input_base;

    if (entry->output_ar_index < 0 || entry->output_ar_index >= max_ar_index ||
        entry->input_ar_index < 0 || entry->input_ar_index >= max_ar_index ||
        entry->work_ar_index < 0 || entry->work_ar_index >= max_ar_index) {
      continue;
    }

    input_base = p->ar[entry->input_ar_index];
    if (pbr_is_readonly_source(input_base)) {
      continue;
    }

    /* Skip writeback when the caller's input variable is also bound to one
       of the UDO's output slots.  In that case the output was already
       materialised by xout/sync and writing the work value back would
       overwrite it.  This covers the #2061 scenario where the input is
       aliased to a non-pass-through output position. */
    {
      int32_t j;
      int32_t skip = 0;
      for (j = 0; j < udoinfo->outchns; j++) {
        if (j == entry->work_ar_index) {
          /* The work output slot is where the pass-through value is
             redirected; that alias is intentional, not a collision. */
          continue;
        }
        if (p->ar[j] == input_base) {
          skip = 1;
          break;
        }
      }
      if (skip) {
        continue;
      }
    }

    dst = pbr_resolve_struct_target(csound, input_base,
                                    entry->input_struct_path);
    src = p->ar[entry->work_ar_index];
    pbr_copy_value(csound, dst, src, ctx);
  }
}

void csoundBuildUserOpcodeRewirePlan(CSOUND *csound, OPCODINFO *udoinfo) {
  PBR_PLAN_BUILDER builder = {0};
  PBR_REWIRE_PLAN *plan;
  OPTXT *optxt;
  OPTXT *xin_optxt = NULL;
  OPTXT *xout_optxt = NULL;
  size_t opcode_mem_offset = 0;
  size_t xout_opcode_mem_offset = 0;

  if (udoinfo == NULL) {
    return;
  }

  pbr_free_plan(csound, udoinfo->pbr_plan);
  udoinfo->pbr_plan = NULL;

  plan = (PBR_REWIRE_PLAN *)csound->Calloc(csound, sizeof(PBR_REWIRE_PLAN));
  if (plan == NULL) {
    return;
  }

  udoinfo->pbr_plan = plan;

  if (!udoinfo->newStyle || udoinfo->ip == NULL) {
    return;
  }

  optxt = (OPTXT *)udoinfo->ip;
  while ((optxt = optxt->nxtop) != NULL) {
    OENTRY *ep = optxt->t.oentry;
    size_t current_offset = opcode_mem_offset;

    if (ep != NULL) {
      opcode_mem_offset += ep->dsblksiz;
    }

    if (optxt->t.opcod == NULL) {
      continue;
    }

    if (strcmp(optxt->t.opcod, "xin") == 0) {
      xin_optxt = optxt;
    } else if (strcmp(optxt->t.opcod, "xout") == 0) {
      xout_optxt = optxt;
      xout_opcode_mem_offset = current_offset;
    }
  }

  pbr_collect_io_aliases(csound, udoinfo, xin_optxt, xout_optxt,
                         xout_opcode_mem_offset, &builder);
  pbr_collect_rewire_entries(csound, udoinfo, &builder);

  plan->init_count = builder.init_count;
  plan->perf_count = builder.perf_count;
  plan->seed_count = builder.seed_count;
  plan->init_entries = builder.init_entries;
  plan->perf_entries = builder.perf_entries;
  plan->seed_entries = builder.seed_entries;

  csound->Free(csound, builder.aliases);
}

void csoundFreeOpcodeInfoChain(CSOUND *csound) {
  OPCODINFO *current = csound->opcodeInfo;

  while (current != NULL) {
    OPCODINFO *next = current->prv;

    pbr_free_plan(csound, current->pbr_plan);
    pbr_free_var_pool(csound, current->in_arg_pool);
    pbr_free_var_pool(csound, current->out_arg_pool);
    csound->Free(csound, current->name);
    csound->Free(csound, current->intypes);
    csound->Free(csound, current->outtypes);
    csound->Free(csound, current);
    current = next;
  }

  csound->opcodeInfo = NULL;
  csound->engineState.opcodeInfo = NULL;
}

/* Helper to rewire an opcode argument pointer to pass-by-ref location */
static inline void rewire_argpp(CSOUND *csound, OPDS *chain, int32_t index,
                                MYFLT *argPtr, const char *structPath) {
  OENTRY *ep = chain->optext->t.oentry;
  MYFLT *target_ptr = pbr_resolve_struct_target(csound, argPtr, structPath);

  // The opcode structure consists of OPDS header followed by argument pointer fields.
  // We need to update the structure field at the given index.
  // Structure layout: OPDS | ptr0 | ptr1 | ptr2 | ...
  // So field at index i is at offset: sizeof(OPDS) + i * sizeof(void*)

  if (ep->useropinfo == NULL) {
    // Regular opcode - update the structure field directly
    void **fieldPtr = (void**)((char*)chain + sizeof(OPDS) + index * sizeof(void*));
    *fieldPtr = (void*)target_ptr;
  } else {
    // UDO - use the ar array
    UOPCODE *uop = (UOPCODE*)chain;
    OPCODINFO *useropinfo = (OPCODINFO*)ep->useropinfo;

    // Bounds check: compute max = outchns + inchns
    int32_t max = useropinfo->outchns + useropinfo->inchns;

    // Validate index is within bounds and non-negative
    if (index < 0 || index >= max) {
      csound->Message(csound, Str("Error: UDO argument index %d out of bounds "
                      "(max: %d, outchns: %d, inchns: %d)\n"),
                      index, max, useropinfo->outchns, useropinfo->inchns);
      return; // Bail out instead of writing to prevent overflow
    }

    uop->ar[index] = target_ptr;
  }
}

static MYFLT *pbr_resolve_struct_target(CSOUND *csound, MYFLT *argPtr,
                                        const char *structPath) {
  MYFLT *target_ptr = argPtr;

  IGN(csound);

  if (structPath != NULL && *structPath != '\0') {
    char pathBuf[256];
    size_t pathLen = strlen(structPath);
    if (pathLen >= sizeof(pathBuf)) {
      pathLen = sizeof(pathBuf) - 1;
    }
    memcpy(pathBuf, structPath, pathLen);
    pathBuf[pathLen] = '\0';

    const char *p = pathBuf;
    char memberName[64];

    while (*p != '\0' && target_ptr != NULL) {
      size_t nameLen = 0;
      while (*p != '\0' && *p != '.' && nameLen < sizeof(memberName) - 1) {
        memberName[nameLen++] = *p++;
      }
      memberName[nameLen] = '\0';

      if (nameLen == 0) {
        break;
      }

      CS_TYPE *type = csoundGetTypeForArg(target_ptr);
      CS_STRUCT_VAR *structVar = (CS_STRUCT_VAR*)target_ptr;

      if (type == NULL || structVar == NULL || structVar->members == NULL) {
        break;
      }

      CONS_CELL *members = type->members;
      int member_index = 0;
      int found = 0;
      while (members != NULL) {
        CS_VARIABLE *member = (CS_VARIABLE*)members->value;
        if (!strcmp(member->varName, memberName)) {
          target_ptr = &(structVar->members[member_index]->value);
          found = 1;
          break;
        }
        member_index++;
        members = members->next;
      }

      if (!found) {
        break;
      }

      if (*p == '.') {
        p++;
      }
    }
  }

  return target_ptr;
}

/* Wire UDO internals to caller storage for pass-by-reference.
 *
 * Internal opcodes that reference xin/xout-aliased variables are rewired to
 * point directly at the caller's argument storage, like native opcode output
 * pointers.  For xin->xout pass-through variables, the alias is directed to
 * the caller's output slot and the output is seeded with the input value so
 * that internal operations start from the correct initial value.
 */
static void handle_pass_by_ref(CSOUND* csound, UOPCODE* p, INSDS* lcurip) {
  OPCODINFO *udoinfo = (OPCODINFO*) p->h.optext->t.oentry->useropinfo;
  PBR_REWIRE_PLAN *plan = udoinfo != NULL ? udoinfo->pbr_plan : NULL;
  char *op_mem_start = pbr_get_opcode_mem_start(lcurip);

  if (plan == NULL || op_mem_start == NULL) {
    return;
  }

  /* Rewire internal opcodes to point at caller storage.  For pass-through
     variables (xin->xout same name), the alias now points at the output slot
     so internal writes go to the caller's output variable. */
  pbr_apply_entries(csound, p, lcurip->nxti, op_mem_start,
                    plan->init_entries, plan->init_count);
  pbr_apply_entries(csound, p, lcurip->nxtp, op_mem_start,
                    plan->perf_entries, plan->perf_count);

  /* Seed pass-through outputs before the internal chain runs.  Constants and
     other immutable inputs stay protected because writes happen to the output
     scratch slot first; mutable inputs are written back after the chain. */
  pbr_seed_pass_through_outputs(csound, p, lcurip);
}

/*
  UDOs now use the local ksmps/local sr stored in lcurip, and
  all the other dependent parameters are calculated in relation to
  this.

  lcurip->ksmps is set to the caller ksmps (CS_KSMPS), unless a new
  local ksmps is used, in which case it is set to that value.
  Local ksmps can only be set by setksmps.
  If local ksmps differs from CS_KSMPS, we set useropcd1() to
  deal with the perf-time code. Otherwise useropcd2() is used.

  For recursive calls when the local ksmps is set to differ from
  the calling instrument ksmps, the top-level call
  will use useropcd1(), whereas all the other recursive calls
  will use useropdc2(), since their local ksmps will be the same
  as the caller.

  Also in case of a local ksmps that differs from the caller,
  the local kcounter value, obtained from the caller is
  scaled to denote the correct kcount in terms of local
  kcycles.

  Similarly, a local SR is now implemented. This is set by
  the oversample/undersample opcode. It is not allowed with
  local ksmps setting (setksmps) or with audio/k-rate array
  arguments. It uses useropcd2().

*/

static OPCODINFO *find_latest_useropinfo(CSOUND *csound, const char *name,
                                         const char *outtypes,
                                         const char *intypes) {
  OPCODINFO *opinfo = csound->opcodeInfo;
  while (opinfo != NULL) {
    if (strcmp(opinfo->name, name) == 0 &&
        strcmp(opinfo->outtypes, outtypes) == 0 &&
        inargs_match(opinfo->intypes, intypes) == 0) {
      return opinfo;
    }
    opinfo = opinfo->prv;
  }
  return NULL;
}

int32_t useropcdset(CSOUND *csound, UOPCODE *p)
{
  OPDS         *saved_ids = csound->ids;
  INSDS        *parent_ip = csound->curip, *lcurip;
  INSTRTXT     *tp;
  uint32_t instno;
  uint32_t i;
  OPCODINFO    *inm;
  OPCOD_IOBUFS *buf = p->buf;
  /* look up the 'fake' instr number, and opcode name */
  inm = (OPCODINFO*) p->h.optext->t.oentry->useropinfo;
  if (inm != NULL) {
    /* Find the latest OPCODINFO for this UDO signature, in case it was
       redefined after this instrument was compiled. */
    OPCODINFO *latest = find_latest_useropinfo(csound, inm->name,
                                               inm->outtypes, inm->intypes);
    if (latest != NULL) {
      inm = latest;
    }
  }
  instno = inm->instno;
  tp = csound->engineState.instrtxtp[instno];
  if (tp == NULL)
    return csound->InitError(csound, Str("Cannot find instr %d (UDO %s)\n"),
                             instno, inm->name);
  if (!p->ip) {
    /* search for already allocated, but not active instance */
    /* if none was found, allocate a new instance */
    tp = csound->engineState.instrtxtp[instno];
    if (tp == NULL) {
      return csound->InitError(csound, Str("Cannot find instr %d (UDO %s)\n"),
                               instno, inm->name);
    }
    if (!tp->act_instance)
      instance(csound, instno);
    lcurip = tp->act_instance;            /* use free instance, and */
    tp->act_instance = lcurip->nxtact;    /* remove from chain      */
    if (lcurip->opcod_iobufs==NULL)
      return csound->InitError(csound, "Broken redefinition of UDO %d (UDO %s)\n",
                               instno, inm->name);
    lcurip->actflg++;                     /*    and mark the instr active */
    tp->active++;
    tp->instcnt++;
    /* link into deact chain */
    lcurip->opcod_deact = parent_ip->opcod_deact;
    lcurip->subins_deact = NULL;
    parent_ip->opcod_deact = (void*) p;
    p->ip = lcurip;
    /* IV - Nov 10 2002: set up pointers to I/O buffers */
    buf = p->buf = (OPCOD_IOBUFS*) lcurip->opcod_iobufs;
    buf->opcode_info = inm;
    /* initialise perf time address lists */
    /* **** Could be a memset **** */
    buf->iobufp_ptrs[0] = buf->iobufp_ptrs[1] = NULL;
    buf->iobufp_ptrs[2] = buf->iobufp_ptrs[3] = NULL;
    buf->iobufp_ptrs[4] = buf->iobufp_ptrs[5] = NULL;
    buf->iobufp_ptrs[6] = buf->iobufp_ptrs[7] = NULL;
    buf->iobufp_ptrs[8] = buf->iobufp_ptrs[9] = NULL;
    buf->iobufp_ptrs[10] = buf->iobufp_ptrs[11] = NULL;
    /* store parameters of input and output channels, and parent ip */
    buf->uopcode_struct = (void*) p;
    buf->parent_ip = p->parent_ip = parent_ip;
  } else {
    /* copy parameters from the caller instrument into our subinstrument */
    lcurip = p->ip;
  }

  lcurip->esr = CS_ESR;
  lcurip->pidsr = CS_PIDSR;
  lcurip->sicvt = CS_SICVT;
  lcurip->onedsr = CS_ONEDSR;
  lcurip->ksmps = CS_KSMPS;
  lcurip->kcounter = CS_KCNT;
  lcurip->ekr = CS_EKR;
  lcurip->onedkr = CS_ONEDKR;
  lcurip->onedksmps = CS_ONEDKSMPS;
  lcurip->kicvt = CS_KICVT;



  /* VL 13-12-13 */
  /* this sets ksmps and kr local variables */
  /* create local ksmps variable and init with ksmps */
  if (lcurip->lclbas != NULL) {
    CS_VARIABLE *var =
      csoundFindVariableWithName(csound, lcurip->instr->varPool, "ksmps");
    *((MYFLT *)(var->memBlockIndex + lcurip->lclbas)) = lcurip->ksmps;
    /* same for kr */
    var =
      csoundFindVariableWithName(csound, lcurip->instr->varPool, "kr");
    *((MYFLT *)(var->memBlockIndex + lcurip->lclbas)) = lcurip->ekr;
    /* VL 15-08-24 same for sr */
    var =
      csoundFindVariableWithName(csound, lcurip->instr->varPool, "sr");
    *((MYFLT *)(var->memBlockIndex + lcurip->lclbas)) = lcurip->esr;
  }

  lcurip->m_chnbp = parent_ip->m_chnbp;       /* MIDI parameters */
  lcurip->m_pitch = parent_ip->m_pitch;
  lcurip->m_veloc = parent_ip->m_veloc;
  lcurip->xtratim = parent_ip->xtratim;
  lcurip->m_sust = 0;
  lcurip->relesing = parent_ip->relesing;
  lcurip->offbet = parent_ip->offbet;
  lcurip->offtim = parent_ip->offtim;
  lcurip->nxtolap = NULL;
  lcurip->ksmps_offset = parent_ip->ksmps_offset;
  lcurip->ksmps_no_end = parent_ip->ksmps_no_end;
  lcurip->tieflag = parent_ip->tieflag;
  lcurip->reinitflag = parent_ip->reinitflag;
  /* copy all p-fields, including p1 (will this work ?) */
  if (tp->pmax > 3) {         /* requested number of p-fields */
    uint32 n = tp->pmax, pcnt = 0;
    while (pcnt < n) {
      if ((i = csound->engineState.instrtxtp[parent_ip->insno]->pmax) > pcnt) {
        if (i > n) i = n;
        /* copy next block of p-fields */
        memcpy(&(lcurip->p1) + pcnt, &(parent_ip->p1) + pcnt,
               (size_t) ((i - pcnt) * sizeof(CS_VAR_MEM)));
        pcnt = i;
      }
      /* top level instr reached */
      if (parent_ip->opcod_iobufs == NULL) break;
      parent_ip = ((OPCOD_IOBUFS*) parent_ip->opcod_iobufs)->parent_ip;
    }
  } else {
    memcpy(&(lcurip->p1), &(parent_ip->p1), 3 * sizeof(CS_VAR_MEM));
  }

   // check for setksmps or over/undersample
  csound->curip = lcurip;
  csound->ids = (OPDS *) (lcurip->nxti);
  ATOMIC_SET(p->ip->init_done, 0);
  csound->mode = 1;
  buf->iflag = 0;
  int err = 0;
  while (csound->ids != NULL && err == 0) {
    csound->op = csound->ids->optext->t.oentry->opname;
    if(strcmp("setksmps", csound->op) == 0  ||
       strcmp("oversample", csound->op) == 0 ||
       strcmp("undersample", csound->op) == 0)
       err = (*csound->ids->init)
               (csound, csound->ids);
    csound->ids = csound->ids->nxti;
  }

  inm->passByRef = buf->opcode_info->newStyle &&
    parent_ip->ksmps == p->ip->ksmps &&
    parent_ip->esr == p->ip->esr;

  if(inm->passByRef) {
    handle_pass_by_ref(csound, p, lcurip);
  }

  /* Initialize the UDO */
  csound->curip = lcurip;
  csound->ids = (OPDS *) (lcurip->nxti);
  ATOMIC_SET(p->ip->init_done, 0);
  csound->mode = 1;
  buf->iflag = 0;
  err = 0;
  while (csound->ids != NULL && err == 0) {
    csound->op = csound->ids->optext->t.oentry->opname;
    // don't run setksmps etc
    if(strcmp("setksmps", csound->op) != 0 &&
       strcmp("oversample", csound->op) != 0 &&
       strcmp("undersample", csound->op) != 0)
      err = (*csound->ids->init)(csound, csound->ids);
    csound->ids = csound->ids->nxti;
  }

  if(err) return err;
  if(inm->passByRef) {
    pbr_sync_pass_through_outputs(csound, p, lcurip);
    pbr_writeback_pass_through_inputs(csound, p, lcurip);
  }
  csound->mode = 0;  ATOMIC_SET(p->ip->init_done, 1);

  /* After init chain completes, materialise UDO outputs only for pass-by-copy.
     In pass-by-ref mode, internal outputs are already rewired to caller storage
     and direct pass-through outputs have already been written back. */
  if (!inm->passByRef) {
    OPCOD_IOBUFS *buf_local = p->buf;
    OPCODINFO *inm_local = buf_local->opcode_info;
    CS_VARIABLE* cur = inm_local->out_arg_pool ? inm_local->out_arg_pool->head : NULL;
    MYFLT** internal_ptrs = buf_local->iobufp_ptrs;  // recorded by xoutset (pass-by-copy)
    MYFLT** external_ptrs = p->ar;                   // caller-side pointers
    UOPCODE *udo_local = (UOPCODE*) buf_local->uopcode_struct;
    MYFLT** udo_out_ptrs = udo_local ? udo_local->ar : NULL; // UDO's own outputs

    // Locate xout opcode instance in sub-instrument (to access its args reliably)
    XOUT* xout_node = NULL;
    for (OPDS* op = (OPDS*) lcurip->nxti; op != NULL; op = op->nxti) {
      const char* oname = op->optext ? op->optext->t.oentry->opname : NULL;
      if (oname && oname[0]=='x' && strcmp(oname, "xout") == 0) {
        xout_node = (XOUT*) op; // keep last
      }
    }

    for (i = 0; i < inm_local->outchns && cur; i++) {
      void* src = NULL;
      void* dst = (void*)external_ptrs[i];
      src = (void*)internal_ptrs[i];
      if (src == NULL && xout_node)
        src = (void*)xout_node->args[i]; // prefer xout arg (local var)
      if (src == NULL && udo_out_ptrs)
        src = (void*)udo_out_ptrs[i]; // fallback: UDO's declared OUT var memory

      // If array out still unresolved or aliased to dst, try to locate
      // a concrete local array to copy from
      if ((src == NULL || src == dst) && dst && cur->varType == &CS_VAR_TYPE_ARRAY
          && lcurip && lcurip->instr && lcurip->instr->varPool && lcurip->lclbas) {
        const CS_TYPE* wantedSubType = cur ? cur->subType : NULL;
        CS_VARIABLE* v = lcurip->instr->varPool->head;
        while (v) {
          if (v->varType == &CS_VAR_TYPE_ARRAY && (wantedSubType == NULL
                                                   || v->subType == wantedSubType)) {
            ARRAYDAT* cand = (ARRAYDAT*)(lcurip->lclbas + v->memBlockIndex);
            if (cand && cand->data && cand->allocated > 0
                && cand->dimensions >= 0 && cand->arrayType) {
              if ((void*)cand != dst) {
                src = (void*)cand;
                break;
              }
            }
          }
          v = v->next;
        }
      }

      if (src && dst) {
        if (src != dst) {
          // no copying of a & k types at i-time !!!
          if((cur->varType != &CS_VAR_TYPE_A &&
              cur->varType != &CS_VAR_TYPE_K) ||
             // special case - K-type arg
             inm_local->outtypes[i] == 'K')
            cur->varType->copyValue(csound, cur->varType, dst, src, lcurip);
         }
      }


      cur = cur->next;
    }
  }

  /* copy length related parameters back to caller instr */
  parent_ip->relesing = lcurip->relesing;
  parent_ip->offbet = lcurip->offbet;
  parent_ip->offtim = lcurip->offtim;
  parent_ip->p3 = lcurip->p3;

  /* restore globals */
  csound->ids = saved_ids;
  csound->curip = parent_ip;

  /* ksmps and esr may have changed, check against insdshead
     select perf routine and scale xtratim accordingly.
     4 cases:
     (0) passByRef: select useropcd_passByRef
     (1) local ksmps; local sr == parent sr: select useropcd1
     (2) local ksmps; local sr < parent sr: select useropcd2
     (3) local sr >= parent sr: select useropcd2
  */

  if(inm->passByRef) {
    parent_ip->xtratim = lcurip->xtratim;
    p->h.perf = (SUBR) useropcd_pass_by_ref;
  } else if (lcurip->ksmps != parent_ip->ksmps &&
	     lcurip->esr == parent_ip->esr) {
    MYFLT ksmps_scale = (MYFLT) lcurip->ksmps / parent_ip->ksmps;
    parent_ip->xtratim = lcurip->xtratim * ksmps_scale;
    // (1) local sr == parent sr
    p->h.perf = (SUBR) useropcd_local_ksmps;
  } else if (lcurip->esr < parent_ip->esr) {
    // (2) local sr < parent sr
      int32_t xtratim = lcurip->xtratim;
      if(parent_ip->xtratim < xtratim)
        parent_ip->xtratim = xtratim;
      p->h.perf = (SUBR) useropcd_pass_by_copy;
  } else {
    // (3) local sr >= parent sr
    MYFLT scal = (MYFLT) parent_ip->esr / lcurip->esr;
    int32_t xtratim = lcurip->xtratim*scal;
    if(parent_ip->xtratim < xtratim)
	parent_ip->xtratim = xtratim;
    p->h.perf = (SUBR) useropcd_pass_by_copy;
  }
  // debug msg
   if (UNLIKELY(csound->oparms->odebug))
    csound->Message(csound, "EXTRATIM=> cur(%p): %d, parent(%p): %d\n",
                    lcurip, lcurip->xtratim, parent_ip, parent_ip->xtratim);
  return OK;
}

int32_t useropcd(CSOUND *csound, UOPCODE *p)
{
  if (UNLIKELY(p->h.nxtp)) {
    return csoundPerfError(csound, &(p->h), Str("%s: not initialised"),
                           p->h.optext->t.opcod);
  }
  return OK;
}

/** This function sets up the input
    buffers for a UDO in pass-by-copy.
    If oversampling is set *after* xin, then
    this is called again to set up the oversampling
    buffers.
*/

int32_t set_inbufs(CSOUND *csound,
                   OPDS *h,
                   OPCOD_IOBUFS *buf) {
  OPCODINFO   *inm;
  MYFLT **bufs, **tmp;
  int32_t i;
  CS_VARIABLE* current;
  UOPCODE  *udo;
  MYFLT parent_sr = buf->parent_ip->esr;
  MYFLT esr = h->insdshead->esr;
  MYFLT ratio = esr/parent_sr;
  MYFLT **args = buf->inargs;

  buf->iflag = 1;
  inm = buf->opcode_info;
  udo = (UOPCODE*) buf->uopcode_struct;
  bufs = udo->ar + inm->outchns;
  tmp = buf->iobufp_ptrs;
  current = inm->in_arg_pool->head;

  if(inm->passByRef) {
    return OK;
  }

  for (i = 0; i < inm->inchns; i++) {
    void* in = (void*)bufs[i];
    void* out = (void*)args[i];
    tmp[i + inm->outchns] = out;
    if ((csoundGetTypeForArg(out) != &CS_VAR_TYPE_K &&
         csoundGetTypeForArg(out) != &CS_VAR_TYPE_A) ||
        // special case: K-type inputs
          inm->intypes[i] == 'K') {
      current->varType->copyValue(csound, current->varType, out, in, h->insdshead);
    }
    // set up src units one per input arg - non k/a sigs/arrays are bypassed
    if(esr != parent_sr) {
        if((udo->cvt_in[i] = src_init(csound, h->insdshead->in_cvt,
                                        ratio, current, h->insdshead)) == NULL)
          return csound->InitError(csound, "could not initialise sample rate "
                                   "converter");
      }
    current = current->next;
  }

  return OK;
}



int32_t xinset(CSOUND *csound, XIN *p)
{
  OPCOD_IOBUFS *buf = (OPCOD_IOBUFS*) p->h.insdshead->opcod_iobufs;
  buf->inargs =  p->args;
  return set_inbufs(csound, &(p->h),
                    buf);
}

int32_t xoutset(CSOUND *csound, XOUT *p)
{
  OPCOD_IOBUFS  *buf;
  OPCODINFO   *inm;
  MYFLT       **bufs, **tmp;
  CS_VARIABLE* current;
  UOPCODE  *udo;
  int32_t i;
  MYFLT parent_sr;

  (void) csound;

  buf = (OPCOD_IOBUFS*) p->h.insdshead->opcod_iobufs;
  parent_sr = buf->parent_ip->esr;
  inm = buf->opcode_info;
  udo = (UOPCODE*) buf->uopcode_struct;
  bufs = udo->ar;
  tmp = buf->iobufp_ptrs;
  current = inm->out_arg_pool->head;

  if(inm->passByRef) {
    return OK;
  }

  for (i = 0; i < inm->outchns; i++) {
    void* in = (void*) p->args[i];
    void* out = (void*) bufs[i];
    CS_TYPE* outType = csoundGetTypeForArg(out);
    tmp[i] = in;
    if (outType != &CS_VAR_TYPE_K && outType != &CS_VAR_TYPE_A) {
      current->varType->copyValue(csound, current->varType, out, in, p->h.insdshead);
    }
    if(CS_ESR != parent_sr) {
        // set up src units one per input arg - non k/a sigs/arrays are bypassed
        if((udo->cvt_out[i] = src_init(csound, p->h.insdshead->out_cvt,
                                         parent_sr/CS_ESR, current,
                                         p->h.insdshead)) == 0)
          return csound->InitError(csound, "could not initialise sample rate "
                                   "converter");
      }
    current = current->next;
  }
  return OK;
}


// local ksmps and global sr, pass-by-copy
int32_t useropcd_local_ksmps(CSOUND *csound, UOPCODE *p)
{
  int32_t    g_ksmps, ofs, early, offset, i;
  OPDS *opstart;
  OPCODINFO   *inm;
  CS_VARIABLE* current;
  INSDS    *this_instr = p->ip;
  MYFLT** internal_ptrs = p->buf->iobufp_ptrs;
  MYFLT** external_ptrs = p->ar;
  int32_t done;

  done = ATOMIC_GET(p->ip->init_done);
  if (UNLIKELY(!done)) /* init not done, exit */
    return OK;

  p->ip->relesing = p->parent_ip->relesing;   /* IV - Nov 16 2002 */
  early = p->h.insdshead->ksmps_no_end;
  offset = p->h.insdshead->ksmps_offset;
  p->ip->spin = p->parent_ip->spin;
  p->ip->spout = p->parent_ip->spout;
  inm = p->buf->opcode_info;

  /* global ksmps is the caller instr ksmps minus sample-accurate end */
  g_ksmps = CS_KSMPS - early;

  /* sample-accurate offset */
  ofs = offset;

  if (this_instr->ksmps == 1) {           /* special case for local kr == sr */
    /* clear offsets, since with CS_KSMPS=1
       they don't apply to opcodes, but to the
       calling code (ie. this code)
    */
    this_instr->ksmps_offset = 0;
    this_instr->ksmps_no_end = 0;
    do {
      this_instr->kcounter++; /*kcounter needs to be incremented BEFORE perf */
      /* copy inputs */
      current = inm->in_arg_pool->head;
      for (i = 0; i < inm->inchns; i++) {
        // this hardcoded type check for non-perf time vars needs to change
        //to use generic code...
        // skip a-vars for now, handle uniquely within performance loop
        if (current->varType != &CS_VAR_TYPE_I &&
            current->varType != &CS_VAR_TYPE_b &&
            current->varType != &CS_VAR_TYPE_A &&
            current->subType != &CS_VAR_TYPE_I &&
            current->subType != &CS_VAR_TYPE_A) {
          // This one checks if an array has a subtype of 'i'
          void* in = (void*)external_ptrs[i + inm->outchns];
          void* out = (void*)internal_ptrs[i + inm->outchns];
          current->varType->copyValue(csound, current->varType, out, in, p->h.insdshead);
        } else if (current->varType == &CS_VAR_TYPE_A) {
          MYFLT* in = (void*)external_ptrs[i + inm->outchns];
          MYFLT* out = (void*)internal_ptrs[i + inm->outchns];
          *out = *(in + ofs);
        } else if (current->varType == &CS_VAR_TYPE_ARRAY &&
                   current->subType == &CS_VAR_TYPE_A) {
          ARRAYDAT* src = (ARRAYDAT*)external_ptrs[i + inm->outchns];
          ARRAYDAT* target = (ARRAYDAT*)internal_ptrs[i + inm->outchns];
          int32_t count = src->sizes[0];
          int32_t j;
          if (src->dimensions > 1) {
            for (j = 0; j < src->dimensions; j++) {
              count *= src ->sizes[j];
            }
          }

          for (j = 0; j < count; j++) {
            int32_t memberOffset = j * (src->arrayMemberSize / sizeof(MYFLT));
            MYFLT* in = src->data + memberOffset;
            MYFLT* out = target->data + memberOffset;
            *out = *(in + ofs);
          }
        }
        current = current->next;
      }

      if ((opstart = (OPDS *) (this_instr->nxtp)) != NULL) {
        int32_t error = 0;
        do {
          if(UNLIKELY(!ATOMIC_GET8(p->ip->actflg))) goto endop;
          opstart->insdshead->pds = opstart;
          error = (*opstart->perf)(csound, opstart);
          opstart = opstart->insdshead->pds;
        } while (error == 0 && p->ip != NULL
                 && (opstart = opstart->nxtp));
      }

      /* copy a-sig outputs, accounting for offset */
      current = inm->out_arg_pool->head;
      for (i = 0; i < inm->outchns; i++) {
        if (current->varType == &CS_VAR_TYPE_A) {
          MYFLT* in = (void*)internal_ptrs[i];
          MYFLT* out = (void*)external_ptrs[i];
          *(out + ofs) = *in;
        } else if (current->varType == &CS_VAR_TYPE_ARRAY &&
                   current->subType == &CS_VAR_TYPE_A) {
          ARRAYDAT* src = (ARRAYDAT*)internal_ptrs[i];
          ARRAYDAT* target = (ARRAYDAT*)external_ptrs[i];
          int32_t count = src->sizes[0];
          int32_t j;
          if (src->dimensions > 1) {
            for (j = 0; j < src->dimensions; j++) {
              count *= src->sizes[j];
            }
          }

          for (j = 0; j < count; j++) {
            int32_t memberOffset = j * (src->arrayMemberSize / sizeof(MYFLT));
            MYFLT* in = src->data + memberOffset;
            MYFLT* out = target->data + memberOffset;
            *(out + ofs) = *in;
          }
        }

        current = current->next;
      }

      this_instr->spout += csound->nchnls;
      this_instr->spin  += csound->nchnls;
    } while (++ofs < g_ksmps);
  }
  else {
    /* generic case for local kr != sr */
    /* we have to deal with sample-accurate code
       whole CS_KSMPS blocks are offset here, the
       remainder is left to each opcode to deal with.
    */
    int32_t start = 0;
    int32_t lksmps = this_instr->ksmps;
    while (ofs >= lksmps) {
      ofs -= lksmps;
      start++;
    }
    this_instr->ksmps_offset = ofs;
    ofs = start;
    if (UNLIKELY(early)) this_instr->ksmps_no_end = early % lksmps;
    do {
      this_instr->kcounter++;
      /* copy a-sig inputs, accounting for offset */
      size_t asigSize = (this_instr->ksmps * sizeof(MYFLT));
      current = inm->in_arg_pool->head;
      for (i = 0; i < inm->inchns; i++) {
        // this hardcoded type check for non-perf time vars needs to change
        // to use generic code...
        // skip a-vars for now, handle uniquely within performance loop
        if (current->varType != &CS_VAR_TYPE_I &&
            current->varType != &CS_VAR_TYPE_b &&
            current->varType != &CS_VAR_TYPE_A &&
            current->subType != &CS_VAR_TYPE_I &&
            current->subType != &CS_VAR_TYPE_A) {
          // This one checks if an array has a subtype of 'i'
          void* in = (void*)external_ptrs[i + inm->outchns];
          void* out = (void*)internal_ptrs[i + inm->outchns];
          current->varType->copyValue(csound, current->varType, out, in, p->h.insdshead);
        } else if (current->varType == &CS_VAR_TYPE_A) {
          MYFLT* in = (void*)external_ptrs[i + inm->outchns];
          MYFLT* out = (void*)internal_ptrs[i + inm->outchns];
          memcpy(out, in + ofs, asigSize);
        } else if (current->varType == &CS_VAR_TYPE_ARRAY &&
                   current->subType == &CS_VAR_TYPE_A) {
          ARRAYDAT* src = (ARRAYDAT*)external_ptrs[i + inm->outchns];
          ARRAYDAT* target = (ARRAYDAT*)internal_ptrs[i + inm->outchns];
          int32_t count = src->sizes[0];
          int32_t j;
          if (src->dimensions > 1) {
            for (j = 0; j < src->dimensions; j++) {
              count *= src->sizes[j];
            }
          }

          for (j = 0; j < count; j++) {
            int memberOffset = j * (src->arrayMemberSize / sizeof(MYFLT));
            MYFLT* in = src->data + memberOffset;
            MYFLT* out = target->data + memberOffset;
            memcpy(out, in + ofs, asigSize);
          }
        }
        current = current->next;
      }

      this_instr->ksmps_offset = 0; /* reset sample-accuracy offset for UDO */
      this_instr->ksmps_no_end = 0;  /* reset end of loop samples for UDO */

      /*  run each opcode  */
      if ((opstart = (OPDS *) (this_instr->nxtp)) != NULL) {
        int32_t error = 0;
        do {
          if(UNLIKELY(!ATOMIC_GET8(p->ip->actflg))) goto endop;
          opstart->insdshead->pds = opstart;
          error = (*opstart->perf)(csound, opstart);
          opstart = opstart->insdshead->pds;
        } while (error == 0 && p->ip != NULL
                 && (opstart = opstart->nxtp));
      }

      /* copy a-sig outputs, accounting for offset */
      current = inm->out_arg_pool->head;
      for (i = 0; i < inm->outchns; i++) {
        if (current->varType == &CS_VAR_TYPE_A) {
          MYFLT* in = (void*)internal_ptrs[i];
          MYFLT* out = (void*)external_ptrs[i];
          memcpy(out + ofs, in, asigSize);
        } else if (current->varType == &CS_VAR_TYPE_ARRAY &&
                   current->subType == &CS_VAR_TYPE_A) {
          ARRAYDAT* src = (ARRAYDAT*)internal_ptrs[i];
          ARRAYDAT* target = (ARRAYDAT*)external_ptrs[i];
          int32_t count = src->sizes[0];
          int32_t j;
          if (src->dimensions > 1) {
            for (j = 0; j < src->dimensions; j++) {
              count *= src->sizes[j];
            }
          }
          for (j = 0; j < count; j++) {
            int memberOffset = j * (src->arrayMemberSize / sizeof(MYFLT));
            MYFLT* in = src->data + memberOffset;
            MYFLT* out = target->data + memberOffset;
            memcpy(out + ofs, in, asigSize);
          }

        }
        current = current->next;
      }

      this_instr->spout += csound->nchnls*lksmps;
      this_instr->spin  += csound->nchnls*lksmps;

    } while ((ofs += this_instr->ksmps) < g_ksmps);
  }


  /* copy outputs */
  current = inm->out_arg_pool->head;
  for (i = 0; i < inm->outchns; i++) {
    // this hardcoded type check for non-perf time vars needs to change
    // to use generic code...
    if (current->varType != &CS_VAR_TYPE_I &&
        current->varType != &CS_VAR_TYPE_b &&
        current->subType != &CS_VAR_TYPE_I) {
      void* in = (void*)internal_ptrs[i];
      void* out = (void*)external_ptrs[i];

      if (current->varType == &CS_VAR_TYPE_A) {
        /* clear the beginning portion of outputs for sample accurate end */
        if (offset) {
          memset(out, '\0', sizeof(MYFLT) * offset);
        }

        /* clear the end portion of outputs for sample accurate end */
        if (early) {
          memset((char*)out + g_ksmps, '\0', sizeof(MYFLT) * early);
        }
      } else if (current->varType == &CS_VAR_TYPE_ARRAY &&
                 current->subType == &CS_VAR_TYPE_A) {
        if (offset || early) {
          ARRAYDAT* outDat = (ARRAYDAT*)out;
          int32_t count = outDat->sizes[0];
          int32_t j;
          if (outDat->dimensions > 1) {
            for (j = 0; j < outDat->dimensions; j++) {
              count *= outDat->sizes[j];
            }
          }

          if (offset) {
            for (j = 0; j < count; j++) {
              int memberOffset = j * (outDat->arrayMemberSize / sizeof(MYFLT));
              MYFLT* outMem = outDat->data + memberOffset;
              memset(outMem, '\0', sizeof(MYFLT) * offset);
            }
          }

          if (early) {
            for (j = 0; j < count; j++) {
              int32_t memberOffset = j * (outDat->arrayMemberSize / sizeof(MYFLT));
              MYFLT* outMem = outDat->data + memberOffset;
              memset(outMem + g_ksmps, '\0', sizeof(MYFLT) * early);
            }
          }
        }
      } else {
        current->varType->copyValue(csound, current->varType, out, in, p->h.insdshead);
      }
    }
    current = current->next;
  }
 endop:
  /* check if instrument was deactivated (e.g. by perferror) */
  if (!p->ip)                                         /* loop to last opds */
    while (CS_PDS && CS_PDS->nxtp) CS_PDS = CS_PDS->nxtp;

  return OK;
}

// global ksmps and global or local sr, pass-by-copy
int32_t useropcd_pass_by_copy(CSOUND *csound, UOPCODE *p)
{
  MYFLT   **tmp;
  OPCODINFO   *inm;
  CS_VARIABLE* current;
  int32_t i, done;
  int32_t os = (int) (p->ip->esr/p->parent_ip->esr);
  inm = (OPCODINFO*) p->h.optext->t.oentry->useropinfo;
  done = ATOMIC_GET(p->ip->init_done);

  if (UNLIKELY(!done)) /* init not done, exit */
    return OK;

  /* VL 18.12.24: ksmps_no_end is copied here as it
     applies only to last kcycle */
  p->ip->ksmps_no_end = p->h.insdshead->ksmps_no_end;
  p->ip->spin = p->parent_ip->spin;
  p->ip->spout = p->parent_ip->spout;

  if (UNLIKELY(p->ip->nxtp == NULL))
    goto endop; /* no perf code */

  p->ip->relesing = p->parent_ip->relesing;
  tmp = p->buf->iobufp_ptrs;
  inm = p->buf->opcode_info;

  MYFLT** internal_ptrs = tmp;
  MYFLT** external_ptrs = p->ar;
  int32_t ocnt = 0;

  /*  run each opcode, oversampling if necessary  */
  for(ocnt = 0; ocnt < os; ocnt++){
    int error = 0;
    int cvt;
    OPDS *opstart;
    /* copy inputs */
    current = inm->in_arg_pool->head;
    for (i = cvt = 0; i < inm->inchns; i++, cvt++) {
      // this hardcoded type check for non-perf time vars needs to
      // change to use generic code...
      if (current->varType != &CS_VAR_TYPE_I &&
          current->varType != &CS_VAR_TYPE_b &&
          current->subType != &CS_VAR_TYPE_I) {
        if(os == 1) {
          if (current->varType == &CS_VAR_TYPE_A && CS_KSMPS == 1) {
            *internal_ptrs[i + inm->outchns] = *external_ptrs[i + inm->outchns];
          } else {
            void* in = (void*)external_ptrs[i + inm->outchns];
            void* out = (void*)internal_ptrs[i + inm->outchns];
            current->varType->copyValue(csound, current->varType, out, in, p->h.insdshead);
          }
        } else { // under/oversampling
          void* in = (void*) external_ptrs[i + inm->outchns];
          void* out = (void*) internal_ptrs[i + inm->outchns];
          src_convert(csound, p->cvt_in[cvt], in, out);
        }
      }
      current = current->next;
    }

    if ((opstart = (OPDS *) (p->ip->nxtp)) != NULL) {
      p->ip->kcounter++;  /* kcount should be incremented BEFORE perf */
      do {
        if(UNLIKELY(!ATOMIC_GET8(p->ip->actflg))) goto endop;
        opstart->insdshead->pds = opstart;
        error = (*opstart->perf)(csound, opstart);
        opstart = opstart->insdshead->pds;
      } while (error == 0 && p->ip != NULL
               && (opstart = opstart->nxtp));
    }

    /* copy outputs */
    current = inm->out_arg_pool->head;
    for (i = cvt = 0; i < inm->outchns; i++, cvt++) {
      // this hardcoded type check for non-perf time vars needs to change to
      // use generic code...
      if (current->varType != &CS_VAR_TYPE_I &&
          current->varType != &CS_VAR_TYPE_b &&
          current->subType != &CS_VAR_TYPE_I) {
        if(os == 1) {
          if (current->varType == &CS_VAR_TYPE_A && CS_KSMPS == 1) {
            *external_ptrs[i] = *internal_ptrs[i];
          } else {
            void* in = (void*)internal_ptrs[i];
            void* out = (void*)external_ptrs[i];
            current->varType->copyValue(csound, current->varType,
                                        out, in, p->h.insdshead);
          }
        }
        else { // under/oversampling
          void* in = (void*)internal_ptrs[i];
          void* out = (void*)external_ptrs[i];
          src_convert(csound, p->cvt_out[cvt], in, out);
        }
      }
      current = current->next;
    }
  }

 endop:
  /* check if instrument was deactivated (e.g. by perferror) */
  if (!p->ip)  {                   /* loop to last opds */
    while (CS_PDS && CS_PDS->nxtp) {
      CS_PDS = CS_PDS->nxtp;
    }
  }
  if(p->ip) { // check in case instrument has called turnoff
  p->ip->ksmps_offset = 0; /* reset sample-accuracy offset */
  p->ip->ksmps_no_end = 0;  /* reset end of loop samples */
  }
  return OK;
}

/** Runs perf-time chain*/
int32_t useropcd_pass_by_ref(CSOUND *csound, UOPCODE *p)
{
  OPDS    *saved_pds = CS_PDS;
  int32_t done;
  done = ATOMIC_GET(p->ip->init_done);
  if (UNLIKELY(!done)) /* init not done, exit */
    return OK;
  p->ip->spin = p->parent_ip->spin;
  p->ip->spout = p->parent_ip->spout;
  pbr_seed_pass_through_outputs(csound, p, p->ip);
  p->ip->kcounter++;  /* kcount should be incremented BEFORE perf */
  if (UNLIKELY(!(CS_PDS = (OPDS*) (p->ip->nxtp))))
    goto writeback; /* no perf code */

  /* IV - Nov 16 2002: update release flag */
  p->ip->relesing = p->parent_ip->relesing;

  /*  run each opcode  */
  {
  int error = 0;
  CS_PDS->insdshead->pds = NULL;
  do {
    if(UNLIKELY(!ATOMIC_GET8(p->ip->actflg))) goto endop;
    if (CS_PDS->perf) {
      error = (*CS_PDS->perf)(csound, CS_PDS);
    }
    if (CS_PDS->insdshead->pds != NULL &&
        CS_PDS->insdshead->pds->insdshead) {
      CS_PDS = CS_PDS->insdshead->pds;
      CS_PDS->insdshead->pds = NULL;
    }
  } while (error == 0 && p->ip != NULL
           && (CS_PDS = CS_PDS->nxtp));
  }

 writeback:
  pbr_sync_pass_through_outputs(csound, p, p->ip);
  pbr_writeback_pass_through_inputs(csound, p, p->ip);

 endop:
  /* restore globals */
  CS_PDS = saved_pds;
  /* check if instrument was deactivated (e.g. by perferror) */
  if (!p->ip)  {                   /* loop to last opds */
    while (CS_PDS && CS_PDS->nxtp) {
      CS_PDS = CS_PDS->nxtp;
    }
  }
  return OK;
}


/*
  This opcode sets the local ksmps for an instrument
  it can be used on any instrument with the implementation
  of a mechanism to perform at local ksmps (in kperf etc)
*/
int32_t setksmpsset(CSOUND *csound, SETKSMPS *p)
{

  uint32_t  l_ksmps, n;
  OPCOD_IOBUFS *udo = (OPCOD_IOBUFS *) p->h.insdshead->opcod_iobufs;
  MYFLT parent_sr = udo ? udo->parent_ip->esr : csound->esr;
  MYFLT parent_ksmps = udo ? udo->parent_ip->ksmps : csound->ksmps;

  if(CS_ESR != parent_sr)
    return csoundInitError(csound,
                           "can't set ksmps value if local sr != parent sr\n");
  if(CS_KSMPS != parent_ksmps) return OK; // no op if this has already changed

  l_ksmps = (uint32_t) *(p->i_ksmps);
  if (!l_ksmps) return OK;       /* zero: do not change */
  if (UNLIKELY(l_ksmps < 1 || l_ksmps > CS_KSMPS ||
               ((CS_KSMPS / l_ksmps) * l_ksmps != CS_KSMPS))) {
    return csoundInitError(csound,
                           Str("setksmps: invalid ksmps value: %d, original: %d"),
                           l_ksmps, CS_KSMPS);
  }

  n = CS_KSMPS / l_ksmps;
  p->h.insdshead->xtratim *= n;
  CS_KSMPS = l_ksmps;
  CS_ONEDKSMPS = FL(1.0) / (MYFLT) CS_KSMPS;
  CS_EKR = CS_ESR / (MYFLT) CS_KSMPS;
  CS_ONEDKR = FL(1.0) / CS_EKR;
  CS_KICVT = (MYFLT) FMAXLEN / CS_EKR;
  CS_KCNT *= n;

  /* VL 13-12-13 */
  /* this sets ksmps and kr local variables */
  /* lookup local ksmps variable and init with ksmps */
  INSTRTXT *ip = p->h.insdshead->instr;
  CS_VARIABLE *var =
    csoundFindVariableWithName(csound, ip->varPool, "ksmps");
  MYFLT *varmem = p->h.insdshead->lclbas + var->memBlockIndex;
  *varmem = CS_KSMPS;

  /* same for kr */
  var =
    csoundFindVariableWithName(csound, ip->varPool, "kr");
  varmem = p->h.insdshead->lclbas + var->memBlockIndex;
  *varmem = CS_EKR;

  return OK;
}

/* oversample opcode
   oversample ifactor
   ifactor - oversampling factor (positive integer)

   if oversampling is used, xin/xout need
   to initialise the converters.
   oversampling is not allowed with local ksmps or
   with audio/control array arguments.
*/
int32_t oversampleset(CSOUND *csound, OVSMPLE *p) {
  int32_t os;
  MYFLT l_sr, onedos;
  OPCOD_IOBUFS *udo = (OPCOD_IOBUFS *) p->h.insdshead->opcod_iobufs;
  MYFLT parent_sr, parent_ksmps;

  if(udo == NULL)
    return csound->InitError(csound, "oversampling only allowed in UDOs\n");

  parent_sr = udo->parent_ip->esr;
  parent_ksmps = udo->parent_ip->ksmps;

  if(CS_KSMPS != parent_ksmps)
    return csoundInitError(csound,
                           "can't oversample if local ksmps != parent ksmps\n");

  os = MYFLT2LRND(*p->os);
  onedos = FL(1.0)/os;
  if(os < 1)
    return csound->InitError(csound, "illegal oversampling ratio: %d\n", os);
  if(os == 1 || CS_ESR != parent_sr) return OK; /* no op if changed already */

  l_sr = CS_ESR*os;
  CS_ESR = l_sr;
  CS_PIDSR = PI/l_sr;
  CS_ONEDSR = 1./l_sr;
  CS_SICVT = (MYFLT) FMAXLEN / CS_ESR;
  CS_EKR = CS_ESR/CS_KSMPS;
  CS_ONEDKR = 1./CS_EKR;
  CS_KICVT = (MYFLT) FMAXLEN / CS_EKR;
  /* ksmsp does not change,
     however, because we are oversampling, we will need
     to run the code os times in a loop to consume
     os*ksmps input samples and produce os*ksmps output
     samples. This means that the kcounter will run fast by a
     factor of 1/os, and xtratim also needs to be scaled by
     that factor
  */
  p->h.insdshead->xtratim *= os;
  CS_KCNT *= onedos;
  /* oversampling mode (s) */
  p->h.insdshead->in_cvt = *p->in_cvt >= 0 ? MYFLT2LRND(*p->in_cvt) : 0;
  if(*p->out_cvt >= 0)
    p->h.insdshead->out_cvt = MYFLT2LRND(*p->out_cvt);
  else p->h.insdshead->out_cvt = p->h.insdshead->in_cvt;
  /* set local sr variable */
  INSTRTXT *ip = p->h.insdshead->instr;
  CS_VARIABLE *var =
    csoundFindVariableWithName(csound, ip->varPool, "sr");
  MYFLT *varmem = p->h.insdshead->lclbas + var->memBlockIndex;
  *varmem = CS_ESR;
  var = csoundFindVariableWithName(csound, ip->varPool, "kr");
  varmem = p->h.insdshead->lclbas + var->memBlockIndex;
  *varmem = CS_EKR;

  if(udo->iflag) // if xin has already been called, reset bufs
    return set_inbufs(csound, &(p->h), udo);
  else return OK;
}

/* undersample opcode
   undersample ifactor
   ifactor - undersampling factor (positive integer)

   if ubdersampling is used, xin/xout need
   to initialise the converters.
   undersampling is not allowed with
   with audio/control array arguments.
   It modifies ksmps according to the resampling factor.
*/
int32_t undersampleset(CSOUND *csound, OVSMPLE *p) {
  int32_t os, lksmps;
  MYFLT l_sr, onedos;
  OPCOD_IOBUFS *udo = (OPCOD_IOBUFS *) p->h.insdshead->opcod_iobufs;
  MYFLT parent_sr, parent_ksmps;

  if(udo == NULL)
    return csound->InitError(csound, "oversampling only allowed in UDOs\n");

  parent_sr = udo->parent_ip->esr;
  parent_ksmps = udo->parent_ip->ksmps;

  if(CS_KSMPS != parent_ksmps)
    return csoundInitError(csound,
                           "can't undersample if local ksmps != parent ksmps\n");

  os = MYFLT2LRND(*p->os);
  onedos = FL(1.0)/os;
  if(os < 1)
    return csound->InitError(csound,
                             "illegal undersampling ratio: %d\n", os);

  if(os == 1 || CS_ESR != parent_sr) return OK; /* no op if already changed */

  /* round to an integer number of ksmps */
  lksmps = MYFLT2LRND(CS_KSMPS*onedos);
  /* and check */
  if(lksmps < 1)
    return csound->InitError(csound,
                             "illegal oversampling ratio: %d\n", os);

  /* set corrected ratio  */
  onedos = (MYFLT) lksmps/CS_KSMPS;

  /* and now local ksmps */
  CS_KSMPS = lksmps;
  CS_ONEDKSMPS = FL(1.0)/lksmps;
  l_sr = CS_ESR*onedos;
  CS_ESR = l_sr;
  CS_PIDSR = PI/l_sr;
  CS_ONEDSR = 1./l_sr;
  CS_SICVT = (MYFLT) FMAXLEN / CS_ESR;
  CS_EKR = CS_ESR/CS_KSMPS;
  CS_ONEDKR = 1./CS_EKR;
  CS_KICVT = (MYFLT) FMAXLEN / CS_EKR;

  p->h.insdshead->xtratim *= onedos;
  CS_KCNT *= FL(1.0)/onedos;
  /* undersampling mode (s) */
  p->h.insdshead->in_cvt = *p->in_cvt >= 0 ? MYFLT2LRND(*p->in_cvt) : 0;
  if(*p->out_cvt >= 0)
    p->h.insdshead->out_cvt = MYFLT2LRND(*p->out_cvt);
  else p->h.insdshead->out_cvt = p->h.insdshead->in_cvt;
  /* set local sr variable */
  INSTRTXT *ip = p->h.insdshead->instr;
  CS_VARIABLE *var =
    csoundFindVariableWithName(csound, ip->varPool, "sr");
  MYFLT *varmem = p->h.insdshead->lclbas + var->memBlockIndex;
  *varmem = CS_ESR;
  var = csoundFindVariableWithName(csound, ip->varPool, "kr");
  varmem = p->h.insdshead->lclbas + var->memBlockIndex;
  *varmem = CS_EKR;
  var = csoundFindVariableWithName(csound, ip->varPool, "ksmps");
  varmem = p->h.insdshead->lclbas + var->memBlockIndex;
  *varmem = CS_KSMPS;

  if(udo->iflag) // if xin has already been called, reset bufs
    return set_inbufs(csound, &(p->h), udo);
  else return OK;
}


/*
 * subinstr opcode
 */
int32_t subinstrset_(CSOUND *csound, SUBINST *p, int32_t instno)
{
  OPDS    *saved_ids = csound->ids;
  INSDS   *saved_curip = csound->curip;
  CS_VAR_MEM   *pfield;
  int32_t     n, init_op, inarg_ofs;
  INSDS  *pip = p->h.insdshead;

  init_op = (p->h.perf == NULL ? 1 : 0);
  inarg_ofs = (init_op ? 0 : SUBINSTNUMOUTS);
  if (UNLIKELY(instno < 0)) return NOTOK;
  /* IV - Oct 9 2002: need this check */
  if (UNLIKELY(!init_op && p->OUTOCOUNT > csound->nchnls)) {
    return csoundInitError(csound, "%s",Str("subinstr: number of output "
                                       "args greater than nchnls"));
  }
  /* IV - Oct 9 2002: copied this code from useropcdset() to fix some bugs */
  if (!(pip->reinitflag | pip->tieflag) || p->ip == NULL) {
    /* get instance */
    if (csound->engineState.instrtxtp[instno]->act_instance == NULL)
      instance(csound, instno);
    p->ip = csound->engineState.instrtxtp[instno]->act_instance;
    csound->engineState.instrtxtp[instno]->act_instance = p->ip->nxtact;
    p->ip->insno = (int16) instno;
    p->ip->actflg++;                  /*    and mark the instr active */
    csound->engineState.instrtxtp[instno]->active++;
    csound->engineState.instrtxtp[instno]->instcnt++;
    p->ip->p1.value = (MYFLT) instno;
    /* VL 21-10-16: iobufs are not used here and
       are causing trouble elsewhere. Commenting
       it out */
    /* p->ip->opcod_iobufs = (void*) &p->buf; */
    /* link into deact chain */
    p->ip->subins_deact = saved_curip->subins_deact;
    p->ip->opcod_deact = NULL;
    saved_curip->subins_deact = (void*) p;
    p->parent_ip = p->buf.parent_ip = saved_curip;
  }

  p->ip->esr = CS_ESR;
  p->ip->pidsr = CS_PIDSR;
  p->ip->sicvt = CS_SICVT;
  p->ip->onedsr = CS_ONEDSR;
  p->ip->ksmps = CS_KSMPS;
  p->ip->kcounter = CS_KCNT;
  p->ip->ekr = CS_EKR;
  p->ip->onedkr = CS_ONEDKR;
  p->ip->onedksmps = CS_ONEDKSMPS;
  p->ip->kicvt = CS_KICVT;

  /* copy parameters from this instrument into our subinstrument */
  p->ip->xtratim  = saved_curip->xtratim;
  p->ip->m_sust   = 0;
  p->ip->relesing = saved_curip->relesing;
  p->ip->offbet   = saved_curip->offbet;
  p->ip->offtim   = saved_curip->offtim;
  p->ip->nxtolap  = NULL;
  p->ip->p2       = saved_curip->p2;
  p->ip->p3       = saved_curip->p3;

  /* IV - Oct 31 2002 */
  p->ip->m_chnbp  = saved_curip->m_chnbp;
  p->ip->m_pitch  = saved_curip->m_pitch;
  p->ip->m_veloc  = saved_curip->m_veloc;

  p->ip->ksmps_offset =  saved_curip->ksmps_offset;
  p->ip->ksmps_no_end =  saved_curip->ksmps_no_end;
  p->ip->tieflag = saved_curip->tieflag;
  p->ip->reinitflag = saved_curip->reinitflag;

  /* copy remainder of pfields */
  pfield = (CS_VAR_MEM*)&p->ip->p3;
  /* by default all inputs are i-rate mapped to p-fields */
  if (UNLIKELY(p->INOCOUNT >
               (unsigned int)(csound->engineState.instrtxtp[instno]->pmax + 1)))
    return csoundInitError(csound, "%s", Str("subinstr: too many p-fields"));
#ifdef USE_DOUBLE
  union {
    MYFLT d;
    int32 i[2];
  } ch;
  int32_t sel = byte_order()==0? 1 :0;
  int32_t str_cnt = 0, len = 0;
  char *argstr;
  for (n = 1; (uint32_t) n < p->INOCOUNT; n++){
    if (IS_STR_ARG(p->ar[inarg_ofs + n])) {
      ch.d = SSTRCOD;
      ch.i[sel] += str_cnt & 0xffff;
      (pfield + n)->value = ch.d;
      argstr = ((STRINGDAT *)p->ar[inarg_ofs + n])->data;
      if (str_cnt == 0)
        p->ip->strarg = csound->Calloc(csound, strlen(argstr)+1);
      else
        p->ip->strarg = csound->ReAlloc(csound, p->ip->strarg,
                                        len+strlen(argstr)+1);
      strcpy(p->ip->strarg + len, argstr);
      len += strlen(argstr)+1;
      str_cnt++;
    }

    else (pfield + n)->value = *p->ar[inarg_ofs + n];
  }
#else
  union {
    MYFLT d;
    int32 j;
  } ch;
  int32_t str_cnt = 0, len = 0;
  char *argstr;
  for (n = 1; (uint32_t) n < p->INOCOUNT; n++){
    if (IS_STR_ARG(p->ar[inarg_ofs + n])) {
      ch.d = SSTRCOD;
      ch.j += str_cnt & 0xffff;
      (pfield + n)->value = ch.d;
      argstr = ((STRINGDAT *)p->ar[inarg_ofs + n])->data;
      if (str_cnt == 0)
        p->ip->strarg = csound->Calloc(csound, strlen(argstr)+1);
      else
        p->ip->strarg = csound->ReAlloc(csound, p->ip->strarg,
                                        len+strlen(argstr)+1);
      strcpy(p->ip->strarg + len, argstr);
      len += strlen(argstr)+1;
      str_cnt++;
    }
    else (pfield + n)->value = *p->ar[inarg_ofs + n];
  }
#endif

  // allocate memory for a temporary store of spout buffers
  if (!init_op && !(pip->reinitflag | pip->tieflag))
    csound->AuxAlloc(csound, (int32) csound->nspout * sizeof(MYFLT), &p->saved_spout);

  /* do init pass for this instr */
  csound->curip = p->ip;        /* **** NEW *** */
  p->ip->init_done = 0;
  csound->ids = (OPDS *)p->ip;
  csound->mode = 1;
  while ((csound->ids = csound->ids->nxti) != NULL) {
    csound->op = csound->ids->optext->t.oentry->opname;
    (*csound->ids->init)(csound, csound->ids);
  }
  csound->mode = 0;
  p->ip->init_done = 1;
  /* copy length related parameters back to caller instr */
  saved_curip->xtratim = csound->curip->xtratim;
  saved_curip->relesing = csound->curip->relesing;
  saved_curip->offbet = csound->curip->offbet;
  saved_curip->offtim = csound->curip->offtim;
  saved_curip->p3 = csound->curip->p3;

  /* restore globals */
  csound->ids = saved_ids;
  csound->curip = saved_curip;
  return OK;
}

int32_t subinstrset_S(CSOUND *csound, SUBINST *p){
  int32_t instno, init_op, inarg_ofs;
  /* check if we are using subinstrinit or subinstr */
  init_op = (p->h.perf == NULL ? 1 : 0);
  inarg_ofs = (init_op ? 0 : SUBINSTNUMOUTS);
  instno = csoundStringArg2Insno(csound, ((STRINGDAT *)p->ar[inarg_ofs])->data, 1);
  if (UNLIKELY(instno==NOT_AN_INSTRUMENT)) instno = -1;
  return subinstrset_(csound,p,instno);
}


int32_t subinstrset(CSOUND *csound, SUBINST *p){
  int32_t instno, init_op, inarg_ofs;
  /* check if we are using subinstrinit or subinstr */
  init_op = (p->h.perf == NULL ? 1 : 0);
  inarg_ofs = (init_op ? 0 : SUBINSTNUMOUTS);
  instno = (int32_t) *(p->ar[inarg_ofs]);
  return subinstrset_(csound,p,instno);
}

int32_t subinstr(CSOUND *csound, SUBINST *p)
{
  OPDS    *saved_pds = CS_PDS;
  MYFLT   *pbuf;
  uint32_t frame, chan;
  uint32_t nsmps = CS_KSMPS;
  INSDS *ip = p->ip;
  int32_t done = ATOMIC_GET(p->ip->init_done);

  if (UNLIKELY(!done)) /* init not done, exit */
    return OK;

  if (UNLIKELY(p->ip == NULL)) {                /* IV - Oct 26 2002 */
    return csoundPerfError(csound, &(p->h), "%s",
                           Str("subinstr: not initialised"));
  }

  /* copy current spout buffer and clear it */
  ip->spout = (MYFLT*) p->saved_spout.auxp;
  memset(ip->spout, 0, csound->nspout*sizeof(MYFLT));

  /* update release flag */
  ip->relesing = p->parent_ip->relesing;   /* IV - Nov 16 2002 */
  /*  run each opcode  */
  if (csound->ksmps == ip->ksmps) {
    int32_t error = 0;
    ip->kcounter++;
    if ((CS_PDS = (OPDS *) (ip->nxtp)) != NULL) {
      CS_PDS->insdshead->pds = NULL;
      do {
        error = (*CS_PDS->perf)(csound, CS_PDS);
        if (CS_PDS->insdshead->pds != NULL) {
          CS_PDS = CS_PDS->insdshead->pds;
          CS_PDS->insdshead->pds = NULL;
        }
      } while (error == 0 && (CS_PDS = CS_PDS->nxtp));
    }

  }
  else {
    int32_t i, n = csound->nspout, start = 0;
    int32_t lksmps = ip->ksmps;
    int32_t incr = csound->nchnls*lksmps;
    int32_t offset =  ip->ksmps_offset;
    int32_t early = ip->ksmps_no_end;
    ip->spin = csound->spin;
    ip->kcounter =  csound->kcounter*csound->ksmps/lksmps;

    /* we have to deal with sample-accurate code
       whole CS_KSMPS blocks are offset here, the
       remainder is left to each opcode to deal with.
    */
    while (offset >= lksmps) {
      offset -= lksmps;
      start += csound->nchnls;
    }
    ip->ksmps_offset = offset;
    if (early) {
      n -= (early*csound->nchnls);
      ip->ksmps_no_end = early % lksmps;
    }

    for (i=start; i < n; i+=incr, ip->spin+=incr, ip->spout+=incr) {
      ip->kcounter++;
      if ((CS_PDS = (OPDS *) (ip->nxtp)) != NULL) {
        int32_t error = 0;
        CS_PDS->insdshead->pds = NULL;
        do {
          if(UNLIKELY(!ATOMIC_GET8(p->ip->actflg))){
            memset(p->ar, 0, sizeof(MYFLT)*CS_KSMPS*p->OUTOCOUNT);
            goto endin;
          }
          error = (*CS_PDS->perf)(csound, CS_PDS);
          if (CS_PDS->insdshead->pds != NULL) {
            CS_PDS = CS_PDS->insdshead->pds;
            CS_PDS->insdshead->pds = NULL;
          }
        } while (error == 0 && (CS_PDS = CS_PDS->nxtp));
      }
    }
    ip->spout = (MYFLT*) p->saved_spout.auxp;
  }
  /* copy outputs */
  for (chan = 0; chan < p->OUTOCOUNT; chan++) {
    for (pbuf = ip->spout + chan*nsmps, frame = 0;
         frame < nsmps; frame++) {
      p->ar[chan][frame] = pbuf[frame];
    }
  }
 endin:
  CS_PDS = saved_pds;
  /* check if instrument was deactivated (e.g. by perferror) */
  if (!p->ip) {                                  /* loop to last opds */
    while (CS_PDS->nxtp) {
      CS_PDS = CS_PDS->nxtp;
    }
  }
  return OK;
}
