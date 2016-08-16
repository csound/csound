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

extern OENTRIES* find_opcode2(CSOUND* csound, char* opname);
extern char** splitArgs(CSOUND* csound, char* argString);

// this value is chosen arbitrarily, feel free to modify
static const int MAX_VAR_ARGS = 8;


/** Creates a UGEN_FACTORY, used to list available UGENs (Csound Opcodes),
 * as well as create instances of UGENs. User should configure the CSOUND
 * instance for sr and ksmps before creating a factory. */ 
PUBLIC UGEN_FACTORY* ugen_factory_new(CSOUND* csound) {
  UGEN_FACTORY* factory = csound->Calloc(csound, sizeof(UGEN_FACTORY));
  INSDS* insds = csound->Calloc(csound, sizeof(INSDS));

  factory->csound = csound;
  factory->insds = insds;

  /* Inherit values from CSOUND */
  insds->ksmps = csound->ksmps;
  insds->kcounter = csound->kcounter;
  insds->ekr = csound->ekr;
  insds->onedkr = csound->onedkr;
  insds->onedksmps = csound->onedksmps;
  insds->kicvt = csound->kicvt;

  return factory;
}

/* Delete a UGEN_FACTORY */
PUBLIC bool ugen_factory_delete(CSOUND* csound, UGEN_FACTORY* factory) {
  csound->Free(csound, factory);
  return true;
}

/*
 PUBLIC UGEN_CONTEXT* ugen_context_new(UGEN_FACTORY* factory) {
  return NULL;
}

PUBLIC UGEN_CONTEXT* ugen_context_delete(UGEN_FACTORY* factory) {
  return NULL;
}

*/


OENTRY* ugen_resolve_opcode(OENTRIES* entries, char* outargTypes, char* inargTypes) {
    int i;
    
    for (i = 0; i < entries->count; i++) {
        OENTRY* temp = entries->entries[i];
        
        if (strcmp(outargTypes, temp->outypes) == 0 &&
            strcmp(inargTypes, temp->intypes) == 0) {
            return temp;
        }
    }
   
    return NULL;
}

/** Create a new UGEN, using the given UGEN_FACTORY and OENTRY */
PUBLIC UGEN* ugen_new(UGEN_FACTORY* factory, char* opName, char* outargTypes, char* inargTypes) {
    UGEN* ugen;
    OPDS* opds;
    OPTXT* optxt;
    CSOUND* csound = factory->csound;
    INSDS* insds = factory->insds;
    OENTRIES* entries = find_opcode2(csound, opName);

    if(entries == NULL) {
        return NULL;
    }
    
    OENTRY* entry = ugen_resolve_opcode(entries, outargTypes, inargTypes);
   
    // need to filter here...
    
    if (entry == NULL) {
        return NULL;
    }
   

    //CSOpcode* opcode = new CSOpcode(csound, insds, entry);
    ugen = csound->Calloc(csound, sizeof(UGEN));
    optxt = (OPTXT*)csound->Calloc(csound, sizeof(OPTXT));

    ugen->csound = csound;
    ugen->insds = insds;
    ugen->oentry = entry; 
    ugen->opcodeMem = csound->Calloc(csound, sizeof(entry->dsblksiz)); 

    opds = ugen->opcodeMem;
    opds->insdshead = insds;
    opds->iopadr = entry->iopadr;
    opds->opadr = entry->kopadr;
    opds->optext = optxt;

    
    /*vector<const CS_TYPE*> inTypes = get_assignable_in_types(oentry->intypes);*/
    /*vector<const CS_TYPE*> outTypes = get_assignable_out_types(oentry->outypes);*/
    
    /*this->outPool = (CS_VAR_POOL*)csound->Calloc(csound, sizeof(CS_VAR_POOL));*/
    /*this->inPool = (CS_VAR_POOL*)csound->Calloc(csound, sizeof(CS_VAR_POOL));*/
    /*this->inPoolCount = inTypes.size();*/
    /*this->outPoolCount = outTypes.size();*/

    /*optxt->t.outArgCount = outTypes.size();*/
    /*optxt->t.inArgCount = inTypes.size();*/
    
    /*char name[10];*/
    /*for(int i = 0; i < outTypes.size(); i++) {*/
        /*sprintf(name, "out%d", i);*/
        /*CS_VARIABLE* var = csoundCreateVariable(csound, csound->typePool, (CS_TYPE*)outTypes[i], name, NULL);*/
        /*csoundAddVariable(outPool, var);*/
    /*}*/
    /*for(int i = 0; i < inTypes.size(); i++) {*/
        
        /*if(inTypes[i] == &CS_VAR_ARG_TYPE_A) {*/
            /*inPoolCount += MAX_VAR_ARGS - 1;*/
            /*for (int j = 0; j < MAX_VAR_ARGS; j++) {*/
                /*sprintf(name, "in%d", i + j);*/
                /*CS_VARIABLE* var = csoundCreateVariable(csound, csound->typePool,*/
                                                        /*(CS_TYPE*)&CS_VAR_TYPE_A, name, NULL);*/
                /*csoundAddVariable(inPool, var);*/
            /*}*/
        /*} else if(inTypes[i] == &CS_VAR_ARG_TYPE_K) {*/
            /*inPoolCount += MAX_VAR_ARGS - 1;*/
            /*for (int j = 0; j < MAX_VAR_ARGS; j++) {*/
                /*sprintf(name, "in%d", i + j);*/
                /*CS_VARIABLE* var = csoundCreateVariable(csound, csound->typePool,*/
                                                        /*(CS_TYPE*)&CS_VAR_TYPE_K, name, NULL);*/
                /*csoundAddVariable(inPool, var);*/
            /*}*/
        /*} else {*/
            /*sprintf(name, "in%d", i);*/
            /*CS_VARIABLE* var = csoundCreateVariable(csound, csound->typePool, (CS_TYPE*)inTypes[i], name, NULL);*/
            /*csoundAddVariable(inPool, var);*/
        /*}*/
    /*}*/
    
    /*recalculateVarPoolMemory(csound, inPool);*/
    /*recalculateVarPoolMemory(csound, outPool);*/
   
    /*data = (MYFLT*)csound->Calloc(csound, outPool->poolSize + inPool->poolSize);*/
    
/*//    VCO2* vco2 = (VCO2*)this->opcodeMem;*/
/*//    data[0] = 1.0;*/
/*//    data[1] = 2.0;*/
/*//    data[2] = 3.0;*/
    
    /*MYFLT* temp = (MYFLT*)this->opcodeMem +(sizeof(OPDS) / sizeof(MYFLT));*/
    /*MYFLT** p = (MYFLT**) temp;*/
    /*int outOffset = outPool->poolSize / sizeof(MYFLT);*/
    /*int count = 0;*/
    /*CS_VARIABLE* var = outPool->head;*/
    
    /*while(var != NULL) {*/
        /*p[count] = data + var->memBlockIndex; //curMemBlockLocation;*/
/*//        curMemBlockLocation += 1;*/
        /*count++;*/
        /*var = var->next;*/
    /*}*/
  
    /*var = inPool->head;*/
    
    /*while(var != NULL) {*/
        /*p[count] = data + outOffset + var->memBlockIndex; //curMemBlockLocation;*/
/*//        curMemBlockLocation += 1;*/
        /*count++;*/
        /*var = var->next;*/
    /*}*/
    
    return ugen;
}


PUBLIC bool ugen_set_output(UGEN* ugen, int index, void* arg) {
  return false;
}

PUBLIC bool ugen_set_input(UGEN* ugen, int index, void* arg) {
  return false;
}

PUBLIC int ugen_init(UGEN* ugen) {
  OPDS* opds = (OPDS*)ugen->opcodeMem;
  OENTRY* oentry = ugen->oentry;
  opds->optext->t.inArgCount = ugen->inocount;
  if (oentry->iopadr != NULL) {
      return (*oentry->iopadr)(ugen->csound, ugen->opcodeMem);
  }
  return CSOUND_SUCCESS;
}

PUBLIC int ugen_perform(UGEN* ugen) {
  return 0;
}

PUBLIC bool ugen_delete(UGEN* ugen) {
  return true;
}



