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

SRC_OBJS = $(patsubst %.cpp, build/%.o, $(SRC_FILES))
TEST_OBJS = $(patsubst %.cpp, build/%.o, $(TEST_FILES))

all: all-tests simpass

all-tests: $(SRC_OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(SRC_OBJS) $(TEST_OBJS) -I$(COREIR_INCLUDE_PATH) -I/opt/local/include -I./src -lcoreir -L$(COREIR_LIB_PATH) -o $@

build/%.o: %.cpp $(SRC_HEADERS)
	$(CXX) $(CXXFLAGS) -I$(COREIR_INCLUDE_PATH) -I/opt/local/include -I./src -c -o $@ $<

simpass: build/simpass.dylib

build/simpass.dylib: $(SRC_OBJS)
	$(CXX) -L$(COREIR_LIB_PATH) -install_name "simpass.dylib" -dynamiclib $(LPATH) -lcoreir -o $@ $^

clean:
	rm -rf ./build
	mkdir ./build
	mkdir ./build/src
	mkdir ./build/test
