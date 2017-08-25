#include "coreir.h"
#include "coreir-passes/transform/flatten.h"
#include "coreir-passes/transform/rungenerators.h"

using namespace CoreIR;
using namespace CoreIR::Passes;

bool isSelect(Wireable* fst) {
  return fst->getKind() == Wireable::WK_Select;
}

int main() {
  uint n = 32;
  
  // New context
  Context* c = newContext();
  
  Namespace* g = c->getGlobal();
  
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
  add4_n->print();

  RunGenerators rg;
  rg.runOnNamespace(g);

  cout << "After running generators" << endl;
  add4_n->print();

  cout << "Instance ptrs" << endl;
  for (auto& inst : add4_n->getDef()->getInstances()) {
    cout << inst.first << " = " << inst.second << endl;

    Instance* ist = inst.second;
    if (ist->hasConfigArgs()) {
      cout << "--- Config args" << endl;
      for (auto& arg : ist->getConfigArgs()) {
      }
    }

    for (auto& arg : ist->getGenArgs()) {
      cout << "Gen Arg" << endl;
      Arg* ag = arg.second;
      cout << ag->toString() << endl;
    }
  }
  cout << "My connections printout" << endl;
  for (auto& connection : add4_n->getDef()->getConnections()) {
    cout << "---- CONNECTION" << endl;
    Wireable* fst = connection.first;
    cout << "First wireable kind = " << fst->wireableKind2Str(fst->getKind()) << endl;
    cout << "con first = " << connection.first->toString() << endl;

    Wireable* snd = connection.second;
    cout << "con second = " << connection.second->toString() << endl;

    assert(isSelect(fst));
    assert(isSelect(snd));


    Select* fst_select = static_cast<Select*>(fst);
    Select* snd_select = static_cast<Select*>(snd);
    cout << "fst parent ptr = " << fst_select->getParent() << endl;
    cout << "snd parent ptr = " << snd_select->getParent() << endl;
  }

  deleteContext(c);
  
  return 0;
}
