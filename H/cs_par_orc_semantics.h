#ifndef __CSOUND_ORC_SEMANTIC_ANALYSIS_H__
#define __CSOUND_ORC_SEMANTIC_ANALYSIS_H__

/*
 * This module maintains a list of instruments that have been parsed
 * When parsing an instrument:
 *  csp_orc_sa_instr_add
 *          called first to setup instrument (when parsed the instrument
                                              name/number)
 *  csp_orc_sa_global_read_write_add_list
 *          called to add globals to that instruments dependency lists
 *  csp_orc_sa_instr_finalize
 *          called when finished parsing that instrument
 *
 *  csp_orc_sa_instr_get_by_name or by_num
 *          called to fetch an instrument later
 */

/* maintain information about insturments defined */
typedef struct instr_semantics_t {
    char                        hdr[HDR_LEN];
    char                        *name;
    int32                       insno;
    int32                       sanitized;
    struct set_t                *read;
    struct set_t                *write;
    struct set_t                *read_write;
    uint32_t                    weight;
    struct instr_semantics_t    *next;
} INSTR_SEMANTICS;

void csp_orc_sa_cleanup(CSOUND *csound);
void csp_orc_sa_print_list(CSOUND *csound);

/* maintain state about the current instrument we are parsing */
/* add a new instrument */
void csp_orc_sa_instr_add(CSOUND *csound, char *name);
void csp_orc_sa_instr_add_tree(CSOUND *csound, TREE *x);
/* finish the current instrument */
#define csp_orc_sa_instr_finalize(csound) \
  { csound->instCurr = NULL; csound->inInstr = 0; }

/* add the globals read and written to the current instrument; second case
 * if write and read contain the same global and size of both is 1 then
 * that global is added to the read-write list of the current instrument */
void csp_orc_sa_global_read_write_add_list(CSOUND *csound,
                                           struct set_t *write, struct set_t *read);
void csp_orc_sa_global_read_write_add_list1(CSOUND *csound,
                                           struct set_t *write, struct set_t *read);

/* add to the read and write lists of the current instrument */
void csp_orc_sa_global_write_add_list(CSOUND *csound, struct set_t *list);
void csp_orc_sa_global_read_add_list(CSOUND *csound, struct set_t *list);

/* find all the globals contained in the sub-tree node */
struct set_t *csp_orc_sa_globals_find(CSOUND *csound, TREE *node);

/* find an instrument from the instruments parsed */
struct instr_semantics_t
    *csp_orc_sa_instr_get_by_name(CSOUND *csound, char *instr_name);
struct instr_semantics_t
    *csp_orc_sa_instr_get_by_num(CSOUND *csound, int16 insno);

/* interlocks */
void csp_orc_sa_interlocks(CSOUND *, ORCTOKEN *);
void csp_orc_sa_interlocksf(CSOUND *, int);

void csp_orc_analyze_tree(CSOUND *, TREE*);

#endif /* end of include guard: __CSOUND_ORC_SEMANTIC_ANALYSIS_H__ */
