/* Layout contracts from develop, before orchestra modules.
   Test-only copies keep ABI checks independent of the current declarations. */
  typedef struct legacy_typepool {
    CS_TYPE_ITEM* head;
  } LEGACY_TYPE_POOL;
  typedef struct legacy_csvarpool {
    CS_HASH_TABLE* table;
    CS_VARIABLE* head;
    CS_VARIABLE* tail;
    int32_t poolSize;
    struct legacy_csvarpool* parent;
    int32_t varCount;
    int32_t synthArgCount;
  } LEGACY_CS_VAR_POOL;
typedef struct legacy_instr {
  struct op *nxtop;            /* Linked list of instr opcodes */
  TEXT t;                      /* Text of instrument (same in nxtop) */
  int32_t pmax, vmax, pextrab; /* Arg count, size of data for all
                                  opcodes in instr */
  CS_VAR_POOL *varPool;
  int16 muted;
  int32 opdstot;              /* Total size of opds structs in instr */
  cs_float *psetdata;            /* Used for pset opcode */
  struct insds *instance;     /* Chain of allocated instances of
                                 this instrument */
  struct insds *lst_instance; /* last allocated instance */
  struct insds *act_instance; /* Chain of free (inactive) instances */
  /* (pointer to next one is INSDS.nxtact) */
  struct instr *nxtinstxt; /* Next instrument in orch (num order) */
  int32_t active;          /* To count activations for control */
  int32_t pending_release; /* To count instruments in release phase */
  int32_t maxalloc;
  int32_t turnoff_mode; /* Opt turnoff instruments instances above maxalloc */
  cs_float cpuload;    /* % load this instrumemnt makes */
  struct opcodinfo *opcode_info; /* UDO info (when instrs are UDOs) */
  char *insname;                 /* instrument name */
  int32_t instcnt;               /* Count number of instances ever */
  int32_t isNew;                 /* is this a new definition */
  int32_t nocheckpcnt;           /* Control checks on pcnt */
  int32_t glbvarcnt;             /* global audio var count */
} LEGACY_INSTRTXT;
