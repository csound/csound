#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <cstdlib>
#include <string>
#include <unordered_set>
#include <vector>

extern "C" {
#include "csound_orc.h"
#include "csound_orc_compile.h"
#include "csound_orc_semantics.h"
OENTRY *resolve_opcode(CSOUND *, OENTRIES *, char *, char *);
OENTRY *resolve_opcode_exact(CSOUND *, OENTRIES *, const char *, const char *);
}

class OpcodeSignatureTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;
    void SetUp() override {
        csound = csoundCreate(nullptr, nullptr);
        csoundCreateMessageBuffer(csound, 0);
    }
    void TearDown() override { csoundDestroy(csound); }
};

// Plain allocations give ASan a redzone at the payload boundary. Csound's
// normal allocation header can hide an access just before the payload.
class SignatureMemory {
    CSOUND *csound;
    decltype(CSOUND::Malloc) savedMalloc;
    decltype(CSOUND::Free) savedFree;
    static thread_local SignatureMemory *active;
    static void *allocate(CSOUND *, size_t bytes) {
        void *p = std::malloc(bytes);
        active->live.insert(p);
        active->allocations++;
        return p;
    }
    static void release(CSOUND *, void *p) {
        if (p == nullptr) return;
        EXPECT_EQ(1u, active->live.erase(p));
        std::free(p);
    }
public:
    std::unordered_set<void *> live;
    size_t allocations = 0;
    explicit SignatureMemory(CSOUND *cs)
        : csound(cs), savedMalloc(cs->Malloc), savedFree(cs->Free) {
        active = this;
        cs->Malloc = allocate;
        cs->Free = release;
    }
    ~SignatureMemory() {
        csound->Malloc = savedMalloc;
        csound->Free = savedFree;
        active = nullptr;
        for (void *p : live) std::free(p);
    }
};
thread_local SignatureMemory *SignatureMemory::active = nullptr;

TEST_F(OpcodeSignatureTests, EmptyAndMissingSignatures) {
    SignatureMemory memory(csound);
    const char *emptySignatures[] = {nullptr, ""};
    for (const char *empty : emptySignatures) {
        for (const char *other : emptySignatures) {
            EXPECT_EQ(1, check_in_args(csound, empty, other));
            EXPECT_EQ(1, check_out_args(csound, empty, other));
        }
        for (const char *nonempty : {"a", "S", "a[]", ":Voice;[]"}) {
            EXPECT_EQ(0, check_in_args(csound, nonempty, empty));
            EXPECT_EQ(0, check_out_args(csound, nonempty, empty));
            EXPECT_EQ(0, check_in_args(csound, empty, nonempty));
            EXPECT_EQ(0, check_out_args(csound, empty, nonempty));
        }
        EXPECT_EQ(1, check_in_args(csound, empty, "op"));
        EXPECT_EQ(1, check_in_args(csound, empty, "N"));
        EXPECT_EQ(1, check_out_args(csound, empty, "N"));
        EXPECT_EQ(0, check_in_arg(empty, "i"));
        EXPECT_EQ(0, check_out_arg(empty, "i"));
    }
    EXPECT_EQ(0, check_in_arg("", ""));
    EXPECT_EQ(0, check_out_arg("", ""));
    EXPECT_TRUE(memory.live.empty());
}

TEST_F(OpcodeSignatureTests, RejectMalformedDescriptorsBeforeAllocation) {
    SignatureMemory memory(csound);
    for (const char *bad : {"[", "[]", "[a", "a[", "a[[]", "aa[",
                            "[a]]", ":", ":Voice", ":;", ":Voice[];",
                            ":Voice:;", "[:Voice]", "a:Voice", "a]", ";",
                            "a [a]", "[:Voice;", "a[][][", "a a", "a\ta",
                            "a\na", "a\ra", "a\fa", "a\va"}) {
        SCOPED_TRACE(bad);
        EXPECT_EQ(-1, args_required(bad));
        EXPECT_EQ(nullptr, split_args(csound, bad));
        EXPECT_EQ(0, check_in_args(csound, bad, "N"));
        EXPECT_EQ(0, check_in_args(csound, bad, "*"));
        EXPECT_EQ(0, check_in_args(csound, bad, bad));
        EXPECT_EQ(0, check_in_args(csound, "a", bad));
        EXPECT_EQ(0, check_out_args(csound, bad, "N"));
        EXPECT_EQ(0, check_out_args(csound, "a", bad));
        EXPECT_EQ(0u, memory.allocations);
    }
}

TEST_F(OpcodeSignatureTests, SplitBothArrayFormatsAndNamedTypes) {
    const struct {
        const char *signature;
        std::vector<const char *> tokens;
    } cases[] = {
        {nullptr, {}}, {"", {}}, {"0", {"0"}},
        {"ik[]:Voice;[]", {"i", "[k]", "[:Voice;]"}},
        {"[[a][:Voice;]i", {"[[a]", "[:Voice;]", "i"}},
        {":Voice;[][]:Voice2;", {"[[:Voice;]", ":Voice2;"}},
        {".[]?[][]*[]", {"[.]", "[[?]", "[*]"}},
    };
    SignatureMemory memory(csound);
    for (const auto &item : cases) {
        SCOPED_TRACE(item.signature == nullptr ? "NULL" : item.signature);
        EXPECT_EQ(item.tokens.size(), static_cast<size_t>(args_required(item.signature)));
        char **parts = split_args(csound, item.signature);
        ASSERT_NE(nullptr, parts);
        for (size_t i = 0; i < item.tokens.size(); i++) {
            EXPECT_STREQ(item.tokens[i], parts[i]);
            EXPECT_EQ(1, args_required(parts[i]));
        }
        EXPECT_EQ(nullptr, parts[item.tokens.size()]);
        free_split_args(csound, parts);
        EXPECT_TRUE(memory.live.empty());
    }
}

TEST_F(OpcodeSignatureTests, NamedArrayTypesMustMatchInFull) {
    SignatureMemory memory(csound);
    for (const char *found : {":Voice;[]", "[:Voice;]", ":Voice;[][]"}) {
        for (const char *required : {":Voice;[]", "[:Voice;]"}) {
            EXPECT_EQ(1, check_in_args(csound, found, required));
            EXPECT_EQ(1, check_out_args(csound, found, required));
        }
        for (const char *wrong : {":Other;[]", ":Voice2;[]", "[:Other;]", "i[]"}) {
            EXPECT_EQ(0, check_in_args(csound, found, wrong));
            EXPECT_EQ(0, check_out_args(csound, found, wrong));
        }
        for (const char *any : {".[]", "?[]", "*[]"}) {
            EXPECT_EQ(1, check_in_args(csound, found, any));
            EXPECT_EQ(1, check_out_args(csound, found, any));
        }
    }
    EXPECT_TRUE(memory.live.empty());
}

TEST_F(OpcodeSignatureTests, PreserveArrayRankAndInputRateRules) {
    EXPECT_EQ(1, check_in_args(csound, "a[][]", "a[]"));
    EXPECT_EQ(1, check_out_args(csound, "a[][]", "a[]"));
    EXPECT_EQ(1, check_in_args(csound, "i[][]", "k[]"));
    EXPECT_EQ(0, check_out_args(csound, "i[][]", "k[]"));
    EXPECT_EQ(0, check_in_args(csound, "k[]", "i[][]"));
    EXPECT_EQ(0, check_in_args(csound, "a", "a[]"));
    EXPECT_EQ(0, check_out_args(csound, "a[]", "a"));
}

TEST_F(OpcodeSignatureTests, FailedMatchesReleaseAllTokens) {
    SignatureMemory memory(csound);
    for (int i = 0; i < 100; i++) {
        for (const char *required : {"a", "a[]", ":Voice;[]", "aa"}) {
            EXPECT_EQ(0, check_in_args(csound, "aaa", required));
            EXPECT_EQ(0, check_out_args(csound, "aaa", required));
            EXPECT_TRUE(memory.live.empty());
        }
        EXPECT_EQ(0, check_in_args(csound, "S", "ai"));
        EXPECT_EQ(0, check_out_args(csound, "S", "ai"));
        EXPECT_TRUE(memory.live.empty());
    }
}

TEST_F(OpcodeSignatureTests, GeneratedDescriptorsStayWithinTheirAllocations) {
    SignatureMemory memory(csound);
    const std::string alphabet = "a:;[]";
    size_t combinations = 1;
    for (int length = 0; length <= 5; length++, combinations *= alphabet.size()) {
        for (size_t index = 0; index < combinations; index++) {
            std::string signature;
            size_t value = index;
            for (int i = 0; i < length; i++, value /= alphabet.size())
                signature += alphabet[value % alphabet.size()];
            SCOPED_TRACE(signature);
            int32_t count = args_required(signature.c_str());
            char **parts = split_args(csound, signature.c_str());
            if (count < 0) {
                EXPECT_EQ(nullptr, parts);
            } else {
                ASSERT_NE(nullptr, parts);
                for (int32_t i = 0; i < count; i++) {
                    ASSERT_NE(nullptr, parts[i]);
                    EXPECT_EQ(1, args_required(parts[i]));
                    char **again = split_args(csound, parts[i]);
                    ASSERT_NE(nullptr, again);
                    EXPECT_STREQ(parts[i], again[0]);
                    EXPECT_EQ(nullptr, again[1]);
                    free_split_args(csound, again);
                }
                EXPECT_EQ(nullptr, parts[count]);
                EXPECT_EQ(1, check_in_args(csound, signature.c_str(), signature.c_str()));
                EXPECT_EQ(1, check_out_args(csound, signature.c_str(), signature.c_str()));
                free_split_args(csound, parts);
            }
            check_in_args(csound, signature.c_str(), "a");
            check_out_args(csound, signature.c_str(), "a[]");
            EXPECT_TRUE(memory.live.empty());
        }
    }
}

TEST_F(OpcodeSignatureTests, ArgumentLimitsApplyToExactAndFunctionalCalls) {
    OENTRY entry = {};
    char name[] = "signature_limit";
    char output[] = "i";
    char variadic[] = "m";
    entry.opname = name;
    entry.outypes = output;
    OENTRIES *entries = static_cast<OENTRIES *>(
        csound->Calloc(csound, sizeof(OENTRIES) + sizeof(OENTRY *)));
    entries->count = 1;
    entries->entries[0] = &entry;
    for (int count : {VARGMAX - 1, VARGMAX, VARGMAX + 1}) {
        SCOPED_TRACE(count);
        std::string actual(static_cast<size_t>(count), 'i');
        bool allowed = count < VARGMAX;
        EXPECT_EQ(allowed ? 1 : -1, check_in_args(csound, actual.c_str(), "m"));
        entry.intypes = actual.data();
        EXPECT_EQ(allowed ? &entry : nullptr,
                  resolve_opcode_exact(csound, entries, output, actual.c_str()));
        EXPECT_EQ(allowed ? &entry : nullptr,
                  resolve_opcode(csound, entries, output, actual.data()));
        entry.intypes = variadic;
        EXPECT_EQ(allowed ? output : nullptr,
                  resolve_opcode_get_outarg(csound, entries, actual.data()));
    }
    csound->Free(csound, entries);
}

TEST_F(OpcodeSignatureTests, ExactMatchCannotBypassMalformedDescriptorChecks) {
    OENTRY entry = {};
    char name[] = "signature_bad";
    char bad[] = ":Voice";
    char good[] = "i";
    entry.opname = name;
    OENTRIES *entries = static_cast<OENTRIES *>(
        csound->Calloc(csound, sizeof(OENTRIES) + sizeof(OENTRY *)));
    entries->count = 1;
    entries->entries[0] = &entry;
    for (bool badInput : {false, true}) {
        entry.intypes = badInput ? bad : good;
        entry.outypes = badInput ? good : bad;
        EXPECT_EQ(nullptr, resolve_opcode_exact(csound, entries, entry.outypes, entry.intypes));
        EXPECT_EQ(nullptr, resolve_opcode(csound, entries, entry.outypes, entry.intypes));
        EXPECT_EQ(nullptr, resolve_opcode_get_outarg(csound, entries, entry.intypes));
    }
    csound->Free(csound, entries);
}
