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

      deque<vdisc> topo_order = topologicalSort(g);

      printCode(topo_order, g);
      
    }

    deleteContext(c);

  }

}
