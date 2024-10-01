#include <stdio.h>
#include <string.h>
#include "gtest/gtest.h"
#include "csound.h"

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
      csoundSetOption (csound, "--logfile=NULL");
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
    csoundScoreEvent(csound, 'i', pFields, 3);
    err = csoundPerformKsmps(csound);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    ASSERT_EQ(5.0, csoundGetControlChannel(csound, "2", NULL));
    MYFLT pFields2[] = {2.0, 0.0, 1.0};
    csoundScoreEvent(csound, 'i', pFields2, 3);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    err = csoundPerformKsmps(csound);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    ASSERT_EQ(6.0, csoundGetControlChannel(csound, "3", NULL));
    MYFLT pFields3[] = {3.0, 0.0, 1.0};
    csoundScoreEvent(csound, 'i', pFields3, 3);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    err = csoundPerformKsmps(csound);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    ASSERT_EQ(7.0, csoundGetControlChannel(csound, "4", NULL));
}



const char orc5[] = "chn_k \"winsize\", 3\n"
        "instr 1\n"
        "finput pvsin 1 \n"
        "ioverlap, inumbins, iwinsize, iformat pvsinfo finput\n"
        "pvsout finput, 1\n"
        "chnset iwinsize, \"winsize\"\n"
        "endin\n";
/*
 NEEDS TO BE ADAPTED for new API
TEST_F (ChannelTests, PVSOpcodes)
{
    int32_t err = csoundCompileOrc(csound, orc5);
    ASSERT_TRUE(err == CSOUND_SUCCESS);
    err = csoundStart(csound);
    PVSDATEXT pvs_data, pvs_data2;
    memset(&pvs_data,0,sizeof(PVSDATEXT));
    memset(&pvs_data2,0,sizeof(PVSDATEXT));
    pvs_data.N = 16;
    pvs_data.winsize = 32;
    err = csoundSetPvsChannel(csound, &pvs_data, "1");
    err = csoundGetPvsChannel(csound, &pvs_data2, "1");
    ASSERT_EQ(pvs_data.N, pvs_data2.N);
    MYFLT pFields[] = {1.0, 0.0, 1.0};
    err = csoundScoreEvent(csound, 'i', pFields, 3);
    err = csoundPerformKsmps(csound);
    ASSERT_EQ(32.0, csoundGetControlChannel(csound, "winsize", NULL));
}
*/

TEST_F (ChannelTests, InvalidChannel)
{
    csoundCompileOrc(csound, orc5);

    int32_t err;
    ASSERT_EQ(0.0, csoundGetControlChannel(csound, "nonexistent_channel", &err));
    ASSERT_EQ(err, CSOUND_SUCCESS);
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
