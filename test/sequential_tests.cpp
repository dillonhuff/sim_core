#include "catch.hpp"

#include "sim.h"
#include "utils.h"

#include "coreir-passes/transform/flatten.h"
#include "coreir-passes/transform/rungenerators.h"

#include <fstream>

using namespace CoreIR;
using namespace CoreIR::Passes;

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

  bool splitNodeEdgesCorrect(const NGraph& g) {

    cout << "Edges" << endl;

    auto edge_pair = boost::edges(g);
    for (auto it = edge_pair.first; it != edge_pair.second; it++) {
      Conn c = boost::get(boost::edge_name, g, *it);

      cout << (c.first).toString() << " ---> " << (c.second).toString() << endl;

      // Either the first edge is not a register or it is not a receiver
      Wireable* fstParent = toSelect(*(c.first.getWire())).getParent();
      bool notRec = !isRegisterInstance(fstParent) ||
	(c.first.isSequential && !(c.first.isReceiver));

      if (!notRec) { return false; }

      // Either the second edge is not a register or it is a reciver
      Wireable* sndParent = toSelect(*(c.second.getWire())).getParent();
      bool isRec = !isRegisterInstance(sndParent) ||
	(c.second.isSequential && c.second.isReceiver);

      if (!isRec) { return false; }
    }

    return true;

  }

  TEST_CASE("Sequential logic") {

    Context* c = newContext();

    SECTION("Counter") {

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

      SECTION("Checking number of vertices") {
      	// clk, en, out, ai, ci, ri_in, ri_out
      	REQUIRE(numVertices(g) == 7);
      }

      cout << "About to topological sort" << endl;
      deque<vdisc> topoOrder = topologicalSort(g);
      cout << "Done topological sorting" << endl;

      cout << "Vertices" << endl;
      for (auto& vd : topoOrder) {
	WireNode wd = boost::get(boost::vertex_name, g, vd);
	cout << wd.getWire()->toString() << endl;
      }

      REQUIRE(splitNodeEdgesCorrect(g));

      auto str = printCode(topoOrder, g, counter);
      cout << "CODE STRING" << endl;
      cout << str << endl;

      SECTION("Compile and run") {      
	string outFile = "./gencode/counter.c";
	std::ofstream out(outFile);
	out << str;
	out.close();


	string runCmd = "clang " + outFile + " gencode/test_counter.c";
	int s = system(runCmd.c_str());

	cout << "Command result = " << s << endl;

	REQUIRE(s == 0);


	string runTest = "./a.out";
	s = system(runTest.c_str());

	cout << "Test result = " << s << endl;

	REQUIRE(s == 0);
      }
      
    }

    SECTION("Register chain") {

      Type* regChainType = c->Record({
	  {"en",c->BitIn()},
	    {"ap", c->BitIn()->Arr(16)},
	    {"bp", c->BitIn()->Arr(16)},
	    {"out",c->Bit()->Arr(16)}, //Convenient Arr Type Constructor
	      {"clk",c->Named("coreir.clkIn")}, //Named Ref constructor 
		});

      Module* regChain = c->getGlobal()->newModuleDecl("regChain", regChainType);
      ModuleDef* def = regChain->newModuleDef();
      Args wArg({{"width",c->argInt(16)}});

      def->addInstance("ai", "coreir.add", wArg);
      def->addInstance("r0","coreir.reg",{{"width",c->argInt(16)},{"en",c->argBool(true)}});
      def->addInstance("r1","coreir.reg",{{"width",c->argInt(16)},{"en",c->argBool(true)}});
      def->addInstance("r2","coreir.reg",{{"width",c->argInt(16)},{"en",c->argBool(true)}});
    
      //Connections
      def->connect("self.clk", "r0.clk");
      def->connect("self.clk", "r1.clk");
      def->connect("self.clk", "r2.clk");
      
      def->connect("self.en", "r0.en");
      def->connect("self.en", "r1.en");
      def->connect("self.en", "r2.en");

      def->connect("self.ap", "r0.in");
      def->connect("r0.out","r1.in");
      def->connect("r1.out","r2.in");
      def->connect("r2.out","ai.in0");

      def->connect("self.bp", "ai.in1");
      //def->connect("r2.out", "ai.in1");

      def->connect("ai.out","self.out");

      regChain->setDef(def);
      regChain->print();
  
      RunGenerators rg;
      rg.runOnNamespace(c->getGlobal());

      NGraph g;
      buildOrderedGraph(regChain, g);

      SECTION("Checking number of vertices") {
	REQUIRE(splitNodeEdgesCorrect(g));	

      	// clk, en, ap, bp, out, ai, r0_in, r0_out, r1_in, r1_out, r2_in, r2_out
      	REQUIRE(numVertices(g) == 12);
      }



      cout << "About to topological sort" << endl;
      deque<vdisc> topoOrder = topologicalSort(g);
      cout << "Done topological sorting" << endl;

      cout << "Vertices" << endl;
      for (auto& vd : topoOrder) {
	WireNode wd = boost::get(boost::vertex_name, g, vd);
	cout << wd.getWire()->toString() << endl;
      }

      auto str = printCode(topoOrder, g, regChain);
      cout << "CODE STRING" << endl;
      cout << str << endl;

      SECTION("Compile and run") {      
	string outFile = "./gencode/register_chain.c";
	int s = compileCode(str, outFile);

	REQUIRE(s == 0);

      }
      
    }

    SECTION("Register withoug enable") {

      Type* regChainType = c->Record({
	  {"a", c->BitIn()->Arr(8)},
	    {"cout",c->Bit()->Arr(8)},
	      {"clk",c->Named("coreir.clkIn")},
		});

      Module* regChain = c->getGlobal()->newModuleDecl("regChain", regChainType);
      ModuleDef* def = regChain->newModuleDef();
      Args wArg({{"width",c->argInt(8)}});

      def->addInstance("r0","coreir.reg",{{"width",c->argInt(8)},{"en",c->argBool(false)}});
    
      //Connections
      def->connect("self.clk", "r0.clk");
      def->connect("self.a", "r0.in");
      def->connect("r0.out","self.cout");

      regChain->setDef(def);
  
      RunGenerators rg;
      rg.runOnNamespace(c->getGlobal());

      NGraph g;
      buildOrderedGraph(regChain, g);

      SECTION("Checking number of vertices") {
	REQUIRE(splitNodeEdgesCorrect(g));	

      	REQUIRE(numVertices(g) == 5);
      }



      cout << "About to topological sort" << endl;
      deque<vdisc> topoOrder = topologicalSort(g);
      cout << "Done topological sorting" << endl;

      cout << "Vertices" << endl;
      for (auto& vd : topoOrder) {
	WireNode wd = boost::get(boost::vertex_name, g, vd);
	cout << wd.getWire()->toString() << endl;
      }

      auto str = printCode(topoOrder, g, regChain);
      cout << "CODE STRING" << endl;
      cout << str << endl;
      
      SECTION("Compile and run") {
	string outFile = "./gencode/register_no_enable.c";
	int s = compileCode(str, outFile);

	REQUIRE(s == 0);
      }
    }

    SECTION("Register array") {
      uint n = 16;
      uint nRegs = 3;

      Type* regArrayType = c->Record({
	  {"clk", c->Named("coreir.clkIn")},
	    {"a", c->Array(3, c->Array(n, c->BitIn()))},
	      {"b", c->Array(3, c->Array(n, c->Bit()))}
	});

      Module* regArr = c->getGlobal()->newModuleDecl("regArr", regArrayType);
      
      
    }
    
    deleteContext(c);

  }

}
