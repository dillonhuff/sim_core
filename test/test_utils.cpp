#include "test_utils.h"

#include <iostream>
#include <fstream>

using namespace std;

namespace sim_core {

  int compileCode(const std::string& code, const std::string& outFile) {
    std::ofstream out(outFile);
    out << code;
    out.close();


    string runCmd = "clang -c " + outFile;
    int s = system(runCmd.c_str());

    cout << "Command result = " << s << endl;

    return s;

  }

}
