#include "test_utils.hpp"

#include <iostream>
#include <fstream>

using namespace std;

namespace CoreIR {

  int compileCode(const std::string& code, const std::string& outFile) {
    std::ofstream out(outFile);
    out << code;
    out.close();


    string runCmd = "clang -c " + outFile;
    int s = system(runCmd.c_str());

    cout << "Command result = " << s << endl;

    return s;

  }

  int compileCodeAndRun(const std::string& code,
			const std::string& outFile,
			const std::string& harnessFile) {
    std::ofstream out(outFile);
    out << code;
    out.close();

    string runCmd = "clang " + outFile + " " + harnessFile;
    int s = system(runCmd.c_str());

    cout << "Command result = " << s << endl;

    string runTest = "./a.out";
    int r = system(runTest.c_str());

    cout << "Test result = " << r << endl;

    return s || r;
  }  
}
