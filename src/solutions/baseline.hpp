#pragma once
#include "utils.hpp"

/// Binary search implemented to return same result as std::lower_bound
/// When there are multiple values of the searched, it will return index of the first one
/// When the searched value is not found, it will return -1
/// @param hayStack - the input data that will be searched in
/// @param needles - the values that will be searched
/// @param indices - the indices of the needles (or -1 if the needle is not found)
static void binarySearch(const AlignedIntArray &hayStack, const AlignedIntArray &needles, AlignedIntArray &indices) {
	for (int c = 0; c < needles.getCount(); c++) {
		const int value = needles[c];

		int left = 0;
		int count = hayStack.getCount();

		while (count > 0) {
			const int half = count / 2;

			if (hayStack[left + half] < value) {
				left = left + half + 1;
				count -= half + 1;
			} else {
				count = half;
			}
		}

		if (left < hayStack.count && hayStack[left] == value) {
			indices[c] = left;
		} else {
			indices[c] = -1;
		}
	}
}

static void binarySearch(
    const AlignedIntArray &hayStack,
    const AlignedIntArray &needles,
    AlignedIntArray &indices,
    StackAllocator &allocator)
{
	binarySearch(hayStack, needles, indices);
}