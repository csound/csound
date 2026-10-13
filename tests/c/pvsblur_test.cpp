#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "csdebug.h"
#include "pstream.h"
#include "gtest/gtest.h"

#include <cstring>
#include <memory>
#include <string>

TEST(PvsBlurTests, RejectsOddFrameSize)
{
    std::unique_ptr<CSOUND, decltype(&csoundDestroy)> engine(
        csoundCreate(nullptr, nullptr), csoundDestroy);
    CSOUND *csound = engine.get();
    ASSERT_NE(csound, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompileOrc(csound,
        "sr = 8192\nksmps = 32\nnchnls = 1\n0dbfs = 1\n"
        "gfInput pvsinit 64, 32\n"
        "instr 1\n"
        "fOutput pvsblur gfInput, .01, .1\n"
        "endin\n", 0), CSOUND_SUCCESS);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);

    PVSDAT *input = nullptr;
    auto *variables = csoundDebugGetGlobalVariables(csound);
    for (auto *variable = variables; variable; variable = variable->next)
        if (std::strcmp(variable->name, "gfInput") == 0)
            input = static_cast<PVSDAT *>(variable->data);
    csoundDebugFreeVariables(csound, variables);
    ASSERT_NE(input, nullptr);

    // Supply invalid frame metadata directly. Built-in producers now reject
    // odd sizes themselves, but pvsblur must still check its own input.
    input->N = 63;
    csoundEventString(csound, "i 1 0 .01", 0);
    csoundPerformKsmps(csound);

    std::string messages;
    while (csoundGetMessageCnt(csound)) {
        messages += csoundGetFirstMessage(csound);
        csoundPopFirstMessage(csound);
    }
    EXPECT_NE(messages.find("pvsblur: invalid frame size"), std::string::npos)
        << messages;
    EXPECT_GT(csound->inerrcnt, 0);
}
