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
#include "baseline.hpp"
#include "simd-avx256.hpp"



void betterSearch(const AlignedIntArray &hayStack, const AlignedIntArray &needles, AlignedIntArray &indices, StackAllocator &allocator);

static void betterSearchMine(const AlignedIntArray &hayStack, const AlignedIntArray &needles, AlignedIntArray &indices, StackAllocator &allocator) {
	const int stepCount = 10;
	int *allocBin = allocator.alloc<int>((1 << stepCount) - 1);
	int *bin = allocBin - 1;
	precomputeBin(hayStack, hayStack.count, bin, stepCount);

	for (int c = 0; c < needles.count; c++) {
		const int value = needles[c];

		int left = 0;
		int count = hayStack.count;
		int binIdx = 1;
		int step = 0;

		while (count > 0) {
			const int half = count / 2;

			const int testValue = step < stepCount ? bin[binIdx] : hayStack[left + half];
			++step;

			if (testValue < value) {
				left = left + half + 1;
				count -= half + 1;
				binIdx = binIdx * 2 + 1;
			} else {
				count = half;
				binIdx = binIdx * 2;
			}
		}

		if (hayStack[left] == value) {
			indices[c] = left;
		} else {
			indices[c] = -1;
		}
	}

	allocator.freeAll();
}

#if 0
void runSmallTest() {
	struct {
		int hCount;
		int qCount;
		DataType type;
	} testInfo = {
		8, 8, uniform,
	};
	AlignedArrayPtr<int> hayStack(testInfo.hCount);
	AlignedArrayPtr<int> needles(testInfo.qCount);
	initData(hayStack, needles, testInfo.type);
	std::sort(hayStack.get(), hayStack + hayStack.getCount());
	AlignedArrayPtr<int> indices(needles.getCount());
	AlignedArrayPtr<uint8_t> heap(1 << 13);
	StackAllocator allocator(heap, heap.count);

	indices.memset(NOT_SEARCHED);
	allocator.zeroAll();
	TEST_SEARCH(hayStack, needles, indices, allocator);
	if (verify(hayStack, needles, indices) != -1) {
		printf("Failed to verify base betterSearch!\n");
		return;
	}

	indices.memset(NOT_SEARCHED);
	binarySearch(hayStack, needles, indices);
	if (verify(hayStack, needles, indices) != -1) {
		printf("Failed to verify base binarySearch!\n");
	}
}
#endif
const int HEAP_SIZE = (1 << 14) + 1;

bool profile(int index) {

	AlignedArrayPtr<int> hayStack;
	AlignedArrayPtr<int> needles;

	char fname[64] = { 0, };
	snprintf(fname, sizeof(fname), "%d.bsearch", index);

	if (!loadFromFile(hayStack, needles, fname)) {
		return false;
	}

	printf("Checking %s... ", fname);

	AlignedArrayPtr<int> indices(needles.getCount());
	AlignedArrayPtr<uint8_t> heap(HEAP_SIZE);

	StackAllocator allocator(heap, HEAP_SIZE);
	indices.memset(NOT_SEARCHED);
	allocator.zeroAll();

	for (int c = 0; c < 10000; c++) {
		TEST_SEARCH(hayStack, needles, indices, allocator);
	}
}


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

	printf("+ Speed tests ... \n");

	for (int r = 0; r < testCaseCount; r++) {
		AlignedArrayPtr<int> hayStack;
		AlignedArrayPtr<int> needles;
		char fname[64] = { 0, };
		snprintf(fname, sizeof(fname), "%d.bsearch", r);

		if (!loadFromFile(hayStack, needles, fname)) {
			printf("Failed to load %s for speed test, continuing\n", fname);
			continue;
		}

		const int testRepeat = std::min<int64_t>(1000ll, searches / hayStack.getCount());
		printf("Running speed test for %s, %d repeats ", fname, testRepeat);

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
				binarySearch(hayStack, needles, indices);
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
		printf("speedup best [%f] average [%f]\n", double(bestBinary) / bestBetter, double(totalBinary) / totalBetter);
	}
	return 0;
}
