#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <cstdint>
#include <cstring>
#include <limits>

namespace {
class AlignedAllocationTests : public ::testing::Test {
protected:
    CSOUND *csound;
    INSDS instance {};
    AUXCH aux {};

    void SetUp() override {
        csound = csoundCreate(nullptr, nullptr);
        csoundCreateMessageBuffer(csound, 0);
    }

    void TearDown() override {
        csound->curip = nullptr;
        csoundDestroy(csound);
    }
};

TEST_F(AlignedAllocationTests, AlignsAndClearsTheReturnedBytes)
{
    for (size_t alignment : {1, 2, 4, 8, 16, 32, 64, 256, 4096}) {
        for (size_t size : {1, 3, 80, 257}) {
            SCOPED_TRACE(alignment);
            SCOPED_TRACE(size);
            auto *data = static_cast<unsigned char *>(
                csound->CallocAligned(csound, size, alignment));
            ASSERT_NE(nullptr, data);
            EXPECT_EQ(0u, reinterpret_cast<uintptr_t>(data) % alignment);
            for (size_t i = 0; i < size; ++i) EXPECT_EQ(0, data[i]);
            memset(data, 0x5a, size);
            csound->Free(csound, data);
        }
    }
}

TEST_F(AlignedAllocationTests, ReallocPreservesAlignmentAndContents)
{
    for (size_t alignment : {16, 64, 4096}) {
        // Mix ordinary and aligned blocks to exercise both list neighbours.
        void *before = csound->Malloc(csound, 9);
        auto *data = static_cast<unsigned char *>(
            csound->CallocAligned(csound, 80, alignment));
        void *after = csound->Calloc(csound, 13);
        memset(data, 0x5a, 80);
        data = static_cast<unsigned char *>(csound->ReAlloc(csound, data, 513));
        EXPECT_EQ(0u, reinterpret_cast<uintptr_t>(data) % alignment);
        for (size_t i = 0; i < 80; ++i) EXPECT_EQ(0x5a, data[i]);
        memset(data, 0x3c, 513);
        data = static_cast<unsigned char *>(csound->ReAlloc(csound, data, 3));
        EXPECT_EQ(0u, reinterpret_cast<uintptr_t>(data) % alignment);
        for (size_t i = 0; i < 3; ++i) EXPECT_EQ(0x3c, data[i]);
        csound->Free(csound, before);
        csound->Free(csound, after);
        EXPECT_EQ(nullptr, csound->ReAlloc(csound, data, 0));
    }
}

TEST_F(AlignedAllocationTests, AuxiliaryBufferHonoursAChangedAlignment)
{
    csound->curip = &instance;
    csound->AuxAlloc(csound, 80, &aux);

    // Choose an alignment the existing buffer does not already have.
    size_t alignment = 32;
    while (reinterpret_cast<uintptr_t>(aux.auxp) % alignment == 0) {
        ASSERT_LE(alignment, std::numeric_limits<size_t>::max() / 2);
        alignment *= 2;
    }
    memset(aux.auxp, 0x5a, 80);
    csound->AuxAllocAligned(csound, 80, alignment, &aux);
    EXPECT_EQ(0u, reinterpret_cast<uintptr_t>(aux.auxp) % alignment);
    EXPECT_EQ(80u, aux.size);
    EXPECT_EQ(static_cast<char *>(aux.auxp) + 80, aux.endp);
    for (size_t i = 0; i < 80; ++i)
        EXPECT_EQ(0, static_cast<unsigned char *>(aux.auxp)[i]);

    void *previous = aux.auxp;
    memset(aux.auxp, 0x5a, 80);
    csound->AuxAllocAligned(csound, 80, alignment, &aux);
    EXPECT_EQ(previous, aux.auxp);
    EXPECT_EQ(0, static_cast<unsigned char *>(aux.auxp)[79]);
    csound->AuxAllocAligned(csound, 257, alignment, &aux);
    EXPECT_EQ(0u, reinterpret_cast<uintptr_t>(aux.auxp) % alignment);
    EXPECT_EQ(static_cast<char *>(aux.auxp) + 257, aux.endp);
    EXPECT_EQ(&aux, instance.auxchp);
    EXPECT_EQ(nullptr, aux.nxtchp);
}

TEST_F(AlignedAllocationTests, OrdinaryReallocStillPreservesContents)
{
    auto *data = static_cast<unsigned char *>(csound->Malloc(csound, 80));
    void *aligned = csound->CallocAligned(csound, 3, 64);
    memset(data, 0x5a, 80);
    data = static_cast<unsigned char *>(csound->ReAlloc(csound, data, 513));
    for (size_t i = 0; i < 80; ++i) EXPECT_EQ(0x5a, data[i]);
    csound->Free(csound, aligned);
    csound->Free(csound, data);
}

TEST_F(AlignedAllocationTests, RejectsInvalidAlignmentAndSizeOverflow)
{
    const size_t maxSize = std::numeric_limits<size_t>::max();
    const size_t requests[][2] = {{80, 0}, {80, 3}, {0, 0}, {0, 3}, {80, maxSize},
                                 {maxSize, 64}};
    for (const auto &request : requests) {
        // Allocation errors use Csound's normal longjmp exit path.
        if (setjmp(csound->exitjmp) == 0) {
            void *data = csound->CallocAligned(csound, request[0], request[1]);
            csound->Free(csound, data);
            FAIL() << "CallocAligned accepted an invalid request";
        }
        csound->engineStatus &= ~CS_STATE_JMP;
    }
}

TEST_F(AlignedAllocationTests, FailedAuxiliaryAllocationKeepsTheOldBuffer)
{
    csound->curip = &instance;
    csound->AuxAllocAligned(csound, 80, 64, &aux);
    void *previous = aux.auxp;
    memset(previous, 0x5a, 80);
    const size_t requests[][2] = {{80, 0}, {80, 3}, {81, 3},
                                 {std::numeric_limits<size_t>::max(), 64}};
    for (const auto &request : requests) {
        if (setjmp(csound->exitjmp) == 0) {
            csound->AuxAllocAligned(csound, request[0], request[1], &aux);
            FAIL() << "AuxAllocAligned accepted an invalid request";
        }
        csound->engineStatus &= ~CS_STATE_JMP;
        ASSERT_EQ(previous, aux.auxp);
        EXPECT_EQ(80u, aux.size);
        EXPECT_EQ(static_cast<char *>(previous) + 80, aux.endp);
        EXPECT_EQ(&aux, instance.auxchp);
        EXPECT_EQ(nullptr, aux.nxtchp);
        for (size_t i = 0; i < 80; ++i)
            EXPECT_EQ(0x5a, static_cast<unsigned char *>(aux.auxp)[i]);
    }
}

TEST_F(AlignedAllocationTests, FailedAuxiliaryAllocationDoesNotLinkAnEmptyBuffer)
{
    csound->curip = &instance;
    if (setjmp(csound->exitjmp) == 0) {
        csound->AuxAllocAligned(csound, 80, 3, &aux);
        FAIL() << "AuxAllocAligned accepted an invalid alignment";
    }
    csound->engineStatus &= ~CS_STATE_JMP;
    EXPECT_EQ(nullptr, instance.auxchp);
    EXPECT_EQ(nullptr, aux.auxp);
    EXPECT_EQ(nullptr, aux.endp);
    EXPECT_EQ(0u, aux.size);
}

TEST_F(AlignedAllocationTests, OrdinaryAllocationsRejectSizeOverflow)
{
    for (auto allocate : {csound->Malloc, csound->Calloc}) {
        if (setjmp(csound->exitjmp) == 0) {
            void *data = allocate(csound, std::numeric_limits<size_t>::max());
            csound->Free(csound, data);
            FAIL() << "Allocation accepted a size that overflows";
        }
        csound->engineStatus &= ~CS_STATE_JMP;
    }
}

TEST_F(AlignedAllocationTests, FailedReallocLeavesTheOldBlockUsable)
{
    for (bool aligned : {false, true}) {
        auto *data = static_cast<unsigned char *>(aligned
            ? csound->CallocAligned(csound, 80, 4096)
            : csound->Malloc(csound, 80));
        memset(data, 0x5a, 80);
        const size_t maxSize = std::numeric_limits<size_t>::max();
        // The second request fits the ordinary header but not aligned padding.
        const size_t requests[] = {maxSize, aligned ? maxSize - 128 : maxSize - 1};
        for (size_t size : requests) {
            if (setjmp(csound->exitjmp) == 0) {
                csound->ReAlloc(csound, data, size);
                FAIL() << "ReAlloc accepted a size that overflows";
            }
            csound->engineStatus &= ~CS_STATE_JMP;
            for (size_t i = 0; i < 80; ++i) EXPECT_EQ(0x5a, data[i]);
        }
        // A valid resize must still work after the rejected request.
        data = static_cast<unsigned char *>(csound->ReAlloc(csound, data, 160));
        for (size_t i = 0; i < 80; ++i) EXPECT_EQ(0x5a, data[i]);
        csound->Free(csound, data);
    }
}

TEST_F(AlignedAllocationTests, ResetReleasesMixedAllocations)
{
    ASSERT_EQ(0, csoundCompileOrc(csound, "instr 1\nendin\n", 0));
    csound->Malloc(csound, 17);
    csound->CallocAligned(csound, 80, 64);
    void *data = csound->CallocAligned(csound, 3, 4096);
    csound->ReAlloc(csound, data, 257);
    csound->Calloc(csound, 11);
    csoundReset(csound);
    data = csound->CallocAligned(csound, 80, 64);
    EXPECT_EQ(0u, reinterpret_cast<uintptr_t>(data) % 64);
    csound->Free(csound, data);
}
}
