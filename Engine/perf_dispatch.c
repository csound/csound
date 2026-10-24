/* Generic performance dispatch. Copyright (C) 2026 The Csound Developers.
   Licensed under the GNU Lesser General Public License, version 2.1 or later. */
#include "csoundCore.h"
#include "perf_dispatch.h"

#define PERF_PLAN_NAME "__csound_perf_plan"

static PERF_TARGET *targets(PERF_PLAN *plan)
{
  return (PERF_TARGET *)(plan->entries + plan->count + 1);
}

static size_t target_hash(OPDS *op, size_t mask)
{
  uintptr_t key = (uintptr_t)op >> 3;
  key ^= key >> 17;
  key *= (uintptr_t)0x9e3779b1u;
  return (size_t)key & mask;
}

PERF_PLAN *csoundGetPerfPlan(CSOUND *csound, INSDS *ip)
{
  return csoundQueryInstanceVariable(csound, ip, PERF_PLAN_NAME);
}

void csoundPreparePerfPlan(CSOUND *csound, INSDS *ip)
{
  size_t count = 0, capacity = 2;
  if (csoundGetPerfPlan(csound, ip) != NULL) return; /* tie/reinit */
  for (OPDS *op = ip->nxtp; op != NULL; op = op->nxtp) count++;
  if (count == 0 || count > SIZE_MAX / 2 - 1) return;
  while (capacity < (count + 1) * 2) {
    if (capacity > SIZE_MAX / 2) return;
    capacity *= 2;
  }
  if (count + 1 > (SIZE_MAX - sizeof(PERF_PLAN)) / sizeof(PERF_ENTRY))
    return;
  size_t bytes = sizeof(PERF_PLAN) + (count + 1) * sizeof(PERF_ENTRY);
  if (capacity > (SIZE_MAX - bytes) / sizeof(PERF_TARGET)) return;
  bytes += capacity * sizeof(PERF_TARGET);
  if (csoundCreateInstanceVariable(csound, ip, PERF_PLAN_NAME, bytes) != OK)
    return; /* allocation failure keeps linked dispatch */
  PERF_PLAN *plan = csoundGetPerfPlan(csound, ip);
  plan->count = count;
  plan->mask = capacity - 1;
  PERF_TARGET *map = targets(plan);
  size_t i = 0;
  OPDS *previous = (OPDS *)ip;
  for (OPDS *op = ip->nxtp; ; previous = op, op = op->nxtp) {
    plan->entries[i].op = op;
    size_t slot = target_hash(previous, plan->mask);
    while (map[slot].previous != NULL) slot = (slot + 1) & plan->mask;
    map[slot].previous = previous;
    map[slot].next = &plan->entries[i++];
    if (op == NULL) break;
  }
}

static PERF_ENTRY *find_target(PERF_PLAN *plan, OPDS *previous)
{
  if (previous == NULL) return &plan->entries[plan->count];
  PERF_TARGET *map = targets(plan);
  size_t slot = target_hash(previous, plan->mask);
  while (map[slot].previous != NULL) {
    if (map[slot].previous == previous) return map[slot].next;
    slot = (slot + 1) & plan->mask;
  }
  return NULL;
}

/* Compatibility for a missing plan or a redirect outside the original chain. */
static int32_t run_linked(CSOUND *csound, INSDS *ip, OPDS *op, INSDS **live)
{
  while (op != NULL && ATOMIC_GET8(ip->actflg)) {
    ip->pds = op;
    int32_t error = op->perf ? op->perf(csound, op) : OK;
    if ((live != NULL && *live == NULL) || !ATOMIC_GET8(ip->actflg))
      return NOTOK;
    if (error != OK) return error;
    op = ip->pds ? ip->pds->nxtp : NULL;
  }
  return OK;
}

int32_t csoundRunPerfPlan(CSOUND *csound, INSDS *ip, PERF_PLAN *plan, INSDS **live)
{
  if (!ATOMIC_GET8(ip->actflg)) return NOTOK;
  if (plan == NULL) return run_linked(csound, ip, ip->nxtp, live);
  PERF_ENTRY *entry = plan->entries;
  while (entry->op != NULL) {
    if (UNLIKELY(!ATOMIC_GET8(ip->actflg))) return NOTOK;
    OPDS *op = entry->op;
    /* Publish context for every callback, including unknown extensions.
       Read perf live: both init and perf callbacks can replace it. */
    ip->pds = op;
    SUBR fn = op->perf;
    int32_t error = fn ? fn(csound, op) : OK;
    /* Deactivation can reset plan storage and clear a UDO owner slot. */
    if ((live != NULL && *live == NULL) || !ATOMIC_GET8(ip->actflg))
      return NOTOK;
    if (error != OK) return error;
    OPDS *previous = ip->pds;
    if (LIKELY(previous == op)) entry++;
    else {
      /* Each predecessor's destination was bound during initialization.
         No callback identity checks, casts, or per-branch linear search. */
      entry = find_target(plan, previous);
      if (entry == NULL)
        return run_linked(csound, ip, previous ? previous->nxtp : NULL, live);
    }
  }
  return OK;
}
