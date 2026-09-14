#include <stdio.h>
#include <string.h>
#include "gtest/gtest.h"
#include "csound.h"
#include "csound_type_system.h"
#include "csdl.h"
#include "pstream.h"

#define csoundCompileOrc(a,b) csoundCompileOrc(a,b,0)
#define csoundScoreEvent(a,b,c,d) csoundEvent(a,0,c,d,0)

const char orc1 [] = "chn_k \"testing\", 3\n  instr 1\n  endin\n";

class ChannelTests : public ::testing::Test {
public:
    ChannelTests ()
    {
    }

    virtual ~ChannelTests ()
    {
    }

    virtual void SetUp ()
    {
      csound = csoundCreate (NULL, NULL);
      csoundCreateMessageBuffer (csound, 0);
      csoundSetOption (csound, "-n");
    }

    virtual void TearDown ()
    {
        csoundDestroy (csound);
        csound = nullptr;
    }

    CSOUND* csound {nullptr};

};

TEST_F (ChannelTests, ControlChannelParams)
{
    csoundCompileOrc (csound, orc1);
    ASSERT_TRUE(csoundStart(csound) == CSOUND_SUCCESS);
    controlChannelHints_t hints;
    hints.behav = CSOUND_CONTROL_CHANNEL_INT;
    hints.dflt = 5;
    hints.min = 1;
    hints.max = 10;
    hints.attributes = NULL;
    csoundSetControlChannelHints(csound, "testing", hints);

    controlChannelHints_t hints2;
    csoundGetControlChannelHints(csound, "testing", &hints2);
    ASSERT_TRUE(hints2.behav == CSOUND_CONTROL_CHANNEL_INT);
    ASSERT_TRUE(hints2.dflt == 5);
    ASSERT_TRUE(hints2.min == 1);
    ASSERT_TRUE(hints2.max == 10);
}

TEST_F (ChannelTests, ChnparamsWithoutHints)
{
    ASSERT_EQ(CSOUND_SUCCESS, csoundCompileOrc(csound, R"ORC(
        chn_k "plain", 3
        itype, imode, ictltype, idflt, imin, imax chnparams "plain"
        chnset itype, "type"
        chnset imode, "mode"
        chnset ictltype, "ctltype"
        chnset idflt, "default"
        chnset imin, "min"
        chnset imax, "max"
    )ORC"));
    ASSERT_EQ(CSOUND_SUCCESS, csoundStart(csound));

    EXPECT_EQ(CSOUND_CONTROL_CHANNEL,
              csoundGetControlChannel(csound, "type", nullptr));
    EXPECT_EQ(3, csoundGetControlChannel(csound, "mode", nullptr));
    EXPECT_EQ(0, csoundGetControlChannel(csound, "ctltype", nullptr));
    EXPECT_EQ(0, csoundGetControlChannel(csound, "default", nullptr));
    EXPECT_EQ(0, csoundGetControlChannel(csound, "min", nullptr));
    EXPECT_EQ(0, csoundGetControlChannel(csound, "max", nullptr));
}

TEST_F (ChannelTests, ControlChannel)
{
    csoundCompileOrc(csound, orc1);
    ASSERT_TRUE(csoundStart(csound) == CSOUND_SUCCESS);
    csoundSetControlChannel(csound, "testing", 5.0);
    ASSERT_EQ(5.0, csoundGetControlChannel(csound, "testing", NULL));
}

const char orc2[] = "chn_k \"testing\", 3, 1, 1, 0, 10\n  chn_a \"testing2\", 3\n  instr 1\n  endin\n";

TEST_F (ChannelTests, ChannelList)
{
    csoundCompileOrc(csound, orc2);
    ASSERT_TRUE(csoundStart(csound) == CSOUND_SUCCESS);
    controlChannelInfo_t *lst;
    int32_t numchnls = csoundListChannels(csound, &lst);
    ASSERT_TRUE(numchnls == 2);
    ASSERT_STREQ(lst->name, "testing");
    ASSERT_EQ (lst->type, CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL
        | CSOUND_OUTPUT_CHANNEL | CSOUND_CONTROL_CHANNEL_INT);
    ASSERT_STREQ(lst[1].name, "testing2");

    csoundDeleteChannelList(csound, lst);
}

const char orc3[] = "instr 1\n kval invalue \"intest\"\n"
        "outvalue \"intest\",kval\n"
        "Sval invalue \"instrtest\"\n"
        "outvalue \"instrtest\",Sval\n"
        "endin\n"
        "instr 2\n outvalue \"outtest\", 10\n endin\n";

void inputCallback(CSOUND *csound,
                   const char *channelName,
                   void *channelValuePtr,
                   void *channelType)
{
    if (strcmp(channelName, "intest") == 0 /*&& channelType == &CS_VAR_TYPE_K*/) {
        MYFLT *v = (MYFLT *) channelValuePtr;
        *v = 5.0;
    }
    if (strcmp(channelName, "instrtest") == 0 /*&& channelType == &CS_VAR_TYPE_S*/) {
        char *v = (char *) channelValuePtr;
        strcpy(v, "hello channels");
    }
}

void outputCallback(CSOUND *csound,
                   const char *channelName,
                   void *channelValuePtr,
                   void *channelType)
{
    if (strcmp(channelName, "intest") == 0 /*&& channelType == &CS_VAR_TYPE_K*/) {
        MYFLT *v = (MYFLT *) channelValuePtr;
        ASSERT_DOUBLE_EQ(*v, 5.0);
    }
    if (strcmp(channelName, "instrtest") == 0 /*&& channelType == &CS_VAR_TYPE_S*/) {
        ASSERT_STREQ((char *) channelValuePtr, "hello channels");
    }
    if (strcmp(channelName, "outtest") == 0 /*&& channelType == &CS_VAR_TYPE_K*/) {
        MYFLT *v = (MYFLT *) channelValuePtr;
        ASSERT_DOUBLE_EQ(*v, 10.0);
    }

}

TEST_F (ChannelTests, ChannelCallbacks)
{
    csoundCompileOrc(csound, orc3);
    csoundSetInputChannelCallback(csound, (channelCallback_t) inputCallback);
    csoundSetOutputChannelCallback(csound, (channelCallback_t) outputCallback);
    int32_t err = csoundStart(csound);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    MYFLT pFields[] = {1.0, 0.0, 1.0};
    csoundScoreEvent(csound, 'i', pFields, 3);
    MYFLT pFields2[] = {2.0, 0.0, 1.0};
    csoundScoreEvent(csound, 'i', pFields2, 3);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    err = csoundPerformKsmps(csound);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
}

const char orc4[] = "chn_k \"1\", 3\n"
        "chn_k \"2\", 3\n"
        "chn_k \"3\", 3\n"
        "chn_k \"4\", 3\n"

        "instr 1\n"
        "kval invalue \"1\"\n"
        "outvalue \"2\",kval\n"
        "endin\n"

        "instr 2\n"
        "kval chani 2\n"
        "chano kval + 1, 3\n"
        "endin\n"

        "instr 3\n"
        "kval chnget \"3\"\n"
        "chnset kval + 1, \"4\"\n"
        "endin\n";

void inputCallback2(CSOUND *csound,
                   const char *channelName,
                   void *channelValuePtr,
                   void *channelType)
{
    MYFLT val = csoundGetControlChannel(csound, channelName, NULL);
    MYFLT *valPtr = (MYFLT *) channelValuePtr;
    *valPtr = val;
}

void outputCallback2(CSOUND *csound,
                   const char *channelName,
                   void *channelValuePtr,
                   void *channelType)
{
    MYFLT *valPtr = (MYFLT *) channelValuePtr;
    csoundSetControlChannel(csound, channelName, *valPtr);
}

TEST_F (ChannelTests, ChannelOpcodes)
{
    csoundCompileOrc(csound, orc4);
    csoundSetInputChannelCallback(csound, (channelCallback_t) inputCallback2);
    csoundSetOutputChannelCallback(csound, (channelCallback_t) outputCallback2);
    int32_t err = csoundStart(csound);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    csoundGetControlChannel(csound, "1", &err);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    csoundSetControlChannel(csound, "1", 5.0);
    MYFLT pFields[] = {1.0, 0.0, 1.0};
    csoundEvent(csound, 0, pFields, 3, 0);
    err = csoundPerformKsmps(csound);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    ASSERT_EQ(5.0, csoundGetControlChannel(csound, "2", NULL));
    MYFLT pFields2[] = {2.0, 0.0, 1.0};
    csoundEvent(csound, 0, pFields2, 3, 0);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    err = csoundPerformKsmps(csound);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    ASSERT_EQ(6.0, csoundGetControlChannel(csound, "3", NULL));
    MYFLT pFields3[] = {3.0, 0.0, 1.0};
    csoundEvent(csound, 0, pFields3, 3, 0);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    err = csoundPerformKsmps(csound);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    ASSERT_EQ(7.0, csoundGetControlChannel(csound, "4", NULL));
}



const char orc5[] = "chn_k \"winsize\", 3\n"
        "instr 1\n"
        "finput pvsin \"pvs-input\"\n"
        "ioverlap, inumbins, iwinsize, iformat pvsinfo finput\n"
        "pvsout finput, \"pvs-output\"\n"
        "chnset iwinsize, \"winsize\"\n"
        "endin\n";

TEST_F (ChannelTests, PVSOpcodes)
{
    constexpr int32_t fftSize = 1024;
    constexpr int32_t overlap = 256;
    constexpr int32_t windowSize = 1024;
    constexpr size_t frameLength = fftSize + 2;
    float frame[frameLength];
    for (size_t i = 0; i < frameLength; ++i)
      frame[i] = static_cast<float>(i) + 0.25f;

    int32_t err = csoundCompileOrc(csound, orc5);
    ASSERT_EQ(CSOUND_SUCCESS, err);
    ASSERT_EQ(CSOUND_SUCCESS, csoundStart(csound));

    PVSDAT *input = csoundInitPvsChannel(csound, "host-input",
                                         fftSize, overlap, windowSize,
                                         PVS_WIN_HANN, PVS_AMP_FREQ);
    ASSERT_NE(nullptr, input);
    csoundSetPvsData(input, frame);

    PVSDAT *invalidDestination =
      csoundInitPvsChannel(csound, "invalid-input",
                           fftSize, overlap, windowSize,
                           PVS_WIN_HANN, PVS_AMP_FREQ);
    ASSERT_NE(nullptr, invalidDestination);

    PVSDAT invalidInput = *input;
    invalidInput.frame.size -= sizeof(float);
    EXPECT_EQ(CSOUND_ERROR,
              csoundSetPvsChannel(csound, "invalid-input", &invalidInput));
    ASSERT_EQ(CSOUND_SUCCESS,
              csoundSetPvsChannel(csound, "pvs-input", input));

    MYFLT pFields[] = {1.0, 0.0, 1.0};
    csoundScoreEvent(csound, 'i', pFields, 3);
    ASSERT_EQ(CSOUND_SUCCESS, csoundPerformKsmps(csound));
    EXPECT_EQ(windowSize,
              csoundGetControlChannel(csound, "winsize", nullptr));

    PVSDAT *smallOutput = csoundInitPvsChannel(csound, "host-small-output",
                                               fftSize / 2, overlap, windowSize,
                                               PVS_WIN_HANN, PVS_AMP_FREQ);
    ASSERT_NE(nullptr, smallOutput);
    EXPECT_EQ(CSOUND_ERROR,
              csoundGetPvsChannel(csound, "pvs-output", smallOutput));
    EXPECT_EQ(fftSize / 2, csoundPvsDataFFTSize(smallOutput));

    PVSDAT *output = csoundInitPvsChannel(csound, "host-output",
                                          fftSize, overlap, windowSize,
                                          PVS_WIN_HANN, PVS_AMP_FREQ);
    ASSERT_NE(nullptr, output);
    ASSERT_EQ(CSOUND_SUCCESS,
              csoundGetPvsChannel(csound, "pvs-output", output));
    EXPECT_EQ(fftSize, csoundPvsDataFFTSize(output));
    EXPECT_EQ(overlap, csoundPvsDataOverlap(output));
    EXPECT_EQ(windowSize, csoundPvsDataWindowSize(output));
    EXPECT_EQ(PVS_AMP_FREQ, csoundPvsDataFormat(output));

    const float *outputFrame = csoundGetPvsData(output);
    ASSERT_NE(nullptr, outputFrame);
    for (size_t i = 0; i < frameLength; ++i)
      EXPECT_EQ(frame[i], outputFrame[i]) << "frame index " << i;
}

TEST_F (ChannelTests, InvalidChannel)
{
    csoundCompileOrc(csound, orc5);

    int32_t err;
    ASSERT_EQ(0.0, csoundGetControlChannel(csound, "nonexistent_channel", &err));
    ASSERT_EQ(err, CSOUND_SUCCESS);
}

TEST_F (ChannelTests, ArrayDataSetterRejectsManagedElements)
{
    const int32_t sizes[] = {1};
    ARRAYDAT *numbers = csoundInitArrayChannel(
      csound, "host-numbers", "i", 1, sizes);
    ARRAYDAT *strings = csoundInitArrayChannel(
      csound, "host-strings", "S", 1, sizes);
    const MYFLT number = (MYFLT)42.0;
    const void *originalStorage;
    void *originalString;
    void *currentString;

    ASSERT_NE(nullptr, numbers);
    ASSERT_NE(nullptr, strings);
    ASSERT_EQ(CSOUND_SUCCESS, csoundSetArrayData(numbers, &number));
    EXPECT_EQ(number, static_cast<const MYFLT *>(
                        csoundGetArrayData(numbers))[0]);

    originalStorage = csoundGetArrayData(strings);
    ASSERT_NE(nullptr, originalStorage);
    memcpy(&originalString, originalStorage, sizeof(originalString));
    EXPECT_EQ(CSOUND_ERROR, csoundSetArrayData(strings, &number));
    EXPECT_EQ(originalStorage, csoundGetArrayData(strings));
    memcpy(&currentString, originalStorage, sizeof(currentString));
    EXPECT_EQ(originalString, currentString);
}

TEST_F (ChannelTests, InvalidArrayChannelShapeDoesNotPublishMetadata)
{
    const int32_t invalidSizes[] = {-1};
    const int32_t validSizes[] = {2};
    void *channelPointer = nullptr;

    ASSERT_EQ(CSOUND_SUCCESS, csoundGetChannelPtr(
      csound, &channelPointer, "array-shape",
      CSOUND_ARRAY_CHANNEL | CSOUND_INPUT_CHANNEL |
      CSOUND_OUTPUT_CHANNEL));
    ARRAYDAT *array = static_cast<ARRAYDAT *>(channelPointer);
    ASSERT_NE(nullptr, array);
    const int32_t oldDimensions = array->dimensions;
    int32_t *const oldSizes = array->sizes;
    const CS_TYPE *const oldArrayType = array->arrayType;
    MYFLT *const oldData = array->data;
    const size_t oldAllocated = array->allocated;

    EXPECT_EQ(nullptr, csoundInitArrayChannel(
      csound, "array-shape", "k", 1, invalidSizes));
    EXPECT_EQ(oldDimensions, array->dimensions);
    EXPECT_EQ(oldSizes, array->sizes);
    EXPECT_EQ(oldArrayType, array->arrayType);
    EXPECT_EQ(oldData, array->data);
    EXPECT_EQ(oldAllocated, array->allocated);

    EXPECT_EQ(array, csoundInitArrayChannel(
      csound, "array-shape", "k", 1, validSizes));
    EXPECT_EQ(1, array->dimensions);
    ASSERT_NE(nullptr, array->sizes);
    EXPECT_EQ(2, array->sizes[0]);
    EXPECT_NE(nullptr, array->data);
}

const char orc6[] = "chn_k \"chan\", 3, 2, 0.5, 0, 1, 10, 10, 50, 100\n"
        "chn_k \"chan2\", 3, 2, 0.5, 0, 1, 10, 10, 50, 100, \"testattr\"\n"
        "chn_k \"chan3\", 3, 2, 0.5, 0, 1\n"
        "instr 1\n kval invalue \"1\"\n"
        "outvalue \"2\",kval\n"
        "endin\n";

TEST_F (ChannelTests, ChnHints)
{
    csoundCompileOrc(csound, orc6);
    (void)csoundStart(csound);
//    err = csoundPerformKsmps(csound); //Need this to load instr 0
    controlChannelHints_t hints;
    hints.attributes = NULL;
    ASSERT_EQ(0, csoundGetControlChannelHints(csound, "chan", &hints));
    ASSERT_EQ(hints.x, 10);
    ASSERT_EQ(hints.y, 10);
    ASSERT_EQ(hints.width, 50);
    ASSERT_EQ(hints.height, 100);
    ASSERT_EQ(hints.attributes, (char*)NULL);
    ASSERT_EQ(0, csoundGetControlChannelHints(csound, "chan2", &hints));
    ASSERT_EQ(hints.x, 10);
    ASSERT_EQ(hints.y, 10);
    ASSERT_EQ(hints.width, 50);
    ASSERT_EQ(hints.height, 100);
    ASSERT_STREQ(hints.attributes, "testattr");
}

TEST_F (ChannelTests, StringChannel)
{
    const char orcS[] = "chn_S \"strchan1\",1\n chn_S \"strchan2\",2\n chn_S \"strchan3\",3\n instr 1\n  endin\n";

    csoundCompileOrc(csound, orcS);
    int32_t err = csoundStart(csound);
    ASSERT_TRUE(err == CSOUND_SUCCESS);

    csoundSetStringChannel(csound, "testing", "ttt");
    int32_t len = csoundGetChannelDatasize(csound, "testing");
    char* string = new char[len];
    csoundGetStringChannel(csound, "testing", string);
    ASSERT_STREQ(string, "ttt");

    csoundSetStringChannel(csound, "strchan1", "strchan1_val");
    csoundSetStringChannel(csound, "strchan2", "strchan2_val");
    csoundSetStringChannel(csound, "strchan3", "strchan3_val");

    csoundGetStringChannel(csound, "strchan1", string);
    ASSERT_STREQ(string, "strchan1_val");

    csoundGetStringChannel(csound, "strchan2", string);
    ASSERT_STREQ(string, "strchan2_val");

    csoundGetStringChannel(csound, "strchan3", string);
    ASSERT_STREQ(string, "strchan3_val");

    delete [] string;
}

TEST_F (ChannelTests, ChannelVarMem)
{
  csoundCompileOrc(csound, R"ORC(
                 chnset 1, "1"
                 chnset 0, "2"
                 instr 1
                  k1 chnget "2"
                  chnset k1, "1"
                 endin
                 schedule(1,0,1)
                )ORC");
    int32_t err = csoundStart(csound);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    const CS_VAR_MEM *var = csoundGetChannel(csound, "1");
    MYFLT val = csoundGetChannel(csound, "2")->value;
    ASSERT_EQ(val, 0.0);
    err = csoundSetChannel(csound, "2", var);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    csoundPerformKsmps(csound);
    ASSERT_EQ(csoundGetChannel(csound, "2")->value,
              csoundGetChannel(csound, "1")->value);
}

TEST_F (ChannelTests, ChannelNewVarMem)
{
    const char *name = "testing";
    csoundCompileOrc(csound, orc1);
    int32_t err = csoundStart(csound);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    CS_VAR_MEM memBlock; // memblock value holds enough storage for a MYFLT
    memBlock.varType = csoundGetTypeWithVarTypeName(csoundGetTypePool(csound), "k");
    memBlock.value = 1.0;
    err = csoundSetChannel(csound, name, &memBlock);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    ASSERT_EQ(memBlock.value, csoundGetChannel(csound,name)->value);
}


TEST_F (ChannelTests, ChannelSetRejectsInvalidState)
{
    csoundCompileOrc(csound, orc2);
    ASSERT_EQ(CSOUND_SUCCESS, csoundStart(csound));

    const CS_VAR_MEM *channel = csoundGetChannel(csound, "testing");
    ASSERT_NE(nullptr, channel);
    EXPECT_EQ(CSOUND_ERROR, csoundSetChannel(csound, nullptr, channel));
    EXPECT_EQ(CSOUND_ERROR, csoundSetChannel(csound, "testing", nullptr));

    CS_VAR_MEM missingType{};
    EXPECT_EQ(CSOUND_ERROR,
              csoundSetChannel(csound, "testing", &missingType));

    CS_VAR_MEM wrongType{};
    wrongType.varType = csoundGetTypeWithVarTypeName(
      csoundGetTypePool(csound), "i");
    EXPECT_EQ(CSOUND_ERROR,
              csoundSetChannel(csound, "testing", &wrongType));
    EXPECT_EQ(CSOUND_ERROR,
              csoundSetChannel(csound, "missing", channel));
}



TEST_F (ChannelTests, VarChannelType)
{
  csoundCompileOrc(csound, R"ORC(
                 instr 1
                  var:Complex init 1,1
                  chnset var, "cmplx"
                 endin
                 schedule(1,0,1)
                )ORC");
    int32_t err = csoundStart(csound);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    csoundPerformKsmps(csound);
    const CS_VAR_MEM *var = csoundGetChannel(csound, "cmplx");
    COMPLEXDAT *vardat = (COMPLEXDAT *) (&var->value), cmplx = { 2., 2., 0};

    ASSERT_EQ(vardat->real, 1.0);
    ASSERT_EQ(vardat->imag, 1.0);

    // set values
    CS_VAR_MEM *memBlock = (CS_VAR_MEM *) csound->Calloc(csound,
                                                         CS_VAR_TYPE_OFFSET + sizeof(COMPLEXDAT));
    memBlock->varType = csoundGetTypeWithVarTypeName(csoundGetTypePool(csound), "Complex");
    memcpy(&memBlock->value, &cmplx, sizeof(COMPLEXDAT));
    err =  csoundSetChannel(csound,"cmplx", memBlock);

    void *ptr;
    // can access
    err = csoundGetChannelPtr(csound, &ptr, "cmplx", CSOUND_VAR_CHANNEL |
                        CSOUND_OUTPUT_CHANNEL);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    ASSERT_EQ(((COMPLEXDAT *)ptr)->real, 2.0);
    ASSERT_EQ(((COMPLEXDAT *)ptr)->imag, 2.0);

    controlChannelInfo_t *channels = nullptr;
    int32_t channelCount = csoundListChannels(csound, &channels);
    csoundDeleteChannelList(csound, channels);

    // cannot create
    err = csoundGetChannelPtr(csound, &ptr, "cmplx2", CSOUND_VAR_CHANNEL |
                        CSOUND_INPUT_CHANNEL);
    ASSERT_FALSE(err == CSOUND_SUCCESS);
    channels = nullptr;
    ASSERT_EQ(channelCount, csoundListChannels(csound, &channels));
    for(int32_t i = 0; i < channelCount; ++i)
      ASSERT_STRNE("cmplx2", channels[i].name);
    csoundDeleteChannelList(csound, channels);
    csound->Free(csound, memBlock);
}


TEST_F (ChannelTests, ExportedInitChannelKeepsVarType)
{
  csoundCompileOrc(csound, R"ORC(
                 instr 1
                  value@global:i chnexport "init-export", 3
                 endin
                 schedule(1, 0, 1)
                )ORC");
  ASSERT_EQ(CSOUND_SUCCESS, csoundStart(csound));
  ASSERT_EQ(CSOUND_SUCCESS, csoundPerformKsmps(csound));

  const CS_TYPE *initType = csoundGetTypeWithVarTypeName(
    csoundGetTypePool(csound), "i");
  const CS_VAR_MEM *channel = csoundGetChannel(csound, "init-export");
  ASSERT_NE(nullptr, channel);
  ASSERT_EQ(initType, channel->varType);
  ASSERT_EQ(initType, csoundGetChannelVarType(csound, "init-export"));

  CS_VAR_MEM replacement;
  replacement.varType = initType;
  replacement.value = 7.0;
  ASSERT_EQ(CSOUND_SUCCESS,
            csoundSetChannel(csound, "init-export", &replacement));
  ASSERT_EQ(replacement.value,
            csoundGetChannel(csound, "init-export")->value);
}


TEST_F (ChannelTests, ExportedArrayChannelKeepsStorageAndMetadata)
{
  csoundCompileOrc(csound, R"ORC(
                 instr 1
                  values@global:k[] chnexport "array-export", 3
                  values fillarray 3, 4
                 endin
                 instr 2
                  values:k[] chnget "array-export"
                  chnset values[1], "array-export-result"
                 endin
                 schedule(1, 0, 1)
                 schedule(2, 0, 1)
                )ORC");
  ASSERT_EQ(CSOUND_SUCCESS, csoundStart(csound));
  ASSERT_EQ(CSOUND_SUCCESS, csoundPerformKsmps(csound));

  const CS_VAR_MEM *channel = csoundGetChannel(csound, "array-export");
  ASSERT_NE(nullptr, channel);
  ASSERT_STREQ("[", channel->varType->varTypeName);
  const ARRAYDAT *array = reinterpret_cast<const ARRAYDAT*>(&channel->value);
  ASSERT_EQ(1, array->dimensions);
  ASSERT_STREQ("k", array->arrayType->varTypeName);
  ASSERT_NE(nullptr, array->data);
  EXPECT_EQ((MYFLT)3.0, array->data[0]);
  EXPECT_EQ((MYFLT)4.0, array->data[1]);

  ASSERT_EQ(CSOUND_SUCCESS, csoundPerformKsmps(csound));
  EXPECT_EQ(channel, csoundGetChannel(csound, "array-export"));
  EXPECT_EQ((MYFLT)4.0,
            csoundGetControlChannel(csound, "array-export-result", nullptr));
}

TEST_F (ChannelTests, AudioArrayArithmeticHonorsSampleAccurateNoteEnds)
{
  ASSERT_EQ(CSOUND_SUCCESS,
            csoundSetOption(csound, "--sample-accurate"));
  ASSERT_EQ(CSOUND_SUCCESS, csoundCompileOrc(csound, R"ORC(
                 sr = 64
                 ksmps = 64
                 nchnls = 1
                 0dbfs = 1

                 gaSub[] init 1
                 gaMul[] init 1
                 gaDiv[] init 1
                 gaSubIn[] init 1
                 gaDivIn[] init 1

                 instr 1
                   a12[] init 1
                   a3[] init 1
                   a6[] init 1
                   a12[0] = 12
                   a3[0] = 3
                   a6[0] = 6

                   gaSub = a12 - a3
                   gaMul = a12 * a3
                   gaDiv = a12 / a3
                   gaSubIn -= a6
                   gaDivIn /= a3

                   chnset gaSub, "array-sub"
                   chnset gaMul, "array-mul"
                   chnset gaDiv, "array-div"
                   chnset gaSubIn, "array-sub-in"
                   chnset gaDivIn, "array-div-in"
                 endin

                 instr 2
                   gaSub[0] = 99
                   gaMul[0] = 99
                   gaDiv[0] = 99
                   gaSubIn[0] = 20
                   gaDivIn[0] = 21
                 endin

                 instr 3
                 endin

                 schedule(2, 0, 1)
                 schedule(1, 1, 1 / sr)
                 schedule(2, 2, 1)
                 schedule(1, 3, 48 / sr)
                 schedule(3, 0, 5)
                )ORC"));
  ASSERT_EQ(CSOUND_SUCCESS, csoundStart(csound));
  ASSERT_EQ(64, csoundGetKsmps(csound));

  struct ExpectedChannel {
    const char *name;
    MYFLT value;
  };
  const ExpectedChannel expectedChannels[] = {
    {"array-sub", 9},
    {"array-mul", 36},
    {"array-div", 4},
    {"array-sub-in", 14},
    {"array-div-in", 7},
  };
  const auto checkChannels = [&](int32_t activeSamples) {
    for (const ExpectedChannel &expected : expectedChannels) {
      const CS_VAR_MEM *channel = csoundGetChannel(csound, expected.name);
      ASSERT_NE(nullptr, channel) << expected.name;
      ASSERT_STREQ("[", channel->varType->varTypeName) << expected.name;
      const ARRAYDAT *array =
        reinterpret_cast<const ARRAYDAT*>(&channel->value);
      ASSERT_EQ(1, array->dimensions) << expected.name;
      ASSERT_NE(nullptr, array->sizes) << expected.name;
      ASSERT_EQ(1, array->sizes[0]) << expected.name;
      ASSERT_STREQ("a", array->arrayType->varTypeName) << expected.name;
      ASSERT_EQ(64 * static_cast<int32_t>(sizeof(MYFLT)),
                array->arrayMemberSize) << expected.name;
      ASSERT_NE(nullptr, array->data) << expected.name;
      for (int32_t sample = 0; sample < 64; ++sample) {
        const MYFLT expectedValue =
          sample < activeSamples ? expected.value : 0;
        EXPECT_EQ(expectedValue, array->data[sample])
          << expected.name << ", sample " << sample;
      }
    }
  };

  ASSERT_EQ(CSOUND_SUCCESS, csoundPerformKsmps(csound));
  ASSERT_EQ(CSOUND_SUCCESS, csoundPerformKsmps(csound));
  checkChannels(1);
  ASSERT_EQ(CSOUND_SUCCESS, csoundPerformKsmps(csound));
  ASSERT_EQ(CSOUND_SUCCESS, csoundPerformKsmps(csound));
  checkChannels(48);
}

TEST_F (ChannelTests, ArrayChannel)
{
    const char *name = "testing";
    int32_t err = csoundStart(csound);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    int32_t size = 2;
    ARRAYDAT *adat = csoundInitArrayChannel(csound, name, "k", 1, &size);
    MYFLT data[2] = {1., 2.};
    csoundSetArrayData(adat, data);
    const MYFLT *dataout = (const MYFLT *) csoundGetArrayData(adat);
    ASSERT_EQ(dataout[0], data[0]);
    ASSERT_EQ(dataout[1], data[1]);
    csoundCompileOrc(csound, R"ORC(
             instr 1
              var:k[] chnget "testing"
              chnset var[0], "control"
             endin
             schedule(1,0,1);
                     )ORC");
    csoundPerformKsmps(csound);
    MYFLT val = csoundGetControlChannel(csound, "control", &err);
    ASSERT_EQ(val, data[0]);
}

TEST_F (ChannelTests, ArrayChannelDataType) {
    ARRAYDAT *adat = NULL;
    csoundCompileOrc(csound, "instr 1\nendin\n");
    csoundStart(csound); 
    csoundGetChannelPtr(csound, (void **) &adat, "never_initialized",
                              CSOUND_ARRAY_CHANNEL |
                              CSOUND_INPUT_CHANNEL |
                              CSOUND_OUTPUT_CHANNEL);
    const char *type = csoundArrayDataType(adat);  
    ASSERT_TRUE(type == NULL);
}


TEST_F (ChannelTests, AudioChannel)
{
    csoundCompileOrc(csound, R"ORC(
             instr 1
              sig:a oscili 0dbfs, A4
              chnset sig, "audio"
             endin
             schedule(1,0,1);
                     )ORC");
    int32_t err = csoundStart(csound);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    csoundPerformKsmps(csound);
    MYFLT audio[10], rms = 0.; // ksmps defaults to 10
    csoundGetAudioChannel(csound, "audio", audio);
    for(int i = 0; i < 10; i++) {
      rms += audio[i]*audio[i];
    }
    rms = sqrt(rms/10);
    ASSERT_TRUE(rms > 0);
    // now let's test getting it as an audio variable
    MYFLT pow = 0;
    const CS_VAR_MEM *var = csoundGetChannel(csound, "audio");
    const MYFLT *sample = &(var->value);
    for(int i = 0; i < 10; i++) {
      pow += sample[i]*sample[i];
    }
    ASSERT_TRUE(pow > 0);
}
