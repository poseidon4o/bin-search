#pragma once

#ifndef __clang__
#include <immintrin.h>
#endif


#include "utils.hpp"



inline __m256i masked_blend(__m256i a, __m256i b, __m256i mask) {
	return _mm256_castps_si256(
		_mm256_blendv_ps(
			_mm256_castsi256_ps(b),
			_mm256_castsi256_ps(a),
			_mm256_castsi256_ps(mask)
		)
	);
}

inline void betterSearchSIMD(const AlignedIntArray &hayStack, const AlignedIntArray &needles, AlignedIntArray &indices, StackAllocator &allocator) {
	const int haystackCount = int(hayStack.count);
	const int needlesCount = int(needles.count);
	const int lowCut = hayStack[0];
	const int highCut = hayStack[haystackCount - 1];

	if (lowCut == highCut) {
		const int resultIndex = lowCut == needles[0] ? 0 : NOT_FOUND;
		for (int c = 0; c < indices.count; c++) {
			indices[c] = resultIndex;
		}
		return;
	}

	const bool useSIMD = (haystackCount > (1024 * 100)) && (needlesCount > 1024);
	const int stepCount = useSIMD ? 1 : 0;
	int *indicesPtr = indices.aligned;
	const int *haystackPtr = hayStack.aligned;

	int *bin = nullptr;
	if (useSIMD) {
		const int binSize = 1 << stepCount;
		bin = allocator.alloc<int>(binSize + 1);
		precomputeBin(haystackPtr, haystackCount, bin, stepCount);
	}

	int c = 0;

	const __m256i zeros = _mm256_set1_epi32(0);
	const __m256i ones = _mm256_set1_epi32(1);
	const __m256i neg1 = _mm256_set1_epi32(-1); // all true mask
	const __m256i haystackCountV = _mm256_set1_epi32(haystackCount);

	if (useSIMD) {
		alignas(32) int needleQueue[8];
		alignas(32) int needleIndices[8];

		while (c < needlesCount) {
			int saved = c;
			int q = 0;

			// produce 8 needles not outside of haystack data range
			// implements if (value < lowCut || value > highCut) { but for 8 values
			while (q < 8 && c < needlesCount) {
				const int candidate = needles[c];
				if (candidate >= lowCut && candidate <= highCut) {
					needleQueue[q] = candidate;
					needleIndices[q] = c; // keep from where this needle is loaded
					q++;
				} else {
					indices[c] = NOT_FOUND;
				}
				c++;
			}
			// not enough needles, give up and use non simd code to finish
			if (q < 8) {
				c = saved;
				break;
			}

			const __m256i value = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(needleQueue));
			__m256i left = zeros;
			__m256i count = haystackCountV;
			__m256i binIndex = ones;
			int step = 0;

			// some lanes can finish the search earlier, keep mask of non finished searches
			__m256i positiveCountMask = _mm256_cmpgt_epi32(count, zeros);
			while (_mm256_movemask_ps(_mm256_castsi256_ps(positiveCountMask)) != 0) {
				// const int half = count / 2;
				const __m256i half = _mm256_srli_epi32(count, 1);


				// const int testValue = step < stepCount ? bin[binIdx] : hayStack[left + half];
				__m256i testValue;
				const __m256i leftHalf = _mm256_add_epi32(left, half);
				if (step < stepCount) {
					const __m256i safeBinIndex = masked_blend(binIndex, ones, positiveCountMask);
					testValue = _mm256_i32gather_epi32(bin, safeBinIndex, sizeof(int));
				} else {
					const __m256i safeArrayIndex = masked_blend(leftHalf, zeros, positiveCountMask);
					testValue = _mm256_i32gather_epi32(haystackPtr, safeArrayIndex, sizeof(int));
				}
				++step;

				// if (testValue < value) {
				const __m256i gt = _mm256_cmpgt_epi32(testValue, value);
				const __m256i eq = _mm256_cmpeq_epi32(testValue, value);
				const __m256i ltMask = _mm256_andnot_si256(_mm256_castps_si256(_mm256_or_ps(_mm256_castsi256_ps(gt), _mm256_castsi256_ps(eq))), neg1);
				// mask by non finished lanes
				const __m256i lt_count_mask = _mm256_castps_si256(_mm256_and_ps(_mm256_castsi256_ps(ltMask), _mm256_castsi256_ps(positiveCountMask)));

				// true branch
				const __m256i lt_left = _mm256_add_epi32(leftHalf, ones);
				const __m256i lt_count = _mm256_sub_epi32(count, _mm256_add_epi32(half, ones));
				const __m256i lt_binIdx = _mm256_add_epi32(_mm256_slli_epi32(binIndex, 1), ones);

				// false branch
				const __m256i gt_eq_binIndex = _mm256_slli_epi32(binIndex, 1);

				// mix with result
				binIndex = masked_blend(lt_binIdx, gt_eq_binIndex, lt_count_mask);
				count = masked_blend(lt_count, half, lt_count_mask);
				left = masked_blend(lt_left, left, lt_count_mask);

				// update non finished lanes
				positiveCountMask = _mm256_cmpgt_epi32(count, zeros);
			}

			const __m256i haystackLeft = _mm256_i32gather_epi32(haystackPtr, left, sizeof(int));
			// if (hayStack[left] == value) {
			const __m256i eqMask = _mm256_cmpeq_epi32(value, haystackLeft);
			const __m256i storeResult = masked_blend(left, neg1, eqMask);

			// search values are not consecutive, scatter according to their indices
			// there is no scatter_epi32 in AVX/AVX2
			alignas(32) int writeBack[8];
			_mm256_store_si256(reinterpret_cast<__m256i *>(writeBack), storeResult);
			for (int r = 0; r < 8; r++) {
				indicesPtr[needleIndices[r]] = writeBack[r];
			}
		}
		// undo last increment so that non simd version can finish remaining elements
		c -= needlesCount % 8;
	}

	for (; c < needlesCount; c++) {
		const int value = needles[c];
		if (value < lowCut || value > highCut) {
			indices[c] = NOT_FOUND;
			continue;
		}

		int left = 0;
		int count = haystackCount;
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
			indices[c] = NOT_FOUND;
		}
	}

	allocator.freeAll();
}