char* get_opcode_short_name(CSOUND* csound, char* opname);

PUBLIC OENTRY* find_opcode_new(CSOUND* csound, char* opname,
                               char* outArgsFound, char* inArgsFound);
PUBLIC OENTRY* find_opcode_exact(CSOUND* csound, char* opname,
                               char* outArgsFound, char* inArgsFound);
/* find OENTRY with the specified name in opcode list */

OENTRY* find_opcode(CSOUND *, char *);
