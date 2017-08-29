cmake_minimum_required(VERSION 2.6)

project(bsim)

INCLUDE_DIRECTORIES(./src/)

SET(EXTRA_CXX_COMPILE_FLAGS "-std=c++11 -I./src -I./test -I/opt/local/include -O2 -Werror -Wall -I/Users/dillon/CppWorkspace/coreir/include/")

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${EXTRA_CXX_COMPILE_FLAGS}")


SET( COREIR_LINK_FLAGS "-L/Users/dillon/CppWorkspace/coreir/lib -lcoreir")
SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${COREIR_LINK_FLAGS}" )

INCLUDE_DIRECTORIES(./src/)

SET(SRC_FILES ./src/sim.cpp
	      ./src/utils.cpp)

SET(TEST_FILES ./test/combinational_tests.cpp
	       ./test/sequential_tests.cpp)

add_executable(all-tests ${TEST_FILES} ${SRC_FILES})
