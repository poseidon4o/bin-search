cmake_minimum_required(VERSION 3.16)

project(bin-search)

set(CMAKE_CXX_STANDARD 20)

set(HEADERS
	src/include/utils.hpp
	src/include/solution-picker.hpp

	src/solutions/baseline.hpp
	src/solutions/simd-avx256.hpp
	src/solutions/eytzinger.hpp
)

set(DEBUG_COMPILER_FLAGS
	-fsanitize=address
	-fsanitize=leak
	-fsanitize=undefined
	-fno-omit-frame-pointer
)

set(DEBUG_LINKER_FLAGS
	-fsanitize=address
	-fsanitize=undefined
)

add_executable(speed-test src/speed-test.cpp ${HEADERS})
add_executable(test-generator src/test-generator.cpp ${HEADERS})
add_executable(profiler src/profiler.cpp ${HEADERS})

set(project_names
	speed-test
	test-generator
	profiler
)

foreach(name ${project_names})

	target_include_directories(${name} PUBLIC
		src
		src/include
		src/solutions
	)

	target_link_libraries(${name} PUBLIC ${DEBUG_LINKER_FLAGS})
	target_compile_options(${name} PUBLIC $<$<CONFIG:Debug>:-O0 ${DEBUG_COMPILER_FLAGS}>)
	target_compile_options(${name} PUBLIC $<$<CONFIG:RelWithDebInfo>:-Og ${DEBUG_COMPILER_FLAGS}>)
	target_compile_options(${name} PUBLIC $<$<CONFIG:Release>:-Ofast>)
endforeach()
