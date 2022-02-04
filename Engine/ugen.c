 /*
    ugen.c:

    Copyright (C) 2021
    Steven Yi

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

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
#include "csound_standard_types.h"
#include "csound_orc.h"

extern OENTRIES* find_opcode2(CSOUND* csound, char* opname);
extern char** splitArgs(CSOUND* csound, char* argString);

// this value is chosen arbitrarily, feel free to modify
//static const int MAX_VAR_ARGS = 8;

typedef struct {
  const CS_TYPE* type;
  bool varArg;
} UGEN_ARG;

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


static CONS_CELL* get_assignable_in_types(CSOUND* csound, char* intypes) {
    CONS_CELL* current = NULL;
    const CS_TYPE* varType = NULL;
    char *temp = intypes;
    
    while (*temp != 0) {
        char c = *temp;
        UGEN_ARG* arg = csound->Calloc(csound, sizeof(UGEN_ARG));
       
        // if var-arg found, break and complete
        if (strchr("My", c)) {
          arg->type = &CS_VAR_TYPE_A;
          arg->varArg = true;

          current = cs_cons(csound, arg, current); 
          break;
        } else if(strchr("mnz", c)) {
          arg->type = &CS_VAR_TYPE_K;
          arg->varArg = true;

          current = cs_cons(csound, arg, current); 
          break;

        } else {
        
          if (strchr("opqvjh", c) != NULL) {
              c = 'i';
          } else if (strchr("OJVP", c) != NULL) {
              c = 'k';
          } else if (strchr("M", c) != NULL) {
              c = 'a';
          }
         
          switch (c) {
              case 'i':
                  varType = &CS_VAR_TYPE_I;
                  break;
                  
              case 'k':
                  varType = &CS_VAR_TYPE_K;
                  break;
                  
              case 'a':
                  varType = &CS_VAR_TYPE_A;
                  break;
                  
              default:
                  varType = NULL;
          }
        }

        arg->type = varType;
        arg->varArg = false;

        current = cs_cons(csound, arg, current); 

        temp++;
    }
    
    return current;
}



static CONS_CELL* get_assignable_out_types(CSOUND* csound, char* intypes) {
    CONS_CELL* current = NULL;
    const CS_TYPE* varType = NULL;
    char *temp = intypes;
    
    while (*temp != 0) {
        char c = *temp;
        UGEN_ARG* arg = csound->Calloc(csound, sizeof(UGEN_ARG));

        //        if (strchr("p", c) != NULL) {
        //            c = 'i';
        //        } else if (strchr("OJVP", c) != NULL) {
        //            c = 'k';
        //        } else if (strchr("s", c) != NULL) {
        if (strchr("s", c) != NULL) {
            c = 'a';
        }
        
        switch (c) {
            case 'i':
                varType = &CS_VAR_TYPE_I;
                break;
                
            case 'k':
                varType = &CS_VAR_TYPE_K;
                break;
                
            case 'a':
                varType = &CS_VAR_TYPE_A;
                break;
                
            default:
                varType = NULL;
        }

        arg->type = varType;
        arg->varArg = false;

        current = cs_cons(csound, arg, current); 

        temp++;
    }
    
    return current;
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
    
    OENTRY* oentry = ugen_resolve_opcode(entries, outargTypes, inargTypes);
   
    // need to filter here...
    
    if (oentry == NULL) {
        return NULL;
    }
   

    //CSOpcode* opcode = new CSOpcode(csound, insds, entry);
    ugen = csound->Calloc(csound, sizeof(UGEN));
    optxt = (OPTXT*)csound->Calloc(csound, sizeof(OPTXT));

    ugen->csound = csound;
    ugen->insds = insds;
    ugen->oentry = oentry; 
    ugen->opcodeMem = csound->Calloc(csound, sizeof(oentry->dsblksiz)); 

    opds = ugen->opcodeMem;
    opds->insdshead = insds;
    opds->iopadr = oentry->iopadr;
    opds->opadr = oentry->kopadr;
    opds->optext = optxt;

    
    CONS_CELL* inTypes = get_assignable_in_types(csound, oentry->intypes);
    CONS_CELL* outTypes = get_assignable_out_types(csound, oentry->outypes);
    
    ugen->outPool = (CS_VAR_POOL*)csound->Calloc(csound, sizeof(CS_VAR_POOL));
    ugen->inPool = (CS_VAR_POOL*)csound->Calloc(csound, sizeof(CS_VAR_POOL));
    ugen->inPoolCount = cs_cons_length(inTypes);
    ugen->outPoolCount = cs_cons_length(outTypes);

    optxt->t.outArgCount = ugen->outPoolCount;
    optxt->t.inArgCount = ugen->inPoolCount;
    
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
    
    recalculateVarPoolMemory(csound, ugen->inPool);
    recalculateVarPoolMemory(csound, ugen->outPool);
  
    // FIXME - this needs to be adjusted for CS_VAR and
    // CS_VAR_TYPE's
    ugen->data = (MYFLT*)csound->Calloc(csound, ugen->outPool->poolSize + ugen->inPool->poolSize);
    
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
    // TODO - check how csound chooses kopadr vs. aopadr
    OENTRY* oentry = ugen->oentry;
    CSOUND* csound = ugen->csound;
    void* opcodeMem = ugen->opcodeMem;
    if((oentry->thread & 2) == 2) {
        if (oentry->kopadr != NULL) {
            return (*oentry->kopadr)(csound, opcodeMem);
        }
    }
    if((oentry->thread & 4) == 4) {
        if (oentry->aopadr != NULL) {
            return (*oentry->aopadr)(csound, opcodeMem);
        }
    }
    
    return CSOUND_SUCCESS;
}

PUBLIC bool ugen_delete(UGEN* ugen) {
  CSOUND* csound = ugen->csound;
  csound->Free(csound, ugen->opcodeMem);
  csound->Free(csound, ugen->outPool);
  csound->Free(csound, ugen->inPool);
  csound->Free(csound, ugen->data);
  csound->Free(csound, ugen);
  return true;
}



