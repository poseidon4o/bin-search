#pragma once

#include "utils.hpp"

#include <algorithm>

static void stlLowerBound(
    const AlignedIntArray &hayStack,
    const AlignedIntArray &needles,
    AlignedIntArray &indices,
    StackAllocator &allocator)
{
    for (int c = 0; c < needles.getCount(); c++) {
        const int value = needles[c];
        const int *pos = std::lower_bound(hayStack.begin(), hayStack.end(), value);
        const int idx = std::distance(hayStack.begin(), pos);

        if (idx == hayStack.getCount() || hayStack[idx] != value) {
            indices[c] = NOT_FOUND;

        } else {
            indices[c] = idx;
        }
    }
}

static void stlLowerBoundTransform(
    const AlignedIntArray &hayStack,
    const AlignedIntArray &needles,
    AlignedIntArray &indices,
    StackAllocator &allocator)
{
    std::transform(needles.begin(), needles.end(), indices.begin(), [&hayStack](int value) {
        const int idx = std::distance(
            hayStack.begin(), std::lower_bound(hayStack.begin(), hayStack.end(), value));

        if (idx == hayStack.getCount() || hayStack[idx] != value) {
            return NOT_FOUND;
        } else {
            return idx;
        }
    });
}

static void stlRanges(
    const AlignedIntArray &hayStack,
    const AlignedIntArray &needles,
    AlignedIntArray &indices,
    StackAllocator &allocator)
{
    std::ranges::transform(needles, indices.begin(), [&hayStack](int value) {
        const int idx = std::distance(hayStack.begin(), std::ranges::lower_bound(hayStack, value));

        if (idx == hayStack.getCount() || hayStack[idx] != value) {
            return NOT_FOUND;
        } else {
            return idx;
        }
    });
}
