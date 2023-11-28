#include <random>
#include <filesystem>
#include <cstring>

#include "utils.hpp"


enum DataType {
	uniform, allFound, allSame, allDifferent, normal, minMax, mostOut
};

void initData(AlignedArrayPtr<int> &haystack, AlignedArrayPtr<int> &needles, DataType type) {
	std::mt19937 rng(42);
	switch (type) {
	case uniform: {
		std::uniform_int<int> dataDist(0, haystack.getCount() << 1);
		std::uniform_int<int> queryDist(0, haystack.getCount() << 2);

		for (int c = 0; c < haystack.getCount(); c++) {
			haystack[c] = dataDist(rng);
		}

		for (int r = 0; r < needles.getCount(); r++) {
			needles[r] = queryDist(rng);
		}
		break;
	}
	case allDifferent:
		memset(haystack, 33, haystack.getCount() * sizeof(haystack[0]));
		memset(needles, 55, needles.getCount() * sizeof(needles[0]));
		break;
	case allSame:
		memset(haystack, 24, haystack.getCount() * sizeof(haystack[0]));
		memset(needles, 24, needles.getCount() * sizeof(needles[0]));
		break;
	case allFound: {
		std::uniform_int<int> dataDist(0, haystack.getCount() << 1);
		std::uniform_int<int> queryDist(0, haystack.getCount());

		for (int c = 0; c < haystack.getCount(); c++) {
			haystack[c] = dataDist(rng);
		}

		for (int c = 0; c < needles.getCount(); c++) {
			needles[c] = haystack[queryDist(rng)];
		}
		break;
	}
	case normal: {
		std::normal_distribution<double> dataDist(0, 1024);
		std::normal_distribution<double> queryDist(1024 * 3, 1024);

		for (int c = 0; c < haystack.getCount(); c++) {
			haystack[c] = dataDist(rng);
		}

		for (int r = 0; r < needles.getCount(); r++) {
			needles[r] = queryDist(rng);
		}
		break;
	}
	case minMax: {
		std::uniform_int<int> dataDist(0, haystack.getCount() << 1);
		for (int c = 0; c < haystack.getCount(); c++) {
			haystack[c] = dataDist(rng);
		}

		for (int c = 0; c < needles.getCount(); c++) {
			const int valueIndex = (c & 1) * (haystack.getCount() - 1);
			needles[c] = haystack[valueIndex];
		}
	}
	case mostOut: {
		std::uniform_int<int> dataDist(INT_MIN, INT_MAX);
		std::uniform_int<int> queryDist(0, 1 >> 16);

		for (int c = 0; c < haystack.getCount(); c++) {
			haystack[c] = dataDist(rng);
		}

		for (int r = 0; r < needles.getCount(); r++) {
			needles[r] = queryDist(rng);
		}
		break;
	}
	default:
		bassert(false);
		return;
	}
}


struct {
	int hCount;
	int qCount;
	DataType type;
} testInfos[] = {
	{1 << 26, 1 << 16, uniform},
	{1 << 26, 1 << 16, normal},
	{1 << 20, 1 << 18, minMax},
	{1 << 20, 1 << 18, mostOut},
	{1 << 26, 100, uniform},
	{1 << 16, 1 << 16, allSame},
	{1 << 16, 1 << 16, allDifferent},
	{ 1 << 18, 1 << 16, allSame},
	{ 1 << 18, 1 << 10, allSame},
};

bool generateInputFiles(bool forceRecreate = false) {
	const int variants = std::size(testInfos);

	for (int c = 0; c < variants; c++) {
		char fname[64] = { 0, };
		snprintf(fname, sizeof(fname), "%d.bsearch", c);
		const bool create = forceRecreate || !std::filesystem::exists(fname);

		AlignedArrayPtr<int> hayStack(testInfos[c].hCount);
		AlignedArrayPtr<int> needles(testInfos[c].qCount);
		if (create) {
			initData(hayStack, needles, testInfos[c].type);
			std::sort(hayStack.get(), hayStack + hayStack.getCount());

			printf("Saving to file %s ... ", fname);
			storeToFile(hayStack, needles, fname);
		}

		printf("Reading from file %s ... ", fname);
		AlignedArrayPtr<int> h, n;
		if (!loadFromFile(h, n, fname)) {
			printf("Failed to load from file %s", fname);
			return false;
		}

		const bool sameH = !memcmp(hayStack.get(), h.get(), hayStack.getCount() * sizeof(int));
		const bool sameN = !memcmp(needles.get(), n.get(), needles.getCount() * sizeof(int));
		printf("Verify haystack %d, verify needles %d \n", int(sameH), int(sameN));
	}
	return true;
}

int main() {
	generateInputFiles();
	return 0;
}
