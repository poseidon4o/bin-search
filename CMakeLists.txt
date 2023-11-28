cmake_minimum_required(VERSION 3.16)

project(bin-search)


set(CMAKE_CXX_STANDARD 20)

set(SOURCES
	src/main.cpp
)

set(HEADERS
	src/utils.hpp
	src/baseline.hpp
	src/simd-avx256.hpp
)


add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
add_executable(test-generator src/test-generator.cpp ${HEADERS})
add_executable(profiler src/profiler.cpp ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC src)

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

target_link_libraries(${PROJECT_NAME} PUBLIC ${DEBUG_LINKER_FLAGS})

target_compile_options(${PROJECT_NAME} PUBLIC $<$<CONFIG:Debug>:-O0 ${DEBUG_COMPILER_FLAGS}>)
target_compile_options(${PROJECT_NAME} PUBLIC $<$<CONFIG:RelWithDebInfo>:-Og ${DEBUG_COMPILER_FLAGS}>)
target_compile_options(${PROJECT_NAME} PUBLIC $<$<CONFIG:Release>:-Ofast>)