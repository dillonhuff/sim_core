#define CATCH_CONFIG_MAIN

#include "catch.hpp"

#include "sim.h"

#include "coreir-passes/transform/flatten.h"
#include "coreir-passes/transform/rungenerators.h"

using namespace CoreIR;
using namespace CoreIR::Passes;

namespace sim_core {

  TEST_CASE("Combinational logic simulation") {

    // New context
    Context* c = newContext();
  
    Namespace* g = c->getGlobal();
    
    SECTION("32 bit add 4") {
      uint n = 32;
  
      Generator* add2 = c->getGenerator("coreir.add");

      // Define Add4 Module
      Type* add4Type = c->Record({
	  {"in",c->Array(4,c->Array(n,c->BitIn()))},
	    {"out",c->Array(n,c->Bit())}
	});

      Module* add4_n = g->newModuleDecl("Add4",add4Type);
      ModuleDef* def = add4_n->newModuleDef();
      Wireable* self = def->sel("self");
      Wireable* add_00 = def->addInstance("add00",add2,{{"width",c->argInt(n)}});
      Wireable* add_01 = def->addInstance("add01",add2,{{"width",c->argInt(n)}});
      Wireable* add_1 = def->addInstance("add1",add2,{{"width",c->argInt(n)}});
    
      def->connect(self->sel("in")->sel(0),add_00->sel("in0"));
      def->connect(self->sel("in")->sel(1),add_00->sel("in1"));
      def->connect(self->sel("in")->sel(2),add_01->sel("in0"));
      def->connect(self->sel("in")->sel(3),add_01->sel("in1"));

      def->connect(add_00->sel("out"),add_1->sel("in0"));
      def->connect(add_01->sel("out"),add_1->sel("in1"));

      def->connect(add_1->sel("out"),self->sel("out"));
      add4_n->setDef(def);

      RunGenerators rg;
      rg.runOnNamespace(g);

      NGraph g;
      buildOrderedGraph(add4_n, g);

      SECTION("Checking graph size") {
	REQUIRE(num_vertices(g) == 5);
      }
      
      deque<vdisc> topo_order = topologicalSort(g);

      auto str = printCode(topo_order, g, add4_n);

      string outFile = "./gencode/add4.c";
      std::ofstream out(outFile);
      out << str;
      out.close();

      string runCmd = "clang -c " + outFile;
      int s = system(runCmd.c_str());

      cout << "Command result = " << s << endl;

      REQUIRE(s == 0);

    }

    SECTION("64 bit subtract") {
      uint n = 64;
  
      Generator* sub2 = c->getGenerator("coreir.sub");

      Type* sub4Type = c->Record({
	  {"in",c->Array(4,c->Array(n,c->BitIn()))},
	    {"out",c->Array(n,c->Bit())}
	});

      Module* sub4_n = g->newModuleDecl("Sub4",sub4Type);
      ModuleDef* def = sub4_n->newModuleDef();
      Wireable* self = def->sel("self");
      Wireable* sub_00 = def->addInstance("sub00",sub2,{{"width",c->argInt(n)}});
      Wireable* sub_01 = def->addInstance("sub01",sub2,{{"width",c->argInt(n)}});
      Wireable* sub_1 = def->addInstance("sub1",sub2,{{"width",c->argInt(n)}});
    
      def->connect(self->sel("in")->sel(0),sub_00->sel("in0"));
      def->connect(self->sel("in")->sel(1),sub_00->sel("in1"));
      def->connect(self->sel("in")->sel(2),sub_01->sel("in0"));
      def->connect(self->sel("in")->sel(3),sub_01->sel("in1"));

      def->connect(sub_00->sel("out"),sub_1->sel("in0"));
      def->connect(sub_01->sel("out"),sub_1->sel("in1"));

      def->connect(sub_1->sel("out"),self->sel("out"));
      sub4_n->setDef(def);

      RunGenerators rg;
      rg.runOnNamespace(g);

      NGraph g;
      buildOrderedGraph(sub4_n, g);

      SECTION("Checking graph size") {
	REQUIRE(num_vertices(g) == 5);
      }
      
      deque<vdisc> topo_order = topologicalSort(g);

      auto str = printCode(topo_order, g, sub4_n);
      cout << "CODE STRING" << endl;
      cout << str << endl;

      string outFile = "./gencode/sub4.c";
      std::ofstream out(outFile);
      out << str;
      out.close();

      string runCmd = "clang -c " + outFile;
      int s = system(runCmd.c_str());

      cout << "Command result = " << s << endl;

      REQUIRE(s == 0);
    }

    SECTION("Multiply 8 bits") {

      uint n = 8;
  
      Generator* mul2 = c->getGenerator("coreir.mul");

      Type* mul2Type = c->Record({
	  {"in",c->Array(2,c->Array(n,c->BitIn()))},
	    {"out",c->Array(n,c->Bit())}
	});

      Module* mul_n = g->newModuleDecl("Mul4", mul2Type);
      ModuleDef* def = mul_n->newModuleDef();

      Wireable* self = def->sel("self");
      Wireable* mul = def->addInstance("mul1", mul2, {{"width", c->argInt(n)}});
    
      def->connect(self->sel("in")->sel(0), mul->sel("in0"));
      def->connect(self->sel("in")->sel(1), mul->sel("in1"));

      def->connect(mul->sel("out"), self->sel("out"));
      mul_n->setDef(def);

      RunGenerators rg;
      rg.runOnNamespace(g);

      NGraph g;
      buildOrderedGraph(mul_n, g);

      SECTION("Checking graph size") {
	REQUIRE(num_vertices(g) == 3);
      }
      
      deque<vdisc> topo_order = topologicalSort(g);

      auto str = printCode(topo_order, g, mul_n);
      cout << "CODE STRING" << endl;
      cout << str << endl;

      string outFile = "./gencode/mul2.c";
      std::ofstream out(outFile);
      out << str;
      out.close();

      string runCmd = "clang -c " + outFile;
      int s = system(runCmd.c_str());

      cout << "Command result = " << s << endl;

      REQUIRE(s == 0);

    }

    SECTION("One 37 bit logical and") {
      uint n = 37;

      Generator* andG = c->getGenerator("coreir.and");

      Type* andType = c->Record({
	  {"input", c->Array(2, c->Array(n, c->BitIn()))},
	    {"output", c->Array(n, c->Bit())}
	});

      Module* andM = g->newModuleDecl("and37", andType);

      ModuleDef* def = andM->newModuleDef();

      Wireable* self = def->sel("self");
      Wireable* and0 = def->addInstance("and0", andG, {{"width", c->argInt(n)}});

      def->connect("self.input.0", "and0.in0");
      def->connect("self.input.1", "and0.in1");
      def->connect("and0.out", "self.output");

      andM->setDef(def);

      RunGenerators rg;
      rg.runOnNamespace(g);

      NGraph g;
      buildOrderedGraph(andM, g);

      SECTION("Checking graph size") {
      	REQUIRE(num_vertices(g) == 3);
      }

      deque<vdisc> topo_order = topologicalSort(g);

      auto str = printCode(topo_order, g, andM);
      cout << "CODE STRING" << endl;
      cout << str << endl;

      string outFile = "./gencode/and37.c";
      std::ofstream out(outFile);
      out << str;
      out.close();

      string runCmd = "clang -c " + outFile;
      int s = system(runCmd.c_str());

      cout << "Command result = " << s << endl;

      REQUIRE(s == 0);
      
    }

    SECTION("One 16 bit negation") {
      uint n = 16;
  
      Generator* neg = c->getGenerator("coreir.neg");

      Type* neg2Type = c->Record({
	  {"A",    c->Array(n,c->BitIn())},
	    {"res", c->Array(n,c->Bit())}
	});

      Module* neg_n = g->newModuleDecl("neg_16", neg2Type);

      ModuleDef* def = neg_n->newModuleDef();

      Wireable* self = def->sel("self");
      Wireable* neg0 = def->addInstance("neg0", neg, {{"width", c->argInt(n)}});

      def->connect(self->sel("A"), neg0->sel("in"));

      def->connect(neg0->sel("out"), self->sel("res"));

      neg_n->setDef(def);

      RunGenerators rg;
      rg.runOnNamespace(g);

      Type* t = neg_n->getType();
      cout << "Module type = " << t->toString() << endl;
      
      NGraph g;
      buildOrderedGraph(neg_n, g);

      SECTION("Checking graph size") {
      	REQUIRE(num_vertices(g) == 3);
      }

      deque<vdisc> topo_order = topologicalSort(g);

      auto str = printCode(topo_order, g, neg_n);
      cout << "CODE STRING" << endl;
      cout << str << endl;

      string outFile = "./gencode/neg16.c";
      std::ofstream out(outFile);
      out << str;
      out.close();

      string runCmd = "clang -c " + outFile;
      int s = system(runCmd.c_str());

      cout << "Command result = " << s << endl;

      REQUIRE(s == 0);
      
    }
    
    SECTION("Two 16 bit negs") {
      uint n = 16;
  
      Generator* neg = c->getGenerator("coreir.neg");

      Type* neg2Type = c->Record({
	  {"in",    c->Array(2, c->Array(n,c->BitIn()))},
	    {"out", c->Array(2, c->Array(n,c->Bit()))}
	});

      Module* neg_n = g->newModuleDecl("two_negs", neg2Type);

      ModuleDef* def = neg_n->newModuleDef();

      Wireable* self = def->sel("self");
      Wireable* neg0 = def->addInstance("neg0", neg, {{"width", c->argInt(n)}});
      Wireable* neg1 = def->addInstance("neg1", neg, {{"width", c->argInt(n)}});

      def->connect(self->sel("in")->sel(0), neg0->sel("in"));
      def->connect(self->sel("in")->sel(1), neg1->sel("in"));

      def->connect(neg0->sel("out"), self->sel("out")->sel(0));
      def->connect(neg1->sel("out"), self->sel("out")->sel(1));

      neg_n->setDef(def);

      RunGenerators rg;
      rg.runOnNamespace(g);

      Type* t = neg_n->getType();
      cout << "Module type = " << t->toString() << endl;
      
      NGraph g;
      buildOrderedGraph(neg_n, g);

      SECTION("Checking graph size") {
	REQUIRE(num_vertices(g) == 4);
      }

      deque<vdisc> topo_order = topologicalSort(g);

      auto str = printCode(topo_order, g, neg_n);
      cout << "CODE STRING" << endl;
      cout << str << endl;

      string outFile = "./gencode/two_negs.c";
      std::ofstream out(outFile);
      out << str;
      out.close();

      string runCmd = "clang -c " + outFile;
      int s = system(runCmd.c_str());

      cout << "Command result = " << s << endl;

      REQUIRE(s == 0);
      
    }

    // Write up coreir pass for 
    SECTION("Add 2 by 3 64 bit matrices") {
      uint n = 64;
  
      Generator* add = c->getGenerator("coreir.add");

      Type* addMatsType = c->Record({
	  {"A",    c->Array(2, c->Array(3, c->Array(n,c->BitIn()))) },
	    {"B",    c->Array(2, c->Array(3, c->Array(n,c->BitIn()))) },
	      {"out", c->Array(2, c->Array(3, c->Array(n,c->Bit()))) }
	});

      Module* addM = g->newModuleDecl("two_negs", addMatsType);

      ModuleDef* def = addM->newModuleDef();

      Wireable* self = def->sel("self");
      for (int i = 0; i < 2; i++) {
	for (int j = 0; j < 3; j++) {
	  string str = "add" + to_string(i) + "_" + to_string(j);
	  Wireable* addInst = def->addInstance(str, add, {{"width", c->argInt(n)}});

	  def->connect(self->sel("A")->sel(i)->sel(j), addInst->sel("in0"));
	  def->connect(self->sel("B")->sel(i)->sel(j), addInst->sel("in1"));

	  def->connect(addInst->sel("out"), self->sel("out")->sel(i)->sel(j));
	}
      }

      addM->setDef(def);
      
	      
      RunGenerators rg;
      rg.runOnNamespace(g);

      Type* t = addM->getType();
      cout << "Module type = " << t->toString() << endl;
      
      NGraph g;
      buildOrderedGraph(addM, g);

      // SECTION("Checking graph size") {
      // 	REQUIRE(num_vertices(g) == 4);
      // }

      deque<vdisc> topo_order = topologicalSort(g);

      auto str = printCode(topo_order, g, addM);
      cout << "CODE STRING" << endl;
      cout << str << endl;

      string outFile = "./gencode/mat2_3_add.c";
      std::ofstream out(outFile);
      out << str;
      out.close();

      string runCmd = "clang " + outFile + " gencode/test_mat2_3_add.c";
      int s = system(runCmd.c_str());

      cout << "Command result = " << s << endl;

      REQUIRE(s == 0);


      string runTest = "./a.out";
      s = system(runTest.c_str());

      cout << "Test result = " << s << endl;

      REQUIRE(s == 0);
    }

    deleteContext(c);

  }

}
