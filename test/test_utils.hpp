#pragma once

#include <string>

namespace CoreIR {

  int compileCode(const std::string& code, const std::string& outFile);

  int compileCodeAndRun(const std::string& code,
			const std::string& outFile,
			const std::string& harnessFile);

}
