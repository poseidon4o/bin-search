#if _WIN64
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstdlib>
#include <algorithm>
#include <random>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <filesystem>

#include "utils.hpp"
#include "solution-picker.hpp"

const int HEAP_SIZE = (1 << 24) + 1;


int main() {
	
	printf("+ Correctness tests ... \n");

	const int64_t searches = 400ll * (1 << 26);

	// enumerate and run correctness test
	int testCaseCount = 0;
	for (int r = 0; /*no-op*/; r++) {
		AlignedArrayPtr<int> hayStack;
		AlignedArrayPtr<int> needles;
		char fname[64] = { 0, };
		snprintf(fname, sizeof(fname), "%d.bsearch", r);

		if (!loadFromFile(hayStack, needles, fname)) {
			break;
		}

		printf("Checking %s... ", fname);

		AlignedArrayPtr<int> indices(needles.getCount());
		AlignedArrayPtr<uint8_t> heap(HEAP_SIZE);

		StackAllocator allocator(heap, HEAP_SIZE);
		{
			indices.memset(NOT_SEARCHED);
			allocator.zeroAll();
			TEST_SEARCH(hayStack, needles, indices, allocator);
			if (verify(hayStack, needles, indices) != -1) {
				printf("Failed to verify base betterSearch!\n");
				return -1;
			}

			indices.memset(NOT_SEARCHED);
			binarySearch(hayStack, needles, indices);
			if (verify(hayStack, needles, indices) != -1) {
				printf("Failed to verify base binarySearch!\n");
				return -1;
			}
		}
		printf("OK\n");
		++testCaseCount;
	}

	printf("Speed tests for %s ... \n", TEST_NAME);

	for (int r = 0; r < testCaseCount; r++) {
		AlignedArrayPtr<int> hayStack;
		AlignedArrayPtr<int> needles;
		char fname[64] = { 0, };
		snprintf(fname, sizeof(fname), "%d.bsearch", r);

		if (!loadFromFile(hayStack, needles, fname)) {
			printf("Failed to load %s for speed test, continuing\n", fname);
			continue;
		}

		const int testRepeat = 200;
		//printf("Running speed test for %s, %d repeats ", fname, testRepeat);

		AlignedArrayPtr<int> indices(needles.getCount());
		AlignedArrayPtr<uint8_t> heap(HEAP_SIZE);

		StackAllocator allocator(heap, HEAP_SIZE);
		uint64_t t0;
		uint64_t t1;
		uint64_t bestBinary = -1, bestBetter = -1;

		// Time the binary search and take average of the runs
		{
			indices.memset(NOT_SEARCHED);
			t0 = timer_nsec();
			for (int test = 0; test < testRepeat; ++test) {
				const uint64_t start = timer_nsec();
                                binarySearch(
                                    hayStack, needles, indices);
				const uint64_t end = timer_nsec();
				bestBinary = std::min(bestBinary, end - start);
			}
			t1 = timer_nsec();
		}

		const double totalBinary = (double(t1 - t0) * 1e-9) / testRepeat;

		// Time the better search and take average of the runs
		{
			indices.memset(NOT_SEARCHED);
			t0 = timer_nsec();
			for (int test = 0; test < testRepeat; ++test) {
				const uint64_t start = timer_nsec();
				TEST_SEARCH(hayStack, needles, indices, allocator);
				const uint64_t end = timer_nsec();
				bestBetter = std::min(bestBetter, end - start);
			}
			t1 = timer_nsec();
		}

		const double totalBetter = (double(t1 - t0) * 1e-9) / testRepeat;
		printf("Test %d best speedup [%f] average speedup [%f]\n", r + 1, double(bestBinary) / bestBetter, double(totalBinary) / totalBetter);
	}
	return 0;
}
