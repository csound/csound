#include "gtest/gtest.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <future>
#include <thread>
#include <vector>

extern "C" {
void csoundTestAtomicSpinLock(int32_t *);
void csoundTestAtomicSpinUnlock(int32_t *);
int csoundTestAtomicSpinTryLock(int32_t *);
}

TEST(AtomicSpinlockTests, ContenderWaitsForOwnerToRelease)
{
    int32_t lock = 0;
    csoundTestAtomicSpinLock(&lock);
    std::promise<void> attempting;
    std::promise<void> acquired;
    auto attemptingFuture = attempting.get_future();
    auto acquiredFuture = acquired.get_future();
    std::thread contender([&] {
        attempting.set_value();
        csoundTestAtomicSpinLock(&lock);
        acquired.set_value();
        csoundTestAtomicSpinUnlock(&lock);
    });
    attemptingFuture.wait();
    EXPECT_EQ(acquiredFuture.wait_for(std::chrono::milliseconds(100)),
              std::future_status::timeout);
    csoundTestAtomicSpinUnlock(&lock);
    contender.join();
    EXPECT_EQ(acquiredFuture.wait_for(std::chrono::seconds(0)),
              std::future_status::ready);
}

TEST(AtomicSpinlockTests, UnlockClearsTheWholeLock)
{
    int32_t lock = 0x01010101;
    csoundTestAtomicSpinUnlock(&lock);
    EXPECT_EQ(lock, 0);
    EXPECT_EQ(csoundTestAtomicSpinTryLock(&lock), 1);
    EXPECT_EQ(csoundTestAtomicSpinTryLock(&lock), 0);
    csoundTestAtomicSpinUnlock(&lock);
    EXPECT_EQ(csoundTestAtomicSpinTryLock(&lock), 1);
    csoundTestAtomicSpinUnlock(&lock);
}

TEST(AtomicSpinlockTests, ContendingWritersHaveOneOwner)
{
    int32_t lock = 0;
    std::atomic<int> owners{0};
    std::atomic<int> collisions{0};
    std::atomic<bool> start{false};
    std::vector<std::thread> workers;
    for (int index = 0; index < 4; ++index) {
        workers.emplace_back([&] {
            while (!start.load()) std::this_thread::yield();
            for (int turn = 0; turn < 10000; ++turn) {
                csoundTestAtomicSpinLock(&lock);
                if (owners.fetch_add(1) != 0) ++collisions;
                owners.fetch_sub(1);
                csoundTestAtomicSpinUnlock(&lock);
            }
        });
    }
    start = true;
    for (auto &worker : workers) worker.join();
    EXPECT_EQ(collisions.load(), 0);
    EXPECT_EQ(owners.load(), 0);
}
