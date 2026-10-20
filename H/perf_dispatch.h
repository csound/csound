/* Private performance dispatch shared by instruments, UDOs and subinstr. */
#ifndef CSOUND_PERF_DISPATCH_H
#define CSOUND_PERF_DISPATCH_H

typedef struct perf_entry {
  OPDS *op;
} PERF_ENTRY;

typedef struct perf_target {
  OPDS *previous;
  PERF_ENTRY *next;
} PERF_TARGET;

typedef struct perf_plan {
  size_t count;
  size_t mask;
  PERF_ENTRY entries[]; /* count + 1 entries, then mask + 1 target slots */
} PERF_PLAN;

/* Init-only allocation. Storage follows the existing INSDS lifetime. */
void csoundPreparePerfPlan(CSOUND *, INSDS *);
PERF_PLAN *csoundGetPerfPlan(CSOUND *, INSDS *);
int32_t csoundRunPerfPlan(CSOUND *, INSDS *, PERF_PLAN *, INSDS **live);

/* A target is the predecessor of the next opcode, matching legacy labels.
   Keep pds observable for old callbacks and the linked debugger. The flat
   runner resolves this request through its init-built target map. */
static inline void csoundRequestPerfJump(OPDS *source, OPDS *previous)
{
  source->insdshead->pds = previous;
}
#endif
