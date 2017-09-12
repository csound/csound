/** API Functions for creating instances of Csound Opcodes as 
 * individual unit generators. UGEN's should also be extensible
 * by host languages at runtime. */

typedef struct {
  CSOUND* csound;
  INSDS* insds;
  OENTRY* oentry;
  void* data;
  int (*init)(void* data);
  int (*perform)(void* data);
  int (*destroy)(void* data);
} UGEN;

UGEN* ugen_new(CSOUND*, OENTRY*); 
bool ugen_set_output(UGEN*, int index, void* arg);
bool ugen_set_input(UGEN*, int index, void* arg);

bool ugen_delete(UGEN*);
