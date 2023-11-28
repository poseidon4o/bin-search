#pragma once

#include "baseline.hpp"
#include "simd-avx256.hpp"
#include "eytzinger.hpp"

#define TEST_SEARCH eytzingerSearchRangeCheck

#define STRINGIZE2(a) #a
#define STRINGIZE(a) STRINGIZE2(a)

#define TEST_NAME STRINGIZE(TEST_SEARCH)