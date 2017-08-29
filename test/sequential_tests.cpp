#include "catch.hpp"

#include "sim.h"

#include "coreir-passes/transform/flatten.h"
#include "coreir-passes/transform/rungenerators.h"

#include <ostream>

using namespace CoreIR;
using namespace CoreIR::Passes;

namespace sim_core {

  TEST_CASE("Sequential logic") {

    SECTION("Counter") {

      Context* c = newContext();

      Type* CounterType = c->Record({
	  {"en",c->BitIn()}, 
	    {"out",c->Bit()->Arr(16)}, //Convenient Arr Type Constructor
	      {"clk",c->Named("coreir.clkIn")}, //Named Ref constructor 
		});

      //Now lets create a module declaration. Declarations are specified separately from the definition
      Module* counter = c->getGlobal()->newModuleDecl("counter",CounterType); //use getGlobalFunction
      ModuleDef* def = counter->newModuleDef();
      Args wArg({{"width",c->argInt(16)}});
      def->addInstance("ai","coreir.add",wArg); // using <namespace>.<module> notation 
      def->addInstance("ci","coreir.const",wArg,{{"value",c->argInt(1)}});

      //Reg has default arguments. en/clr/rst are False by default. Init is also 0 by default
      def->addInstance("ri","coreir.reg",{{"width",c->argInt(16)},{"en",c->argBool(true)}});
    
      //Connections
      def->connect("self.clk","ri.clk");
      def->connect("self.en","ri.en");
      def->connect("ci.out","ai.in0");
      def->connect("ai.out","ri.in");
      def->connect("ri.out","ai.in1");
      def->connect("ri.out","self.out");

      counter->setDef(def);
      counter->print();
  
      RunGenerators rg;
      rg.runOnNamespace(c->getGlobal());

      NGraph g;
      buildOrderedGraph(counter, g);

      // SECTION("Checking number of vertices") {
      // 	// self, ai, ci, ri_in, ri_out
      // 	REQUIRE(numVertices(g) == 5);
      // }

      // cout << "About to topological sort" << endl;
      // deque<vdisc> topo_order = topologicalSort(g);
      // cout << "Done topological sorting" << endl;

      // auto str = printCode(topo_order, g, counter);
      // cout << "CODE STRING" << endl;
      // cout << str << endl;

      // string outFile = "./gencode/two_negs.c";
      // std::ofstream out(outFile);
      // out << str;
      // out.close();

      // string runCmd = "clang -c " + outFile;
      // int s = system(runCmd.c_str());

      // cout << "Command result = " << s << endl;

      // REQUIRE(s == 0);
      
      //Always remember to delete your context!
      deleteContext(c);
      
    }
  }

}
