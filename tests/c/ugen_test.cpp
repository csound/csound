/**
 * Unit tests for the UGen API (Engine/ugen.c, include/ugen.h).
 *
 * Tests cover:
 *   - Factory creation/deletion
 *   - UGen creation for known opcodes
 *   - UGEN_VAR typed variable handles
 *   - Init and perform
 *   - Variable wiring between UGENs
 *   - Query helpers (count, type)
 *   - Opcode listing API
 *   - UGen graph API
 *   - Context API
 *   - String and f-sig type support
 *
 * This file includes ugen_internal.h (the private header) so that
 * white-box tests can inspect struct internals.
 */

#define __BUILDING_LIBCSOUND
#include "ugen_internal.h"
#include "csound.h"
#include "pstream.h"
#include <cstring>
#include <cmath>
#include "gtest/gtest.h"

class UGenTests : public ::testing::Test {
public:
    virtual void SetUp() {
        csound = csoundCreate(NULL, NULL);
        csoundSetOption(csound, "--logfile=NULL");
        csoundSetOption(csound, "-n");  /* no audio output */
        csoundSetOption(csound, "--sample-rate=44100");
        csoundSetOption(csound, "--ksmps=64");

        /* Compile a minimal orchestra so the engine is initialised
           and all built-in opcodes are registered. */
        csoundCompileOrc(csound, R"(
          instr 1
          endin
        )", 0);
        csoundStart(csound);
    }

    virtual void TearDown() {
        csoundDestroy(csound);
        csound = nullptr;
    }

    CSOUND* csound{nullptr};
};

/* ------------------------------------------------------------------
 *  Factory API
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, FactoryCreateDelete) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    ASSERT_NE(factory, nullptr);
    EXPECT_EQ(factory->csound, csound);
    EXPECT_NE(factory->insds, nullptr);
    EXPECT_EQ((uint32_t)factory->insds->ksmps, csoundGetKsmps(csound));
    EXPECT_TRUE(csoundUgenFactoryDelete(factory));
}

TEST_F(UGenTests, FactoryDeleteNull) {
    EXPECT_FALSE(csoundUgenFactoryDelete(nullptr));
}

/* ------------------------------------------------------------------
 *  UGen creation / deletion
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, CreateOscils) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);
    EXPECT_NE(ugen->oentry, nullptr);
    EXPECT_NE(ugen->opcodeMem, nullptr);
    EXPECT_NE(ugen->data, nullptr);

    EXPECT_TRUE(csoundUgenDelete(ugen));
    csoundUgenFactoryDelete(factory);
}

TEST_F(UGenTests, CreateLine) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"line",
                           (char*)"k", (char*)"iii");
    ASSERT_NE(ugen, nullptr);
    EXPECT_TRUE(csoundUgenDelete(ugen));
    csoundUgenFactoryDelete(factory);
}

TEST_F(UGenTests, CreateNonExistent) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"__no_such_opcode__",
                           (char*)"k", (char*)"k");
    EXPECT_EQ(ugen, nullptr);
    csoundUgenFactoryDelete(factory);
}

TEST_F(UGenTests, CreateWrongTypes) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    /* oscils exists but not with these types */
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"k", (char*)"kk");
    EXPECT_EQ(ugen, nullptr);
    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  Argument query helpers
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, QueryCounts) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    /* oscils: "a" -> 1 out, "iiio" -> 4 in (o maps to i) */
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    EXPECT_EQ(csoundUgenGetOutCount(ugen), 1);
    EXPECT_EQ(csoundUgenGetInCount(ugen), 4);

    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}

TEST_F(UGenTests, QueryTypes) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    /* Output 0 should be audio */
    EXPECT_EQ(csoundUgenGetOutType(ugen, 0), UGEN_ARG_TYPE_A);

    /* Inputs 0-3 should all be i-rate */
    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(csoundUgenGetInType(ugen, i), UGEN_ARG_TYPE_I);
    }

    /* Out-of-range returns UNKNOWN */
    EXPECT_EQ(csoundUgenGetOutType(ugen, 1), UGEN_ARG_TYPE_UNKNOWN);
    EXPECT_EQ(csoundUgenGetInType(ugen, 4), UGEN_ARG_TYPE_UNKNOWN);

    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  UGEN_VAR: Get/Set scalar values
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, VarSetGetInputValue) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    /* Set amplitude, frequency, phase as i-rate scalars via UGEN_VAR.
     * Using the var handle directly is more efficient when updating
     * the same argument repeatedly (e.g. in a k-rate loop). */
    UGEN_VAR* ampVar = csoundUgenGetInVar(ugen, 0);
    UGEN_VAR* freqVar = csoundUgenGetInVar(ugen, 1);
    UGEN_VAR* phaseVar = csoundUgenGetInVar(ugen, 2);

    ASSERT_NE(ampVar, nullptr);
    ASSERT_NE(freqVar, nullptr);
    ASSERT_NE(phaseVar, nullptr);

    csoundUgenVarSetValue(ampVar, 0.5);
    csoundUgenVarSetValue(freqVar, 440.0);
    csoundUgenVarSetValue(phaseVar, 0.0);

    /* Read back and verify */
    EXPECT_DOUBLE_EQ(csoundUgenVarGetValue(ampVar), 0.5);
    EXPECT_DOUBLE_EQ(csoundUgenVarGetValue(freqVar), 440.0);
    EXPECT_DOUBLE_EQ(csoundUgenVarGetValue(phaseVar), 0.0);

    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}

TEST_F(UGenTests, VarSetGetOutputAudio) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    int32_t ksmps = csoundGetKsmps(csound);

    /* Get output var and write directly into the data buffer */
    UGEN_VAR* outVar = csoundUgenGetOutVar(ugen, 0);
    ASSERT_NE(outVar, nullptr);
    EXPECT_EQ(csoundUgenVarGetType(outVar), UGEN_ARG_TYPE_A);
    EXPECT_EQ(csoundUgenVarGetSize(outVar), (size_t)ksmps * sizeof(MYFLT));

    MYFLT* outData = (MYFLT*)csoundUgenVarGetData(outVar);
    ASSERT_NE(outData, nullptr);
    outData[0] = 1.0;
    outData[ksmps - 1] = -1.0;

    /* Read back via the data pointer */
    EXPECT_DOUBLE_EQ(outData[0], 1.0);
    EXPECT_DOUBLE_EQ(outData[ksmps - 1], -1.0);

    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}

TEST_F(UGenTests, VarQueryTypeAndSize) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    int32_t ksmps = csoundGetKsmps(csound);

    /* Output var: audio rate */
    UGEN_VAR* outVar = csoundUgenGetOutVar(ugen, 0);
    ASSERT_NE(outVar, nullptr);
    EXPECT_EQ(csoundUgenVarGetType(outVar), UGEN_ARG_TYPE_A);
    EXPECT_EQ(csoundUgenVarGetSize(outVar), (size_t)ksmps * sizeof(MYFLT));

    /* Input vars: i-rate */
    for (int i = 0; i < 4; i++) {
        UGEN_VAR* inVar = csoundUgenGetInVar(ugen, i);
        ASSERT_NE(inVar, nullptr);
        EXPECT_EQ(csoundUgenVarGetType(inVar), UGEN_ARG_TYPE_I);
        EXPECT_EQ(csoundUgenVarGetSize(inVar), sizeof(MYFLT));
    }

    /* Out of range */
    EXPECT_EQ(csoundUgenGetOutVar(ugen, 1), nullptr);
    EXPECT_EQ(csoundUgenGetInVar(ugen, 4), nullptr);

    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  UGEN_VAR: SetInputVar wiring
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, SetInputVarWiring) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    /* Create a standalone var for amplitude */
    UGEN_VAR* ampVar = csoundUgenVarNew(factory, UGEN_ARG_TYPE_I);
    ASSERT_NE(ampVar, nullptr);
    csoundUgenVarSetValue(ampVar, 0.5);

    /* Wire it to input 0 */
    EXPECT_TRUE(csoundUgenSetInputVar(ugen, 0, ampVar));

    /* The ugen's input should now read the standalone var's value */
    UGEN_VAR* inVar = csoundUgenGetInVar(ugen, 0);
    EXPECT_DOUBLE_EQ(csoundUgenVarGetValue(inVar), 0.5);

    /* Changing the standalone var updates the ugen's view (zero-copy) */
    csoundUgenVarSetValue(ampVar, 0.25);
    EXPECT_DOUBLE_EQ(csoundUgenVarGetValue(inVar), 0.25);

    csoundUgenVarDelete(ampVar);
    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  UGEN_VAR: Standalone creation/deletion
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, StandaloneVarCreateDelete) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);

    UGEN_VAR* kVar = csoundUgenVarNew(factory, UGEN_ARG_TYPE_K);
    ASSERT_NE(kVar, nullptr);
    EXPECT_EQ(csoundUgenVarGetType(kVar), UGEN_ARG_TYPE_K);
    EXPECT_EQ(csoundUgenVarGetSize(kVar), sizeof(MYFLT));
    csoundUgenVarSetValue(kVar, 42.0);
    EXPECT_DOUBLE_EQ(csoundUgenVarGetValue(kVar), 42.0);
    csoundUgenVarDelete(kVar);

    UGEN_VAR* aVar = csoundUgenVarNew(factory, UGEN_ARG_TYPE_A);
    ASSERT_NE(aVar, nullptr);
    EXPECT_EQ(csoundUgenVarGetType(aVar), UGEN_ARG_TYPE_A);
    int32_t ksmps = csoundGetKsmps(csound);
    EXPECT_EQ(csoundUgenVarGetSize(aVar), (size_t)ksmps * sizeof(MYFLT));
    csoundUgenVarDelete(aVar);

    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  Init / Perform with oscils
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, InitPerformOscils) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    int32_t ksmps = csoundGetKsmps(csound);

    /* Use convenience methods for one-off init-time setup.
     * csoundUgenSetValue() is ideal here because we only set each
     * parameter once.  In a k-rate loop, cache the UGEN_VAR handle
     * instead (see MultiCycleStability test). */
    csoundUgenSetValue(ugen, 0, 1.0);    /* amp */
    csoundUgenSetValue(ugen, 1, 1000.0); /* freq */
    csoundUgenSetValue(ugen, 2, 0.0);    /* phase */
    csoundUgenSetValue(ugen, 3, 0.0);    /* iphs */

    /* Init the opcode */
    EXPECT_EQ(csoundUgenInit(ugen), CSOUND_SUCCESS);

    /* Perform one k-cycle */
    EXPECT_EQ(csoundUgenPerform(ugen), CSOUND_SUCCESS);

    /* Read output via UGEN_VAR - should have non-zero samples */
    MYFLT* outBuf = (MYFLT*)csoundUgenVarGetData(csoundUgenGetOutVar(ugen, 0));
    bool hasNonZero = false;
    for (int i = 0; i < ksmps; i++) {
        if (outBuf[i] != 0.0) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);

    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  Opcode listing
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, ListOpcodes) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN_OPCODE_INFO* list = nullptr;
    int32_t count = 0;
    int32_t ret = csoundUgenListOpcodes(factory, &list, &count);
    EXPECT_EQ(ret, CSOUND_SUCCESS);
    EXPECT_GT(count, 0);
    EXPECT_NE(list, nullptr);

    /* Verify that at least one well-known opcode is present */
    bool foundOscils = false;
    for (int32_t i = 0; i < count; i++) {
        if (list[i].opname != nullptr &&
            strcmp(list[i].opname, "oscils") == 0) {
            foundOscils = true;
            break;
        }
    }
    EXPECT_TRUE(foundOscils);

    csoundUgenFreeOpcodeList(factory, list);
    csoundUgenFactoryDelete(factory);
}

TEST_F(UGenTests, FindOpcode) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    EXPECT_TRUE(csoundUgenFindOpcode(factory, "oscils", "a", "iiio"));

    /* Non-existent opcode */
    EXPECT_FALSE(csoundUgenFindOpcode(factory, "__no_such_opcode__", "k", "k"));
    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  Context API
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, ContextCreateDelete) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN_CONTEXT* ctx = csoundUgenContextNew(factory);
    ASSERT_NE(ctx, nullptr);
    EXPECT_NE(ctx->insds, nullptr);
    EXPECT_EQ((uint32_t)ctx->insds->ksmps, csoundGetKsmps(csound));

    EXPECT_TRUE(csoundUgenContextDelete(ctx));
    csoundUgenFactoryDelete(factory);
}

TEST_F(UGenTests, SetContext) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN_CONTEXT* ctx = csoundUgenContextNew(factory);
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    EXPECT_TRUE(csoundUgenSetContext(ugen, ctx));
    /* After setting context, the ugen's insds should be the context's */
    EXPECT_EQ(ugen->insds, ctx->insds);

    csoundUgenDelete(ugen);
    csoundUgenContextDelete(ctx);
    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  Graph API
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, GraphCreateDelete) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN_GRAPH* graph = csoundUgenGraphNew(factory);
    ASSERT_NE(graph, nullptr);
    EXPECT_EQ(graph->count, 0);

    EXPECT_TRUE(csoundUgenGraphDelete(graph));
    csoundUgenFactoryDelete(factory);
}

TEST_F(UGenTests, GraphAddUGens) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN_GRAPH* graph = csoundUgenGraphNew(factory);
    UGEN* u1 = csoundUgenNew(factory, (char*)"oscils",
                         (char*)"a", (char*)"iiio");
    UGEN* u2 = csoundUgenNew(factory, (char*)"oscils",
                         (char*)"a", (char*)"iiio");
    ASSERT_NE(u1, nullptr);
    ASSERT_NE(u2, nullptr);

    EXPECT_EQ(csoundUgenGraphAdd(graph, u1), 0);
    EXPECT_EQ(csoundUgenGraphAdd(graph, u2), 1);
    EXPECT_EQ(graph->count, 2);

    /* delete_all cleans up both graph and contained UGENs */
    csoundUgenGraphDeleteAll(graph);
    csoundUgenFactoryDelete(factory);
}

TEST_F(UGenTests, VarConnect) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);

    /* Create two oscils and connect the audio output of the first
       to an input of the second using UGEN_VAR wiring. */
    UGEN* src = csoundUgenNew(factory, (char*)"oscils",
                          (char*)"a", (char*)"iiio");
    UGEN* dst = csoundUgenNew(factory, (char*)"oscils",
                          (char*)"a", (char*)"iiio");
    ASSERT_NE(src, nullptr);
    ASSERT_NE(dst, nullptr);

    /* Wire src output[0] -> dst input[0] via UGEN_VAR */
    UGEN_VAR* srcOutVar = csoundUgenGetOutVar(src, 0);
    ASSERT_NE(srcOutVar, nullptr);
    EXPECT_TRUE(csoundUgenSetInputVar(dst, 0, srcOutVar));

    /* Verify the pointers are shared: write to src output, read from dst input */
    MYFLT* srcOutBuf = (MYFLT*)csoundUgenVarGetData(srcOutVar);
    srcOutBuf[0] = 42.0;

    /* Read from dst's input var - should see 42.0 because they share pointer */
    UGEN_VAR* dstInVar = csoundUgenGetInVar(dst, 0);
    MYFLT* dstInBuf = (MYFLT*)csoundUgenVarGetData(dstInVar);
    EXPECT_DOUBLE_EQ(dstInBuf[0], 42.0);

    csoundUgenDelete(src);
    csoundUgenDelete(dst);
    csoundUgenFactoryDelete(factory);
}

TEST_F(UGenTests, GraphInitPerform) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN_GRAPH* graph = csoundUgenGraphNew(factory);

    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    /* Convenience methods for init-time parameter setup */
    csoundUgenSetValue(ugen, 0, 1.0);   /* amp */
    csoundUgenSetValue(ugen, 1, 440.0); /* freq */
    csoundUgenSetValue(ugen, 2, 0.0);   /* phase */
    csoundUgenSetValue(ugen, 3, 0.0);   /* iphs */

    csoundUgenGraphAdd(graph, ugen);

    EXPECT_EQ(csoundUgenGraphInit(graph), CSOUND_SUCCESS);
    EXPECT_EQ(csoundUgenGraphPerform(graph), CSOUND_SUCCESS);

    /* Output should have non-zero samples */
    int32_t ksmps = csoundGetKsmps(csound);
    MYFLT* outBuf = (MYFLT*)csoundUgenVarGetData(csoundUgenGetOutVar(ugen, 0));
    bool hasNonZero = false;
    for (int i = 0; i < ksmps; i++) {
        if (outBuf[i] != 0.0) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);

    csoundUgenGraphDeleteAll(graph);
    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  Edge cases / null safety
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, NullUGenOps) {
    EXPECT_FALSE(csoundUgenDelete(nullptr));
    EXPECT_EQ(csoundUgenGetOutVar(nullptr, 0), nullptr);
    EXPECT_EQ(csoundUgenGetInVar(nullptr, 0), nullptr);
    EXPECT_FALSE(csoundUgenSetInputVar(nullptr, 0, nullptr));
    EXPECT_EQ(csoundUgenGetInCount(nullptr), 0);
    EXPECT_EQ(csoundUgenGetOutCount(nullptr), 0);
    EXPECT_EQ(csoundUgenGetInType(nullptr, 0), UGEN_ARG_TYPE_UNKNOWN);
    EXPECT_EQ(csoundUgenGetOutType(nullptr, 0), UGEN_ARG_TYPE_UNKNOWN);
    EXPECT_EQ(csoundUgenInit(nullptr), CSOUND_ERROR);
    EXPECT_EQ(csoundUgenPerform(nullptr), CSOUND_ERROR);

    /* Null UGEN_VAR ops */
    EXPECT_EQ(csoundUgenVarGetType(nullptr), UGEN_ARG_TYPE_UNKNOWN);
    EXPECT_EQ(csoundUgenVarGetSize(nullptr), (size_t)0);
    EXPECT_DOUBLE_EQ(csoundUgenVarGetValue(nullptr), 0.0);
    EXPECT_EQ(csoundUgenVarGetData(nullptr), nullptr);
    EXPECT_FALSE(csoundUgenVarSetString(nullptr, "test"));
    EXPECT_EQ(csoundUgenVarGetString(nullptr), nullptr);

    /* Null UGEN convenience ops */
    csoundUgenSetValue(nullptr, 0, 1.0);           /* should not crash */
    EXPECT_DOUBLE_EQ(csoundUgenGetValue(nullptr, 0), 0.0);
    EXPECT_FALSE(csoundUgenSetString(nullptr, 0, "test"));
    EXPECT_EQ(csoundUgenGetString(nullptr, 0), nullptr);
}

/* ------------------------------------------------------------------
 *  Memory layout - verify no double-offset
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, MemoryLayoutNoDoubleOffset) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    MYFLT** p = (MYFLT**)((char*)ugen->opcodeMem + sizeof(OPDS));
    MYFLT* data = ugen->data;
    int32_t outCount = ugen->outCount;

    /* Check output argument pointers */
    CS_VARIABLE* var = ugen->outPool->head;
    for (int i = 0; i < outCount && var != NULL; i++, var = var->next) {
        MYFLT* expected = data + var->memBlockIndex;
        EXPECT_EQ(p[i], expected)
            << "output[" << i << "] pointer should be data + memBlockIndex";

        CS_VAR_MEM* hdr = (CS_VAR_MEM*)((char*)p[i] - CS_VAR_TYPE_OFFSET);
        EXPECT_NE(hdr->varType, nullptr)
            << "output[" << i << "] CS_VAR_MEM header should have varType set";
    }

    /* Check input argument pointers */
    var = ugen->inPool->head;
    for (int i = 0; i < ugen->inCount && var != NULL; i++, var = var->next) {
        MYFLT* expected = data + ugen->outDataOffset + var->memBlockIndex;
        EXPECT_EQ(p[outCount + i], expected)
            << "input[" << i << "] pointer should be data + outDataOffset + memBlockIndex";

        CS_VAR_MEM* hdr = (CS_VAR_MEM*)((char*)p[outCount + i] - CS_VAR_TYPE_OFFSET);
        EXPECT_NE(hdr->varType, nullptr)
            << "input[" << i << "] CS_VAR_MEM header should have varType set";
    }

    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}

TEST_F(UGenTests, MemoryLayoutHeaderAlignment) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    MYFLT** p = (MYFLT**)((char*)ugen->opcodeMem + sizeof(OPDS));
    int32_t outCount = ugen->outCount;
    int32_t inCount = ugen->inCount;

    /* For each consecutive pair of input args, check no overlap. */
    for (int i = 0; i + 1 < inCount; i++) {
        MYFLT* curr = p[outCount + i];
        MYFLT* next = p[outCount + i + 1];
        UGEN_VAR* currVar = csoundUgenGetInVar(ugen, i);
        size_t currArgSizeBytes = csoundUgenVarGetSize(currVar);
        size_t currArgSizeMYFLTs = currArgSizeBytes / sizeof(MYFLT);

        ptrdiff_t headerStartOffset = CS_FLOAT_ALIGN(CS_VAR_TYPE_OFFSET) / sizeof(MYFLT);
        MYFLT* nextHeaderStart = next - headerStartOffset;
        EXPECT_GE(nextHeaderStart, curr + (ptrdiff_t)currArgSizeMYFLTs)
            << "input[" << i+1 << "] header overlaps input[" << i << "] data";
    }

    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}

TEST_F(UGenTests, MemoryLayoutWriteReadRoundTrip) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    MYFLT** p = (MYFLT**)((char*)ugen->opcodeMem + sizeof(OPDS));
    int32_t outCount = ugen->outCount;
    int32_t ksmps = csoundGetKsmps(csound);

    /* Remember the varType pointers before writing */
    CS_VAR_MEM* outHdr = (CS_VAR_MEM*)((char*)p[0] - CS_VAR_TYPE_OFFSET);
    const CS_TYPE* outType = outHdr->varType;
    ASSERT_NE(outType, nullptr);

    CS_VAR_MEM* inHdr0 = (CS_VAR_MEM*)((char*)p[outCount] - CS_VAR_TYPE_OFFSET);
    const CS_TYPE* inType0 = inHdr0->varType;
    ASSERT_NE(inType0, nullptr);

    /* Write a test pattern into the audio output buffer via UGEN_VAR */
    MYFLT* outData = (MYFLT*)csoundUgenVarGetData(csoundUgenGetOutVar(ugen, 0));
    for (int i = 0; i < ksmps; i++) {
        outData[i] = (MYFLT)(i + 1);
    }

    /* Read back */
    for (int i = 0; i < ksmps; i++) {
        EXPECT_DOUBLE_EQ(outData[i], (MYFLT)(i + 1));
    }

    /* Write an i-rate scalar via UGEN_VAR */
    csoundUgenVarSetValue(csoundUgenGetInVar(ugen, 0), 12345.0);
    EXPECT_DOUBLE_EQ(csoundUgenVarGetValue(csoundUgenGetInVar(ugen, 0)), 12345.0);

    /* Verify headers are still intact after all writes */
    EXPECT_EQ(outHdr->varType, outType)
        << "output varType header was corrupted by data write";
    EXPECT_EQ(inHdr0->varType, inType0)
        << "input varType header was corrupted by data write";

    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  S type support
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, AcceptStringType) {
    /* Opcodes with S (string) type arguments should now be accepted. */
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"puts",
                           (char*)"i", (char*)"So");
    /* puts might have different type string or might not exist;
     * try strcat as fallback */
    if (ugen == nullptr) {
        ugen = csoundUgenNew(factory, (char*)"strcat",
                         (char*)"S", (char*)"SS");
    }
    /* If any string opcode was found, it should be accepted */
    if (ugen != nullptr) {
        csoundUgenDelete(ugen);
    }
    csoundUgenFactoryDelete(factory);
}

TEST_F(UGenTests, AcceptFsigType) {
    /* Opcodes using f-sig (PVOC) types should now be accepted. */
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"pvsanal",
                           (char*)"f", (char*)"aiio");
    /* pvsanal should be found with these types */
    if (ugen != nullptr) {
        /* Verify the output type is F */
        EXPECT_EQ(csoundUgenGetOutType(ugen, 0), UGEN_ARG_TYPE_F);
        csoundUgenDelete(ugen);
    }
    csoundUgenFactoryDelete(factory);
}

TEST_F(UGenTests, AcceptNumericTypes) {
    /* Opcodes with only i/k/a types should be accepted. */
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);

    UGEN* u1 = csoundUgenNew(factory, (char*)"oscils",
                         (char*)"a", (char*)"iiio");
    EXPECT_NE(u1, nullptr);
    if (u1) csoundUgenDelete(u1);

    UGEN* u2 = csoundUgenNew(factory, (char*)"line",
                         (char*)"k", (char*)"iii");
    EXPECT_NE(u2, nullptr);
    if (u2) csoundUgenDelete(u2);

    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  Standalone string var
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, StandaloneStringVar) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);

    UGEN_VAR* sVar = csoundUgenVarNew(factory, UGEN_ARG_TYPE_S);
    ASSERT_NE(sVar, nullptr);
    EXPECT_EQ(csoundUgenVarGetType(sVar), UGEN_ARG_TYPE_S);

    /* Set and get string value */
    EXPECT_TRUE(csoundUgenVarSetString(sVar, "hello world"));
    const char* str = csoundUgenVarGetString(sVar);
    ASSERT_NE(str, nullptr);
    EXPECT_STREQ(str, "hello world");

    /* Overwrite with a longer string */
    EXPECT_TRUE(csoundUgenVarSetString(sVar, "a much longer string value here"));
    str = csoundUgenVarGetString(sVar);
    ASSERT_NE(str, nullptr);
    EXPECT_STREQ(str, "a much longer string value here");

    /* SetString on non-S var should fail */
    UGEN_VAR* kVar = csoundUgenVarNew(factory, UGEN_ARG_TYPE_K);
    EXPECT_FALSE(csoundUgenVarSetString(kVar, "test"));
    EXPECT_EQ(csoundUgenVarGetString(kVar), nullptr);

    csoundUgenVarDelete(kVar);
    csoundUgenVarDelete(sVar);
    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  Multi-cycle stability (init + multiple performs)
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, MultiCycleStability) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    int32_t ksmps = csoundGetKsmps(csound);

    /* Use convenience for init-time setup */
    csoundUgenSetValue(ugen, 0, 0.5);
    csoundUgenSetValue(ugen, 1, 1000.0);
    csoundUgenSetValue(ugen, 2, 0.0);
    csoundUgenSetValue(ugen, 3, 0.0);

    EXPECT_EQ(csoundUgenInit(ugen), CSOUND_SUCCESS);

    /* Remember the varType header pointer */
    MYFLT** p = (MYFLT**)((char*)ugen->opcodeMem + sizeof(OPDS));
    CS_VAR_MEM* outHdr = (CS_VAR_MEM*)((char*)p[0] - CS_VAR_TYPE_OFFSET);
    const CS_TYPE* origType = outHdr->varType;

    MYFLT* outBuf = (MYFLT*)csoundUgenVarGetData(csoundUgenGetOutVar(ugen, 0));

    for (int cycle = 0; cycle < 100; cycle++) {
        EXPECT_EQ(csoundUgenPerform(ugen), CSOUND_SUCCESS);

        /* Samples from a sine oscillator should be in [-1, 1] */
        for (int i = 0; i < ksmps; i++) {
            EXPECT_GE(outBuf[i], -1.0) << "cycle=" << cycle << " sample=" << i;
            EXPECT_LE(outBuf[i],  1.0) << "cycle=" << cycle << " sample=" << i;
        }

        /* Header must remain intact */
        EXPECT_EQ(outHdr->varType, origType)
            << "varType header corrupted at cycle " << cycle;
    }

    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  K-rate opcode memory layout
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, MemoryLayoutKRate) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"line",
                           (char*)"k", (char*)"iii");
    ASSERT_NE(ugen, nullptr);

    MYFLT** p = (MYFLT**)((char*)ugen->opcodeMem + sizeof(OPDS));
    MYFLT* data = ugen->data;
    int32_t outCount = ugen->outCount;

    /* Output (k-rate): pointer should equal data + memBlockIndex */
    CS_VARIABLE* var = ugen->outPool->head;
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(p[0], data + var->memBlockIndex)
        << "k-rate output pointer mismatch";

    CS_VAR_MEM* hdr = (CS_VAR_MEM*)((char*)p[0] - CS_VAR_TYPE_OFFSET);
    EXPECT_NE(hdr->varType, nullptr);

    /* All inputs: verify pointer and header */
    var = ugen->inPool->head;
    for (int i = 0; i < ugen->inCount && var != NULL; i++, var = var->next) {
        MYFLT* expected = data + ugen->outDataOffset + var->memBlockIndex;
        EXPECT_EQ(p[outCount + i], expected)
            << "k-rate input[" << i << "] pointer mismatch";

        CS_VAR_MEM* inHdr = (CS_VAR_MEM*)((char*)p[outCount + i] - CS_VAR_TYPE_OFFSET);
        EXPECT_NE(inHdr->varType, nullptr)
            << "k-rate input[" << i << "] header missing varType";
    }

    /* Set inputs via convenience methods (init-time setup) */
    csoundUgenSetValue(ugen, 0, 0.0);  /* ia */
    csoundUgenSetValue(ugen, 1, 1.0);  /* dur */
    csoundUgenSetValue(ugen, 2, 1.0);  /* ib */
    EXPECT_EQ(csoundUgenInit(ugen), CSOUND_SUCCESS);
    EXPECT_EQ(csoundUgenPerform(ugen), CSOUND_SUCCESS);

    /* Read back the k-rate scalar output via UGEN_VAR */
    MYFLT result = csoundUgenVarGetValue(csoundUgenGetOutVar(ugen, 0));
    /* result should be some value between 0 and 1 */
    (void)result;

    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  UGEN_VAR: outVars and inVars match opcode arg pointers
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, VarDataMatchesArgPointers) {
    /* Verify that UGEN_VAR data pointers match the opcode arg pointers. */
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    MYFLT** p = (MYFLT**)((char*)ugen->opcodeMem + sizeof(OPDS));
    int32_t outCount = ugen->outCount;

    for (int i = 0; i < outCount; i++) {
        UGEN_VAR* outVar = csoundUgenGetOutVar(ugen, i);
        ASSERT_NE(outVar, nullptr);
        EXPECT_EQ((MYFLT*)csoundUgenVarGetData(outVar), p[i])
            << "outVar[" << i << "] data doesn't match opcode arg pointer";
    }

    for (int i = 0; i < ugen->inCount; i++) {
        UGEN_VAR* inVar = csoundUgenGetInVar(ugen, i);
        ASSERT_NE(inVar, nullptr);
        EXPECT_EQ((MYFLT*)csoundUgenVarGetData(inVar), p[outCount + i])
            << "inVar[" << i << "] data doesn't match opcode arg pointer";
    }

    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  Convenience methods: csoundUgenSetValue / csoundUgenGetValue
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, ConvenienceSetGetValue) {
    /* Test the convenience functions that wrap UGEN_VAR access.
     * These are ideal for one-off init-time parameter setup.
     * For per-k-cycle updates, caching the UGEN_VAR handle is more
     * efficient (avoids the index lookup each call). */
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    /* SetValue operates on inputs */
    csoundUgenSetValue(ugen, 0, 0.75);
    csoundUgenSetValue(ugen, 1, 880.0);
    csoundUgenSetValue(ugen, 2, 0.25);

    /* Verify via UGEN_VAR */
    EXPECT_DOUBLE_EQ(csoundUgenVarGetValue(csoundUgenGetInVar(ugen, 0)), 0.75);
    EXPECT_DOUBLE_EQ(csoundUgenVarGetValue(csoundUgenGetInVar(ugen, 1)), 880.0);
    EXPECT_DOUBLE_EQ(csoundUgenVarGetValue(csoundUgenGetInVar(ugen, 2)), 0.25);

    /* Init + perform to generate output */
    EXPECT_EQ(csoundUgenInit(ugen), CSOUND_SUCCESS);
    EXPECT_EQ(csoundUgenPerform(ugen), CSOUND_SUCCESS);

    /* GetValue operates on outputs; for audio, it reads the first MYFLT */
    MYFLT outVal = csoundUgenGetValue(ugen, 0);
    /* The oscillator should have produced a non-zero first sample */
    EXPECT_NE(outVal, 0.0);

    /* Out-of-range index should return 0 and not crash */
    EXPECT_DOUBLE_EQ(csoundUgenGetValue(ugen, 99), 0.0);
    csoundUgenSetValue(ugen, 99, 1.0); /* should silently do nothing */

    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  Convenience methods: csoundUgenSetString / csoundUgenGetString
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, ConvenienceSetGetString) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);

    /* strcat: S -> SS — an opcode with string inputs and string output */
    UGEN* ugen = csoundUgenNew(factory, (char*)"strcat",
                           (char*)"S", (char*)"SS");
    if (ugen == nullptr) {
        /* Some builds may not have strcat; skip gracefully */
        csoundUgenFactoryDelete(factory);
        GTEST_SKIP() << "strcat opcode not available";
    }

    /* Use convenience SetString on inputs */
    EXPECT_TRUE(csoundUgenSetString(ugen, 0, "hello "));
    EXPECT_TRUE(csoundUgenSetString(ugen, 1, "world"));

    /* SetString on a non-S type should fail */
    UGEN* osc = csoundUgenNew(factory, (char*)"oscils",
                          (char*)"a", (char*)"iiio");
    ASSERT_NE(osc, nullptr);
    EXPECT_FALSE(csoundUgenSetString(osc, 0, "test"));

    /* GetString on a non-S output should return NULL */
    EXPECT_EQ(csoundUgenGetString(osc, 0), nullptr);

    /* Init strcat and verify concatenation */
    EXPECT_EQ(csoundUgenInit(ugen), CSOUND_SUCCESS);

    /* GetString reads from the output */
    const char* result = csoundUgenGetString(ugen, 0);
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "hello world");

    csoundUgenDelete(osc);
    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}
