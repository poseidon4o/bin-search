#pragma once

#include "utils.hpp"
#include "stl.hpp"

template <int BinStepCount>
static void eytzingerSearch(
    const AlignedIntArray &hayStack,
    const AlignedIntArray &needles,
    AlignedIntArray &indices,
    StackAllocator &allocator)
{
	const int stepCount = BinStepCount;
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

template <int BinStepCount>
static void eytzingerSearchRangeCheck(const AlignedIntArray &hayStack, const AlignedIntArray &needles, AlignedIntArray &indices, StackAllocator &allocator) {
    if (needles.getCount() <= 1024) {
        return stlLowerBound(hayStack, needles, indices, allocator);
    }
	const int stepCount = BinStepCount;
	int *allocBin = allocator.alloc<int>((1 << stepCount) - 1);
	int *bin = allocBin - 1;
	precomputeBin(hayStack, hayStack.count, bin, stepCount);

	const int low = hayStack[0];
	const int high = hayStack[int(hayStack.getCount() - 1)];

	for (int c = 0; c < needles.count; c++) {
		const int value = needles[c];

		if (value < low || value > high) {
			indices[c] = -1;
			continue;
		}

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