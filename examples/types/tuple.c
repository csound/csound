// Plugin type example
// V Lazzarini, Nov 2024
#include <csdl.h>

// New data type: Tuple
typedef struct TUPLE {
  MYFLT data[VARGMAX];
  size_t size;
} TUPLE;

static CS_VARIABLE* create_tuple(void* cs, const void* p, INSDS *ctx);

static void var_init_memory(CSOUND *csound, CS_VARIABLE* var, MYFLT* memblock) {
  memset(memblock, 0, var->memBlockSize);
}

static void tuple_copy_value(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                        const void* src, INSDS *ctx) {
  if(src && dest)
    memcpy(dest, src, sizeof(TUPLE));
}

CS_TYPE CS_VAR_TYPE_TUPLE = {
   "Tuple", "Tuple", CS_ARG_TYPE_BOTH, create_tuple, tuple_copy_value,
    NULL, NULL, 0
};

static CS_VARIABLE* create_tuple(void* cs, const void* p, INSDS *ctx) {
    CSOUND* csound = (CSOUND*) cs;
    CS_VARIABLE* var = (CS_VARIABLE *)
      csound->Calloc(csound, sizeof(CS_VARIABLE));
    IGN(p);
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof(TUPLE));
    var->initializeVariableMemory = &var_init_memory;
    var->ctx = ctx;
    var->varType = &CS_VAR_TYPE_TUPLE;
    return var;
}


static int32_t add_tuple(CSOUND *csound) {
  return csound->AddVariableType(csound, csound->GetTypePool(csound),
                               &CS_VAR_TYPE_TUPLE);                           
}

// Opcodes to operate on Tuples
// Initialise
typedef struct TUPINIT {
  OPDS h;
  TUPLE *r;
  MYFLT *in[VARGMAX];
} TUPINIT;

static int32_t tuple_init(CSOUND *csound, TUPINIT *p) {
  int32_t n;
  p->r->size = p->INCOUNT;
  for(n = 0; n < p->r->size; n++)
    p->r->data[n] = *p->in[n];
  return OK;
}

// Access data
typedef struct TUPGET {
  OPDS h;
  MYFLT *r;
  TUPLE *tuple;
  MYFLT *ndx;
} TUPGET;

static int32_t tuple_get(CSOUND *csound, TUPGET *p) {
  int32_t ndx = (int32_t) *p->ndx;
  if(ndx >= 0 && ndx < p->tuple->size) {
    *p->r = p->tuple->data[ndx];
    return OK;
  }
  else {
    csound->Message(csound, "index out of range\n");
    return NOTOK;
  }
}

// Called on plugin load to create module
PUBLIC int32_t  csoundModuleCreate(CSOUND *csound) {
   return CSOUND_SUCCESS;
}

// Called on plugin load to initialise module
PUBLIC  int32_t  csoundModuleInit(CSOUND *csound){
  // add the new data type to Csound
  add_tuple(csound);
  // add new opcodes
  csound->AppendOpcode(csound, "init", sizeof(TUPINIT), 0,
                     ":Tuple;", "m", (SUBR) tuple_init, NULL, NULL);
  csound->AppendOpcode(csound, "get", sizeof(TUPGET), 0,
                     "i", ":Tuple;i", (SUBR) tuple_get, NULL, NULL);
  csound->AppendOpcode(csound, "get", sizeof(TUPGET), 0,
                     "k", ":Tuple;k", NULL, (SUBR) tuple_get, NULL);
  return CSOUND_SUCCESS;
  
}

PUBLIC  int32_t  csoundModuleDestroy(CSOUND *csound){
  return CSOUND_SUCCESS;
}

PUBLIC int32_t csoundModuleInfo(void){
  return ((CS_VERSION << 16) +
             (CS_SUBVER << 8) + (int32_t) sizeof(MYFLT));
}
