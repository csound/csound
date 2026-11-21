#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <string>

namespace {
/* Notes of instr 1, played one after another on one instance, and the blocks
   Csound holds between them. */
class Notes {
public:
    explicit Notes(const std::string &body)
    {
        csound = csoundCreate(nullptr, nullptr);
        csoundSetMessageCallback(csound,
                                 [](CSOUND *, int32_t, const char *, va_list) {});
        csoundSetOption(csound, "-n");
        csoundSetOption(csound, "-d");
        const std::string orchestra =
            "sr=1024\nksmps=32\nnchnls=1\n0dbfs=1\ninstr 1\n" + body + "\nendin\n";
        EXPECT_EQ(CSOUND_SUCCESS, csoundCompileOrc(csound, orchestra.c_str(), 0));
        EXPECT_EQ(CSOUND_SUCCESS, csoundStart(csound));
    }
    ~Notes() { csoundDestroy(csound); }

    /* A note of one k-cycle, played to its end. */
    void play()
    {
        csoundEventString(csound, "i 1 0 0.03", 0);
        for (int i = 0; i < 3; i++)
            ASSERT_EQ(0, csoundPerformKsmps(csound));
    }

    /* The blocks Csound holds after another 20 notes, less those it held
       after the first two. */
    long kept()
    {
        play();
        play();
        const size_t before = csound_tracked_block_count(csound);
        for (int i = 0; i < 20; i++)
            play();
        EXPECT_EQ(0, csoundErrCnt(csound));
        return (long) csound_tracked_block_count(csound) - (long) before;
    }

    CSOUND *csound;
};
}

/* prints copied its format for printks' use at every call, and never freed
   the copy: one block more for every note. */
TEST(PrintsFormat, KeepsNoCopyOfTheFormat)
{
    Notes notes("prints \"value %d\\n\", 1");
    EXPECT_EQ(0, notes.kept());
}

/* printks copies its format at init, to see when it changes, and each note
   of an instance replaced the copy an earlier note made without freeing it. */
TEST(PrintksFormat, FreesTheCopyAnEarlierNoteMade)
{
    Notes notes("printks \"value %d\\n\", 0, 1");
    EXPECT_EQ(0, notes.kept());
}

/* ... and so did a reinit, here one at every k-cycle of one note. */
TEST(PrintksFormat, FreesTheCopyAReinitReplaces)
{
    Notes notes("again:\nprintks \"value %d\\n\", 0, 1\nrireturn\n"
                "reinit again");
    csoundEventString(notes.csound, "i 1 0 10", 0);
    for (int i = 0; i < 3; i++)
        ASSERT_EQ(0, csoundPerformKsmps(notes.csound));
    const size_t before = csound_tracked_block_count(notes.csound);
    for (int i = 0; i < 20; i++)
        ASSERT_EQ(0, csoundPerformKsmps(notes.csound));
    EXPECT_EQ(0, (long) csound_tracked_block_count(notes.csound) - (long) before);
    EXPECT_EQ(0, csoundErrCnt(notes.csound));
}
