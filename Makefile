CXX = g++

CXXFLAGS = -std=c++11

SRC_FILES = $(wildcard ./src/[^_]*.cpp)
TEST_FILES = $(wildcard ./test/[^_]*.cpp)

COREIR_INCLUDE_PATH = /Users/dillon/CppWorkspace/coreir/include/
COREIR_LIB_PATH = /Users/dillon/CppWorkspace/coreir/lib

all-tests: $(SRC_FILES) $(TEST_FILES)
	$(CXX) $(CXXFLAGS) $(SRC_FILES) $(TEST_FILES) -I$(COREIR_INCLUDE_PATH) -I/opt/local/include -I./src -lcoreir -L$(COREIR_LIB_PATH)

build/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
