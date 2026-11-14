#include "csound.h"
#include "gtest/gtest.h"
#include <cstdlib>
#include <optional>
#include <string>

/* csoundSetGlobalEnv() writes one table for the whole process, read by
 * csoundGetEnv(NULL, ...) and copied by every new instance.
 */

namespace {

const char *const a = "CSOUND_GLOBAL_ENV_TEST_A";
const char *const b = "CSOUND_GLOBAL_ENV_TEST_B";

std::optional<std::string> instanceValue(const char *name)
{
    CSOUND *csound = csoundCreate(nullptr, nullptr);
    const char *value = csoundGetEnv(csound, name);
    std::optional<std::string> result;
    if (value != nullptr) result = value;
    csoundDestroy(csound);
    return result;
}

class GlobalEnvTests : public ::testing::Test {
protected:
    void TearDown() override
    {
        /* One delete per entry, whatever a failed test left: 16 slots. */
        for (int slot = 0; slot < 16; slot++) {
            csoundSetGlobalEnv(a, nullptr);
            csoundSetGlobalEnv(b, nullptr);
        }
    }
};

}

/* A's slot comes before B's, and is free when B is set again. */
TEST_F(GlobalEnvTests, SettingAgainAfterAnEarlierDeleteKeepsOneEntry)
{
    ASSERT_EQ(csoundSetGlobalEnv(a, "a"), 0);
    ASSERT_EQ(csoundSetGlobalEnv(b, "old"), 0);
    ASSERT_EQ(csoundSetGlobalEnv(a, nullptr), 0);

    ASSERT_EQ(csoundSetGlobalEnv(b, "new"), 0);
    EXPECT_STREQ(csoundGetEnv(nullptr, b), "new");
    EXPECT_EQ(instanceValue(b), "new");

    ASSERT_EQ(csoundSetGlobalEnv(b, nullptr), 0);
    /* Gone from the table, so it falls through to the process's own. */
    EXPECT_STREQ(csoundGetEnv(nullptr, b), std::getenv(b));
    EXPECT_EQ(instanceValue(b), std::nullopt);
    EXPECT_NE(csoundSetGlobalEnv(b, nullptr), 0);
}

/* Every instance copies the table through the check --env makes, and a name
 * that check refuses made every later csoundCreate() die. */
TEST_F(GlobalEnvTests, ANameAnInstanceRefusesIsRefused)
{
    const char *const refused[] = {
        "1CSOUND_GLOBAL_ENV_TEST", "CSOUND_GLOBAL_ENV_TEST=X",
        "CSOUND-GLOBAL-ENV-TEST", "CSOUND GLOBAL ENV TEST"
    };
    for (const char *name : refused) {
        const int result = csoundSetGlobalEnv(name, "x");
        if (result == 0)
            csoundSetGlobalEnv(name, nullptr);   /* or csoundCreate dies */
        EXPECT_NE(result, 0) << name;
    }

    const char *const accepted = "_CSOUND_GLOBAL_ENV_TEST_1";
    ASSERT_EQ(csoundSetGlobalEnv(accepted, "x"), 0);
    EXPECT_EQ(instanceValue(accepted), "x");
    EXPECT_EQ(csoundSetGlobalEnv(accepted, nullptr), 0);
}

/* csoundGetEnv(NULL, ...) points into the name's own slot, so a value taken
 * from it is copied onto itself: strcpy's overlap mangled it. */
TEST_F(GlobalEnvTests, AValueFromItsOwnSlotIsCopiedWhole)
{
    std::string value;
    for (int n = 0; value.size() < 400; n++)
        value += "/" + std::to_string(n);
    ASSERT_EQ(csoundSetGlobalEnv(b, value.c_str()), 0);

    ASSERT_EQ(csoundSetGlobalEnv(b, csoundGetEnv(nullptr, b) + 7), 0);
    EXPECT_STREQ(csoundGetEnv(nullptr, b), value.c_str() + 7);
}

/* A host setting back the pointer it read: a copy onto itself, which glibc's
 * strcpy survives and AddressSanitizer reports. */
TEST_F(GlobalEnvTests, ItsOwnValueSetBackIsKept)
{
    ASSERT_EQ(csoundSetGlobalEnv(b, "kept"), 0);
    ASSERT_EQ(csoundSetGlobalEnv(b, csoundGetEnv(nullptr, b)), 0);
    EXPECT_STREQ(csoundGetEnv(nullptr, b), "kept");
}
