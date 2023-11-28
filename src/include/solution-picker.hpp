#pragma once

#include "solutions/baseline.hpp"
#include "solutions/simd-avx256.hpp"
#include "solutions/eytzinger.hpp"
#include "solutions/stl.hpp"

#define TEST_SEARCH eytzingerSearchRangeCheck<15>

#define STRINGIZE2(a) #a
#define STRINGIZE(a) STRINGIZE2(a)

#define TEST_NAME STRINGIZE(TEST_SEARCH)