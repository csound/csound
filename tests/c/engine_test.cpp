#include <unistd.h>       // for sleep

#include "csound.h"       // for csoundSetOption, csoundCleanup, csoundCreate
#include "gtest/gtest.h"  // for TEST_F, Test

class EngineTests : public ::testing::Test {
public:
    EngineTests ()
    {
    }

    virtual ~EngineTests ()
    {
    }

    virtual void SetUp ()
    {
        csoundSetGlobalEnv ("OPCODE6DIR64", "../../");
        csound = csoundCreate (0);
        csoundCreateMessageBuffer (csound, 0);
        csoundSetOption (csound, "--logfile=NULL -odac");
    }

    virtual void TearDown ()
    {
        csoundCleanup (csound);
        csoundDestroyMessageBuffer (csound);
        csoundDestroy (csound);
        csound = nullptr;
    }

    CSOUND* csound {nullptr};
};

TEST_F (EngineTests, testUdpServer)
{
    csoundSetIsGraphable(csound, 1);
    csoundSetOption(csound,"--port=12345");
    csoundStart(csound);
#ifdef _MSC_VER
    //Sleep (1);
#else
    sleep (1);
#endif
    csoundStop(csound);
}
