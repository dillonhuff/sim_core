SRCFILES = $(wildcard ./src/[^_]*.cpp)
TESTFILES = $(wildcard ./test/[^_]*.cpp)

all-tests: $(SRCFILES)
	clang++ -std=c++11 $(SRCFILES) $(TESTFILES) -I/Users/dillon/CppWorkspace/coreir/include/ -I/opt/local/include -I./src -lcoreir -L/Users/dillon/CppWorkspace/coreir/lib
