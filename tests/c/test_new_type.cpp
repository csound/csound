/*
 * File:   test_new_type.cpp
 * Author: vlazzarini
 *
 * Created on Nov 20, 2024
 */

#include <stdio.h>
#include <stdlib.h>
#include "gtest/gtest.h"
#include "csound.h"
#include "csdl.h"
#include "csound_type_system.h"


class NewTypeTests : public ::testing::Test {
public:
    NewTypeTests ()
    {
    }

    virtual ~NewTypeTests ()
    {
    }

    virtual void SetUp ()
    {
        csound = csoundCreate (NULL,NULL);
        csoundSetOption(csound, "-odac");
    }

    virtual void TearDown ()
    {
        csoundDestroy (csound);
        csound = nullptr;
    }

    CSOUND* csound {nullptr};
};

typedef struct Tuple {
  MYFLT *data;
  size_t size;
} TUPLE;

void varInitMemory(CSOUND *csound, CS_VARIABLE* var, MYFLT* memblock) {
  memset(memblock, 0, var->memBlockSize);
}

void tupleCopyValue(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                        const void* src, INSDS *ctx) {
  memcpy(dest, src, sizeof(Tuple));
}

CS_VARIABLE* createTuple(void* cs, void* p, INSDS *ctx) {
    CSOUND* csound = (CSOUND*) cs;
    CS_VARIABLE* var = (CS_VARIABLE *)
      csound->Calloc(csound, sizeof(CS_VARIABLE));
    IGN(p);
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof(Tuple));
    var->initializeVariableMemory = &varInitMemory;
    var->ctx = ctx;
    return var;
}

CS_TYPE CS_VAR_TYPE_TUPLE = {
   (char *) "Tuple", (char *) "Tuple", CS_ARG_TYPE_BOTH,
    createTuple, tupleCopyValue,
   NULL, NULL, 0
};


int32_t addTuple(CSOUND *csound) {
  return csoundAddVariableType(csound, csoundGetTypePool(csound),
                               &CS_VAR_TYPE_TUPLE);                           
}

struct TUPINIT {
  OPDS h;
  TUPLE *r;
  MYFLT *in[VARGMAX];
};

int32_t tuple_init(CSOUND *csound, TUPINIT *p) {
  int32_t n = p->INOCOUNT;
  if(p->r->data == NULL)  
    p->r->data = (MYFLT *)
      csound->Calloc(csound, sizeof(MYFLT)*n);
  else if(p->r->size < n)
    p->r->data = (MYFLT *)
      csound->ReAlloc(csound, p->r->data, sizeof(MYFLT)*n);
  p->r->size = n;
  for(n = 0; n < p->r->size; n++)
    p->r->data[n] = *p->in[n];
  return OK;
}

struct TUPGET {
  OPDS h;
  MYFLT *r;
  TUPLE *tuple;
  MYFLT *ndx;
};

int32_t tuple_get(CSOUND *csound, TUPGET *p) {
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

TEST_F (NewTypeTests, testAddOpcodeC)
{
  int32_t result;
  const char* instrument =
        "0dbfs = 1\n"
        "instr 1 \n"
        "data:Tuple init p4, p5 \n"
        "a1 oscili get(data,0), get(data,1) \n"
        "out linen(a1,0.1,p3,0.1) \n"
        "endin \n";

  addTuple(csound);
  
  csoundAppendOpcode(csound, "init", sizeof(TUPINIT), 0,
                     ":Tuple;", "m", (SUBR) tuple_init, NULL, NULL);
  csoundAppendOpcode(csound, "get", sizeof(TUPGET), 0,
                     "i", ":Tuple;i", (SUBR) tuple_get, NULL, NULL);
  csoundAppendOpcode(csound, "get", sizeof(TUPGET), 0,
                     "k", ":Tuple;k", NULL, (SUBR) tuple_get, NULL);
  
  result = csoundCompileOrc(csound, instrument, 0);
  ASSERT_TRUE (result == 0);
  csoundEventString(csound,  "i 1 0 1 0.5 330 \n", 0);
  result = csoundStart(csound);
  ASSERT_TRUE (result == 0);
  while(!result)
    result = csoundPerformKsmps(csound);
  csoundSleep(500);
}
