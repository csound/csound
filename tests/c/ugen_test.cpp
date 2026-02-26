/**
 * Unit tests for the UGen API (Engine/ugen.c, include/ugen.h).
 *
 * Tests cover:
 *   - Factory creation/deletion
 *   - UGen creation for known opcodes
 *   - Argument set/get by value
 *   - Init and perform
 *   - Argument wiring by pointer
 *   - Query helpers (count, type, size)
 *   - Opcode listing API
 *   - UGen graph API
 *   - Context API
 *
 * This file includes ugen_internal.h (the private header) so that
 * white-box tests can inspect struct internals.
 */

#define __BUILDING_LIBCSOUND
#include "ugen_internal.h"
#include "csound.h"
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

TEST_F(UGenTests, QueryArgSizes) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    int32_t ksmps = csoundGetKsmps(csound);

    /* Audio output should be ksmps * sizeof(MYFLT) */
    EXPECT_EQ(csoundUgenGetOutArgSize(ugen, 0),
              (size_t)ksmps * sizeof(MYFLT));

    /* i-rate inputs should be sizeof(MYFLT) */
    EXPECT_EQ(csoundUgenGetInArgSize(ugen, 0), sizeof(MYFLT));

    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  Set / Get by value
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, SetGetInputValue) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    /* Set amplitude, frequency, phase as i-rate scalars */
    MYFLT amp = 0.5;
    MYFLT freq = 440.0;
    MYFLT phase = 0.0;
    EXPECT_TRUE(csoundUgenSetInputValue(ugen, 0, &amp));
    EXPECT_TRUE(csoundUgenSetInputValue(ugen, 1, &freq));
    EXPECT_TRUE(csoundUgenSetInputValue(ugen, 2, &phase));

    /* Read back and verify */
    MYFLT readBack = 0;
    EXPECT_EQ(csoundUgenGetInputValue(ugen, 0, &readBack), sizeof(MYFLT));
    EXPECT_DOUBLE_EQ(readBack, 0.5);

    EXPECT_EQ(csoundUgenGetInputValue(ugen, 1, &readBack), sizeof(MYFLT));
    EXPECT_DOUBLE_EQ(readBack, 440.0);

    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}

TEST_F(UGenTests, SetGetOutputValue) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    /* Write some data into the output buffer */
    int32_t ksmps = csoundGetKsmps(csound);
    size_t outSz = (size_t)ksmps * sizeof(MYFLT);
    MYFLT* buf = (MYFLT*)calloc(ksmps, sizeof(MYFLT));
    buf[0] = 1.0;
    buf[ksmps - 1] = -1.0;

    EXPECT_TRUE(csoundUgenSetOutputValue(ugen, 0, buf));

    /* Read back */
    MYFLT* readBuf = (MYFLT*)calloc(ksmps, sizeof(MYFLT));
    EXPECT_EQ(csoundUgenGetOutputValue(ugen, 0, readBuf), outSz);
    EXPECT_DOUBLE_EQ(readBuf[0], 1.0);
    EXPECT_DOUBLE_EQ(readBuf[ksmps - 1], -1.0);

    free(buf);
    free(readBuf);
    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  Set args by pointer
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, SetInputByPointer) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    MYFLT amp = 0.5;
    EXPECT_TRUE(csoundUgenSetInput(ugen, 0, &amp));

    /* Reading through get_input_value should see the pointed-to value */
    MYFLT readBack = 0;
    csoundUgenGetInputValue(ugen, 0, &readBack);
    EXPECT_DOUBLE_EQ(readBack, 0.5);

    /* Changing the source updates the ugen's view */
    amp = 0.25;
    csoundUgenGetInputValue(ugen, 0, &readBack);
    EXPECT_DOUBLE_EQ(readBack, 0.25);

    csoundUgenDelete(ugen);
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

    /* Set inputs: amp=1.0, freq=1000, phase=0, iphs(optional)=0 */
    MYFLT amp = 1.0, freq = 1000.0, phase = 0.0, iphs = 0.0;
    csoundUgenSetInputValue(ugen, 0, &amp);
    csoundUgenSetInputValue(ugen, 1, &freq);
    csoundUgenSetInputValue(ugen, 2, &phase);
    csoundUgenSetInputValue(ugen, 3, &iphs);

    /* Init the opcode */
    EXPECT_EQ(csoundUgenInit(ugen), CSOUND_SUCCESS);

    /* Perform one k-cycle */
    EXPECT_EQ(csoundUgenPerform(ugen), CSOUND_SUCCESS);

    /* Read output – it's an audio buffer,  should have non-zero samples */
    MYFLT* outBuf = (MYFLT*)calloc(ksmps, sizeof(MYFLT));
    size_t sz = csoundUgenGetOutputValue(ugen, 0, outBuf);
    EXPECT_EQ(sz, (size_t)ksmps * sizeof(MYFLT));

    /* At least some samples should be non-zero for a 1kHz sine */
    bool hasNonZero = false;
    for (int i = 0; i < ksmps; i++) {
        if (outBuf[i] != 0.0) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);

    free(outBuf);
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

TEST_F(UGenTests, GraphConnect) {
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);

    /* Create two oscils and connect the audio output of the first
       to an input of the second (just testing the wiring, not the
       signal processing semantics). */
    UGEN* src = csoundUgenNew(factory, (char*)"oscils",
                          (char*)"a", (char*)"iiio");
    UGEN* dst = csoundUgenNew(factory, (char*)"oscils",
                          (char*)"a", (char*)"iiio");
    ASSERT_NE(src, nullptr);
    ASSERT_NE(dst, nullptr);

    /* Wire src output[0] -> dst input[0]
       (type mismatch a->i for oscils but this tests the pointer wiring) */
    EXPECT_TRUE(csoundUgenGraphConnect(src, 0, dst, 0));

    /* Verify the pointers are shared: write to src output, read from dst input */
    int32_t ksmps = csoundGetKsmps(csound);
    MYFLT* srcOutBuf = (MYFLT*)calloc(ksmps, sizeof(MYFLT));
    MYFLT* dstInBuf = (MYFLT*)calloc(ksmps, sizeof(MYFLT));

    /* Write a test value into src's output buffer */
    srcOutBuf[0] = 42.0;
    csoundUgenSetOutputValue(src, 0, srcOutBuf);

    /* Read from dst's input – should see 42.0 because they share the pointer */
    csoundUgenGetInputValue(dst, 0, dstInBuf);
    EXPECT_DOUBLE_EQ(dstInBuf[0], 42.0);

    free(srcOutBuf);
    free(dstInBuf);
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

    MYFLT amp = 1.0, freq = 440.0, phase = 0.0, iphs = 0.0;
    csoundUgenSetInputValue(ugen, 0, &amp);
    csoundUgenSetInputValue(ugen, 1, &freq);
    csoundUgenSetInputValue(ugen, 2, &phase);
    csoundUgenSetInputValue(ugen, 3, &iphs);

    csoundUgenGraphAdd(graph, ugen);

    EXPECT_EQ(csoundUgenGraphInit(graph), CSOUND_SUCCESS);
    EXPECT_EQ(csoundUgenGraphPerform(graph), CSOUND_SUCCESS);

    /* Output should have non-zero samples */
    int32_t ksmps = csoundGetKsmps(csound);
    MYFLT* outBuf = (MYFLT*)calloc(ksmps, sizeof(MYFLT));
    csoundUgenGetOutputValue(ugen, 0, outBuf);
    bool hasNonZero = false;
    for (int i = 0; i < ksmps; i++) {
        if (outBuf[i] != 0.0) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);

    free(outBuf);
    csoundUgenGraphDeleteAll(graph);
    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  Edge cases
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, NullUGenOps) {
    EXPECT_FALSE(csoundUgenDelete(nullptr));
    EXPECT_FALSE(csoundUgenSetOutput(nullptr, 0, nullptr));
    EXPECT_FALSE(csoundUgenSetInput(nullptr, 0, nullptr));
    EXPECT_EQ(csoundUgenGetInCount(nullptr), 0);
    EXPECT_EQ(csoundUgenGetOutCount(nullptr), 0);
    EXPECT_EQ(csoundUgenGetInType(nullptr, 0), UGEN_ARG_TYPE_UNKNOWN);
    EXPECT_EQ(csoundUgenGetOutType(nullptr, 0), UGEN_ARG_TYPE_UNKNOWN);
    EXPECT_EQ(csoundUgenGetInArgSize(nullptr, 0), (size_t)0);
    EXPECT_EQ(csoundUgenGetOutArgSize(nullptr, 0), (size_t)0);
    EXPECT_EQ(csoundUgenInit(nullptr), CSOUND_ERROR);
    EXPECT_EQ(csoundUgenPerform(nullptr), CSOUND_ERROR);
}

/* ------------------------------------------------------------------
 *  Memory layout – verify no double-offset
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, MemoryLayoutNoDoubleOffset) {
    /* Regression test for the critical "double-offset" bug.
     *
     * csoundRecalculateVarPoolMemory already bakes the per-variable
     * CS_VAR_TYPE_OFFSET header space into memBlockIndex.  The value
     * pointer p[i] must equal (data + memBlockIndex) – NOT
     * (data + memBlockIndex + headerSize).
     *
     * We verify this by checking that:
     *   1. p[i] == data + var->memBlockIndex   (output)
     *   2. p[outCount+i] == data + outDataOffset + var->memBlockIndex (input)
     *   3. The CS_VAR_MEM header at ((char*)p[i] - CS_VAR_TYPE_OFFSET)
     *      has a non-null varType pointer.
     */
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

        /* The CS_VAR_MEM header should sit just before the value */
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
    /* Verify the CS_VAR_MEM header doesn't overlap into adjacent
     * variable data — the header for variable N must not fall within
     * the value region of variable N-1. */
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    /* Use an opcode with multiple input args of different sizes */
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    MYFLT** p = (MYFLT**)((char*)ugen->opcodeMem + sizeof(OPDS));
    int32_t outCount = ugen->outCount;
    int32_t inCount = ugen->inCount;

    /* For each consecutive pair of input args, check no overlap.
     * p[outCount+i+1] - header_size >= p[outCount+i] + argSizeMYFLTs */
    for (int i = 0; i + 1 < inCount; i++) {
        MYFLT* curr = p[outCount + i];
        MYFLT* next = p[outCount + i + 1];
        size_t currArgSizeBytes = csoundUgenGetInArgSize(ugen, i);
        size_t currArgSizeMYFLTs = currArgSizeBytes / sizeof(MYFLT);

        /* next's header starts at (next - CS_VAR_TYPE_OFFSET/sizeof(MYFLT)) in
         * MYFLT space.  It must be >= curr + currArgSizeMYFLTs. */
        ptrdiff_t headerStartOffset = CS_FLOAT_ALIGN(CS_VAR_TYPE_OFFSET) / sizeof(MYFLT);
        MYFLT* nextHeaderStart = next - headerStartOffset;
        EXPECT_GE(nextHeaderStart, curr + (ptrdiff_t)currArgSizeMYFLTs)
            << "input[" << i+1 << "] header overlaps input[" << i << "] data";
    }

    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}

TEST_F(UGenTests, MemoryLayoutWriteReadRoundTrip) {
    /* Write through the value pointer, read back through the API,
     * and verify the CS_VAR_MEM header is intact.  This catches
     * double-offset bugs where writing at p[i] would clobber the
     * wrong memory region. */
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

    /* Write a test pattern into the audio output buffer */
    for (int i = 0; i < ksmps; i++) {
        p[0][i] = (MYFLT)(i + 1);
    }

    /* Read back through the API */
    MYFLT* readBuf = (MYFLT*)calloc(ksmps, sizeof(MYFLT));
    size_t sz = csoundUgenGetOutputValue(ugen, 0, readBuf);
    EXPECT_EQ(sz, (size_t)ksmps * sizeof(MYFLT));
    for (int i = 0; i < ksmps; i++) {
        EXPECT_DOUBLE_EQ(readBuf[i], (MYFLT)(i + 1));
    }
    free(readBuf);

    /* Write an i-rate scalar through the API */
    MYFLT val = 12345.0;
    csoundUgenSetInputValue(ugen, 0, &val);
    MYFLT readVal = 0;
    csoundUgenGetInputValue(ugen, 0, &readVal);
    EXPECT_DOUBLE_EQ(readVal, 12345.0);

    /* Verify headers are still intact after all writes */
    EXPECT_EQ(outHdr->varType, outType)
        << "output varType header was corrupted by data write";
    EXPECT_EQ(inHdr0->varType, inType0)
        << "input varType header was corrupted by data write";

    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  S / f type rejection
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, RejectStringOutputType) {
    /* Opcodes with S (string) type arguments should be rejected
     * because the UGen data layout lacks STRINGDAT init/free hooks. */
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"puts",
                           (char*)"i", (char*)"So");
    EXPECT_EQ(ugen, nullptr);
    csoundUgenFactoryDelete(factory);
}

TEST_F(UGenTests, RejectStringInputType) {
    /* Even if the output is numeric, string inputs should be rejected. */
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);

    /* Try "sprintf" if available: output S, input S... */
    UGEN* ugen = csoundUgenNew(factory, (char*)"strcat",
                           (char*)"S", (char*)"SS");
    /* Should either be NULL (rejected by S check) or not found */
    EXPECT_EQ(ugen, nullptr);
    csoundUgenFactoryDelete(factory);
}

TEST_F(UGenTests, RejectFsigType) {
    /* Opcodes using f-sig (PVOC) types are unsupported. */
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    /* pvsanal: "f" output, "aiio" input */
    UGEN* ugen = csoundUgenNew(factory, (char*)"pvsanal",
                           (char*)"f", (char*)"aiio");
    EXPECT_EQ(ugen, nullptr);
    csoundUgenFactoryDelete(factory);
}

TEST_F(UGenTests, AcceptNumericTypes) {
    /* Opcodes with only i/k/a types should be accepted. */
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);

    /* oscils: a, iiio – all numeric, should succeed */
    UGEN* u1 = csoundUgenNew(factory, (char*)"oscils",
                         (char*)"a", (char*)"iiio");
    EXPECT_NE(u1, nullptr);
    if (u1) csoundUgenDelete(u1);

    /* line: k, iii – k-rate output with i-rate inputs */
    UGEN* u2 = csoundUgenNew(factory, (char*)"line",
                         (char*)"k", (char*)"iii");
    EXPECT_NE(u2, nullptr);
    if (u2) csoundUgenDelete(u2);

    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  Multi-cycle stability (init + multiple performs)
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, MultiCycleStability) {
    /* Perform multiple k-cycles and verify the output buffer produces
     * valid samples each time, and that the CS_VAR_MEM headers stay
     * intact over repeated use.  Regression test for memory corruption. */
    UGEN_FACTORY* factory = csoundUgenFactoryNew(csound);
    UGEN* ugen = csoundUgenNew(factory, (char*)"oscils",
                           (char*)"a", (char*)"iiio");
    ASSERT_NE(ugen, nullptr);

    int32_t ksmps = csoundGetKsmps(csound);

    MYFLT amp = 0.5, freq = 1000.0, phase = 0.0, iphs = 0.0;
    csoundUgenSetInputValue(ugen, 0, &amp);
    csoundUgenSetInputValue(ugen, 1, &freq);
    csoundUgenSetInputValue(ugen, 2, &phase);
    csoundUgenSetInputValue(ugen, 3, &iphs);

    EXPECT_EQ(csoundUgenInit(ugen), CSOUND_SUCCESS);

    /* Remember the varType header pointer */
    MYFLT** p = (MYFLT**)((char*)ugen->opcodeMem + sizeof(OPDS));
    CS_VAR_MEM* outHdr = (CS_VAR_MEM*)((char*)p[0] - CS_VAR_TYPE_OFFSET);
    const CS_TYPE* origType = outHdr->varType;

    MYFLT* buf = (MYFLT*)calloc(ksmps, sizeof(MYFLT));

    for (int cycle = 0; cycle < 100; cycle++) {
        EXPECT_EQ(csoundUgenPerform(ugen), CSOUND_SUCCESS);

        size_t sz = csoundUgenGetOutputValue(ugen, 0, buf);
        EXPECT_EQ(sz, (size_t)ksmps * sizeof(MYFLT));

        /* Samples from a sine oscillator should be in [-1, 1] */
        for (int i = 0; i < ksmps; i++) {
            EXPECT_GE(buf[i], -1.0) << "cycle=" << cycle << " sample=" << i;
            EXPECT_LE(buf[i],  1.0) << "cycle=" << cycle << " sample=" << i;
        }

        /* Header must remain intact */
        EXPECT_EQ(outHdr->varType, origType)
            << "varType header corrupted at cycle " << cycle;
    }

    free(buf);
    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}

/* ------------------------------------------------------------------
 *  K-rate opcode memory layout
 * ------------------------------------------------------------------ */

TEST_F(UGenTests, MemoryLayoutKRate) {
    /* Verify the memory layout fix with a k-rate opcode (line)
     * which has different sizes than audio-rate opcodes. */
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

    /* Set inputs and verify init/perform work correctly */
    MYFLT ia = 0.0, dur = 1.0, ib = 1.0;
    csoundUgenSetInputValue(ugen, 0, &ia);
    csoundUgenSetInputValue(ugen, 1, &dur);
    csoundUgenSetInputValue(ugen, 2, &ib);
    EXPECT_EQ(csoundUgenInit(ugen), CSOUND_SUCCESS);
    EXPECT_EQ(csoundUgenPerform(ugen), CSOUND_SUCCESS);

    /* Read back the k-rate scalar output */
    MYFLT result = 0;
    EXPECT_EQ(csoundUgenGetOutputValue(ugen, 0, &result), sizeof(MYFLT));

    csoundUgenDelete(ugen);
    csoundUgenFactoryDelete(factory);
}
