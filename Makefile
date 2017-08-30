# Compiler flags and paths
CXX = g++

CXXFLAGS = -std=c++11

COREIR_INCLUDE_PATH = /Users/dillon/CppWorkspace/coreir/include/
COREIR_LIB_PATH = /Users/dillon/CppWorkspace/coreir/lib

# File groups
SRC_FILES = $(wildcard src/[^_]*.cpp)
TEST_FILES = $(wildcard test/[^_]*.cpp)

SRC_HEADERS = $(wildcard src/[^_]*.h)
SRC_HEADERS += $(wildcard src/[^_]*.hpp)

SRC_OBJS = $(patsubst src/%.cpp, build/%.o,$(SRC_FILES))

all-tests: $(SRC_FILES) $(TEST_FILES)
	$(CXX) $(CXXFLAGS) $(SRC_FILES) $(TEST_FILES) -I$(COREIR_INCLUDE_PATH) -I/opt/local/include -I./src -lcoreir -L$(COREIR_LIB_PATH)

build/%.o: src/%.cpp $(SRC_HEADERS)
	$(CXX) $(CXXFLAGS) -I$(COREIR_INCLUDE_PATH) -I/opt/local/include -I./src -c -o $@ $<

simpass: build/simpass.dylib

build/simpass.dylib: $(SRC_OBJS)
	$(CXX) -L$(COREIR_LIB_PATH) -install_name "simpass.dylib" -dynamiclib $(LPATH) -lcoreir -o $@ $^

