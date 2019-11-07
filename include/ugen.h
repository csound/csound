/** API Functions for creating instances of Csound Opcodes as 
 * individual unit generators. UGEN's should also be extensible
 * by host languages at runtime.
 *
 * Workflow:
 *
 * - User creates a CSOUND instance
 * - User creates a UGEN_FACTORY
 * - User lists OENTRYs 
 * - User uses OENTRY with UGEN_FACTORY to create UGEN instance.
 * - User connects arguments together using ugen_set_input and ugen_set_output.
 *   This is the process to dynamically create a graph.
 * - User uses graph of UGENs and schedule to run with a CSOUND instance.
 * - User turns off graph.
 *
 * - context: required for things like hold, releasing, etc.
 * */

#include "csoundCore.h"
#include <stdbool.h>

typedef struct {
  CSOUND* csound;
  INSDS* insds;
  OENTRY* oentry;
  void* data;
  int (*init)(void* data);
  int (*perform)(void* data);
  int (*destroy)(void* data);
} UGEN;

typedef struct {
  INSDS* insds;
} UGEN_CONTEXT;

typedef struct {
  CSOUND* csound;
} UGEN_FACTORY;

/** Creates a UGEN_FACTORY, used to list available UGENs (Csound Opcodes),
 * as well as create instances of UGENs. User should configure the CSOUND
 * instance for sr and ksmps before creating a factory. */ 
PUBLIC UGEN_FACTORY* ugen_factory_new(CSOUND* csound);

/* Delete a UGEN_FACTORY */
PUBLIC bool ugen_factory_delete(CSOUND* csound);

PUBLIC UGEN_CONTEXT* ugen_context_new(UGEN_FACTORY* factory);

PUBLIC UGEN_CONTEXT* ugen_context_delete(UGEN_FACTORY* factory);

/** Create a new UGEN, using the given UGEN_FACTORY and OENTRY */
PUBLIC UGEN* ugen_new(UGEN_FACTORY* factory, OENTRY* oentry);


PUBLIC bool ugen_set_output(UGEN* ugen, int index, void* arg);

PUBLIC bool ugen_set_input(UGEN* ugen, int index, void* arg);

PUBLIC int ugen_init(UGEN* ugen);

PUBLIC int ugen_perform(UGEN* ugen);

PUBLIC bool ugen_delete(UGEN* ugen);



