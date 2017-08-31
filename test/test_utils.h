#pragma once

#include <string>

namespace sim_core {

  int compileCode(const std::string& code, const std::string& outFile);

  int compileCodeAndRun(const std::string& code,
			const std::string& outFile,
			const std::string& harnessFile);

}
