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

#include "ugen.h"

/** Creates a UGEN_FACTORY, used to list available UGENs (Csound Opcodes),
 * as well as create instances of UGENs. User should configure the CSOUND
 * instance for sr and ksmps before creating a factory. */ 
PUBLIC UGEN_FACTORY* ugen_factory_new(CSOUND* csound) {
  UGEN_FACTORY* factory = csound->Calloc(csound, sizeof(UGEN_FACTORY));
  return factory;
}

/* Delete a UGEN_FACTORY */
PUBLIC bool ugen_factory_delete(CSOUND* csound) {
  return false;
}

PUBLIC UGEN_CONTEXT* ugen_context_new(UGEN_FACTORY* factory) {
  return NULL;
}

PUBLIC UGEN_CONTEXT* ugen_context_delete(UGEN_FACTORY* factory) {
  return NULL;
}

/** Create a new UGEN, using the given UGEN_FACTORY and OENTRY */
PUBLIC UGEN* ugen_new(UGEN_FACTORY* factory, OENTRY* oentry) {
  return NULL;
}


PUBLIC bool ugen_set_output(UGEN* ugen, int index, void* arg) {
  return false;
}

PUBLIC bool ugen_set_input(UGEN* ugen, int index, void* arg) {
  return false;
}

PUBLIC int ugen_init(UGEN* ugen) {
  return 0;
}

PUBLIC int ugen_perform(UGEN* ugen) {
  return 0;
}

PUBLIC bool ugen_delete(UGEN* ugen) {
  return true;
}



