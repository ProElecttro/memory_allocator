#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <random>
#include <vector>

// The implementation file currently contains its own main(). Rename it while
// including the implementation so this test file can provide the test runner.
#define main allocator_demo_main
#include "contiguous_memory_allocator_google.cpp"
#undef main

class MemoryAllocatorTest : public ::testing::Test {
protected:
    static constexpr int kPoolSize = 1000;
    int pool[kPoolSize]{};
    MemoryAllocator allocator{pool, kPoolSize};
};

TEST_F(MemoryAllocatorTest, FirstAllocationStartsAtZero) {
    EXPECT_EQ(allocator.Submit({100, 0, ALLOCATE}), 0);
}

TEST_F(MemoryAllocatorTest, AllocationsAreContiguousAndNonOverlapping) {
    const int a = allocator.Submit({100, 0, ALLOCATE});
    const int b = allocator.Submit({200, 0, ALLOCATE});
    const int c = allocator.Submit({50, 0, ALLOCATE});

    ASSERT_EQ(a, 0);
    ASSERT_EQ(b, 100);
    ASSERT_EQ(c, 300);

    EXPECT_LT(a + 100, b + 200);
    EXPECT_LE(a + 100, b);
    EXPECT_LE(b + 200, c);
}

TEST_F(MemoryAllocatorTest, AllocationCanUseEntirePool) {
    EXPECT_EQ(allocator.Submit({kPoolSize, 0, ALLOCATE}), 0);
    EXPECT_EQ(allocator.Submit({1, 0, ALLOCATE}), -1);
}

TEST_F(MemoryAllocatorTest, ReturnsFailureWhenNoContiguousBlockIsLargeEnough) {
    EXPECT_EQ(allocator.Submit({400, 0, ALLOCATE}), 0);
    EXPECT_EQ(allocator.Submit({400, 0, ALLOCATE}), 400);
    EXPECT_EQ(allocator.Submit({201, 0, ALLOCATE}), -1);
}

TEST_F(MemoryAllocatorTest, ReusesFreedBlock) {
    const int a = allocator.Submit({100, 0, ALLOCATE});
    const int b = allocator.Submit({200, 0, ALLOCATE});

    ASSERT_EQ(a, 0);
    ASSERT_EQ(b, 100);
    EXPECT_EQ(allocator.Submit({a, 100, DEALLOCATE}), a);
    EXPECT_EQ(allocator.Submit({50, 0, ALLOCATE}), a);
}

TEST_F(MemoryAllocatorTest, CoalescesAdjacentFreedBlocks) {
    const int a = allocator.Submit({100, 0, ALLOCATE});
    const int b = allocator.Submit({100, 0, ALLOCATE});
    const int c = allocator.Submit({100, 0, ALLOCATE});
    ASSERT_EQ(a, 0);
    ASSERT_EQ(b, 100);
    ASSERT_EQ(c, 200);

    allocator.Submit({a, 100, DEALLOCATE});
    allocator.Submit({b, 100, DEALLOCATE});

    // The two adjacent 100-unit blocks should merge into a 200-unit block.
    EXPECT_EQ(allocator.Submit({200, 0, ALLOCATE}), 0);
}

TEST_F(MemoryAllocatorTest, CoalescesBlocksFreedInReverseOrder) {
    const int a = allocator.Submit({100, 0, ALLOCATE});
    const int b = allocator.Submit({100, 0, ALLOCATE});
    ASSERT_EQ(a, 0);
    ASSERT_EQ(b, 100);

    allocator.Submit({b, 100, DEALLOCATE});
    allocator.Submit({a, 100, DEALLOCATE});

    EXPECT_EQ(allocator.Submit({200, 0, ALLOCATE}), 0);
}

TEST_F(MemoryAllocatorTest, CoalescesThreeAdjacentFreeBlocks) {
    const int a = allocator.Submit({100, 0, ALLOCATE});
    const int b = allocator.Submit({100, 0, ALLOCATE});
    const int c = allocator.Submit({100, 0, ALLOCATE});
    ASSERT_EQ(a, 0);
    ASSERT_EQ(b, 100);
    ASSERT_EQ(c, 200);

    allocator.Submit({a, 100, DEALLOCATE});
    allocator.Submit({b, 100, DEALLOCATE});
    allocator.Submit({c, 100, DEALLOCATE});

    EXPECT_EQ(allocator.Submit({300, 0, ALLOCATE}), 0);
}

TEST_F(MemoryAllocatorTest, DoesNotTreatSeparatedFreeBlocksAsContiguous) {
    const int a = allocator.Submit({100, 0, ALLOCATE});
    const int b = allocator.Submit({100, 0, ALLOCATE});
    const int c = allocator.Submit({100, 0, ALLOCATE});
    ASSERT_EQ(a, 0);
    ASSERT_EQ(b, 100);
    ASSERT_EQ(c, 200);

    allocator.Submit({a, 100, DEALLOCATE});
    allocator.Submit({c, 100, DEALLOCATE});

    // 150 units cannot fit in either 100-unit hole, even though total free
    // space is greater than 150.
    EXPECT_EQ(allocator.Submit({150, 0, ALLOCATE}), 300);
}

TEST_F(MemoryAllocatorTest, RepeatedAllocateAndFreeCycles) {
    for (int i = 0; i < 20; ++i) {
        const int offset = allocator.Submit({25, 0, ALLOCATE});
        ASSERT_GE(offset, 0);
        EXPECT_EQ(allocator.Submit({offset, 25, DEALLOCATE}), offset);
    }

    EXPECT_EQ(allocator.Submit({kPoolSize, 0, ALLOCATE}), 0);
}

TEST_F(MemoryAllocatorTest, SubmitRejectsUnknownRequestType) {
    Request req{10, 0, static_cast<ReqType>(99)};
    EXPECT_EQ(allocator.Submit(req), -1);
}

// These tests describe expected defensive behavior. They will fail until
// allocate()/deallocate() validate request sizes and pool boundaries.
TEST_F(MemoryAllocatorTest, RejectsZeroSizedAllocation) {
    EXPECT_EQ(allocator.Submit({0, 0, ALLOCATE}), -1);
}

TEST_F(MemoryAllocatorTest, RejectsNegativeSizedAllocation) {
    EXPECT_EQ(allocator.Submit({-1, 0, ALLOCATE}), -1);
}

TEST_F(MemoryAllocatorTest, RejectsFreeOutsidePool) {
    EXPECT_EQ(allocator.Submit({kPoolSize, 1, DEALLOCATE}), -1);
    EXPECT_EQ(allocator.Submit({-1, 0, DEALLOCATE}), -1);
    EXPECT_EQ(allocator.Submit({10, -1, DEALLOCATE}), -1);
}

TEST_F(MemoryAllocatorTest, RejectsOverlappingDoubleFree) {
    const int offset = allocator.Submit({100, 0, ALLOCATE});
    ASSERT_EQ(offset, 0);

    EXPECT_EQ(allocator.Submit({offset, 100, DEALLOCATE}), offset);
    EXPECT_EQ(allocator.Submit({offset, 100, DEALLOCATE}), -1);
}

TEST_F(MemoryAllocatorTest, CanAllocateWholePoolAfterAllBlocksAreFreed) {
    const int a = allocator.Submit({250, 0, ALLOCATE});
    const int b = allocator.Submit({250, 0, ALLOCATE});
    const int c = allocator.Submit({500, 0, ALLOCATE});
    ASSERT_EQ(a, 0);
    ASSERT_EQ(b, 250);
    ASSERT_EQ(c, 500);

    allocator.Submit({b, 250, DEALLOCATE});
    allocator.Submit({a, 250, DEALLOCATE});
    allocator.Submit({c, 500, DEALLOCATE});

    EXPECT_EQ(allocator.Submit({kPoolSize, 0, ALLOCATE}), 0);
}

TEST_F(MemoryAllocatorTest, RandomizedAllocationFreeSequenceMaintainsValidOffsets) {
    std::mt19937 rng(123456);
    std::uniform_int_distribution<int> sizeDist(1, 80);
    std::bernoulli_distribution allocateDist(0.6);

    struct LiveAllocation {
        int offset;
        int size;
    };

    std::vector<LiveAllocation> live;

    for (int step = 0; step < 500; ++step) {
        if (live.empty() || allocateDist(rng)) {
            const int size = sizeDist(rng);
            const int offset = allocator.Submit({size, 0, ALLOCATE});
            if (offset >= 0) {
                EXPECT_LE(offset, kPoolSize - size);
                for (const auto& allocation : live) {
                    const bool disjoint =
                        offset + size <= allocation.offset ||
                        allocation.offset + allocation.size <= offset;
                    EXPECT_TRUE(disjoint) << "Overlapping allocations at step " << step;
                }
                live.push_back({offset, size});
            }
        } else {
            const std::size_t index = rng() % live.size();
            const auto allocation = live[index];
            EXPECT_EQ(allocator.Submit(
                          {allocation.offset, allocation.size, DEALLOCATE}),
                      allocation.offset);
            live.erase(live.begin() + static_cast<std::ptrdiff_t>(index));
        }
    }
}
