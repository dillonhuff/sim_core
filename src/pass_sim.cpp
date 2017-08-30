
/*
 * Make sure you start at hellomodule.hpp before this one.
 *
 * This is just filling out some function definitions
 */

#include "coreir.h"
#include "coreir-passes/transform/flatten.h"
#include "coreir-passes/transform/rungenerators.h"

#include "pass_sim.hpp"
#include "sim.h"

//This is For a convenient macro to create the registerPass and deletePass functions
#include "coreir-macros.h"

using namespace CoreIR;
using namespace sim_core;

//Do not forget to set this static variable!!
string SimModule::ID = "simpass";
bool SimModule::runOnModule(Module* m) {

  cout << "RUNNING!!!" << endl;

  NGraph g;
  buildOrderedGraph(m, g);

  auto topOrder = topologicalSort(g);

  cout << "CODE" << endl;
  cout << printCode(topOrder, g, m) << endl;
  
  return false;
}

void SimModule::print() {
  cout << "SIMULATION CODE!!!" << endl;
}

//This is the macro that will define the registerPass and deletePass functions for you.
COREIR_GEN_EXTERNAL_PASS(SimModule);
