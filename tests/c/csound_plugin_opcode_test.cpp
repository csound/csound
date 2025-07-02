/*
 * File:   csound_plugin_opcode_test.cpp
 * Author: vlazzarini
 *
 * Created on Nov 17, 2024
 */

#include <stdio.h>
#include <stdlib.h>
#include "gtest/gtest.h"
#include "csound.h"
#include "csdl.h"


class PluginTests : public ::testing::Test {
public:
    PluginTests ()
    {
    }

    virtual ~PluginTests ()
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

struct OPC {
  OPDS h;
  MYFLT *r,*a,*b,*c;
  MYFLT ival;
};

int32_t opcode_init(CSOUND *csound, OPC *p) {
  p->ival = *p->c;
  return OK;
}

int32_t opcode_perf(CSOUND *csound, OPC *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT *out = p->r, *in = p->a, scal = *p->b, offs = p->ival;

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }
  for(n = offset; n < nsmps;  n++) 
    out[n] = in[n]*scal + offs;
  
  return OK;
}

int32_t opcode_deinit(CSOUND *csound, OPC *p) {
  p->ival = (MYFLT) 0;
  return OK;
}


TEST_F (PluginTests, testAddOpcodeC)
{
  int32_t result;
  const char* instrument =
        "instr 1 \n"
        "a1 oscili p4, p5 \n"
        "a2 opcode_test a1, 0dbfs, 0  \n"
        "out  linen(a2,0.1,p3,0.1)   \n"
        "endin \n";
    
  csoundAppendOpcode(csound, "opcode_test", sizeof(OPC), 0,
                     "a", "aki", (SUBR) opcode_init, (SUBR) opcode_perf,
                     (SUBR) opcode_deinit);
  result = csoundCompileOrc(csound, instrument, 0);
  ASSERT_TRUE (result == 0);
  csoundEventString(csound,  "i 1 0 1 0.5 440 \n", 0);
  result = csoundStart(csound);
  ASSERT_TRUE (result == 0);
  while(!result)
    result = csoundPerformKsmps(csound);
  csoundSleep(500);
}

#include "plugin.h"

/** i-time plugin opcode example
    with 1 output and 1 input \n
    iout simple iin
 */
struct Simplei : csnd::Plugin<1, 1> {
  int init() {
    outargs[0] = inargs[0];
    return OK;
  }
};

/** k-rate plugin opcode example
    with 1 output and 1 input \n
    kout simple kin
 */
struct Simplek : csnd::Plugin<1, 1> {
  int kperf() {
    outargs[0] = inargs[0];
    return OK;
  }
};

/** a-rate plugin opcode example
    with 1 output and 1 input \n
    aout simple ain
 */
struct Simplea : csnd::Plugin<1, 1> {
  int aperf() {
    std::copy(inargs(0) + offset, inargs(0) + nsmps, outargs(0));
    return OK;
  }
};

namespace csnd {
void on_load(Csound *csound) {
  csnd::plugin<Simplei>(csound, "simple", "i", "i", csnd::thread::i);
  csnd::plugin<Simplek>(csound, "simple", "k", "k", csnd::thread::k);
  csnd::plugin<Simplea>(csound, "simple", "a", "a", csnd::thread::a);
}
}

TEST_F (PluginTests, testAddOpcodeCpp)
{
  int32_t result;
  const char* instrument =
        "instr 1 \n"
        "i1 simple 0 \n"
        "k2 simple 0dbfs \n"
        "a2 simple oscili(p4, p5) \n"
        "out  linen(a2*k2 + i1, 0.1,p3,0.1)  \n"
        "endin \n";
    
  csnd::on_load((csnd::Csound *)csound);
  result = csoundCompileOrc(csound, instrument, 0);
  ASSERT_TRUE (result == 0);
  csoundEventString(csound,  "i 1 0 1 0.5 550 \n", 0);
  result = csoundStart(csound);
  ASSERT_TRUE (result == 0);
  while(!result)
    result = csoundPerformKsmps(csound);
  csoundSleep(500);
}
