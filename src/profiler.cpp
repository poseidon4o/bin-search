#include "utils.hpp"
#include "baseline.hpp"
#include "simd-avx256.hpp"


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
	profile(2);
	return 0;
}