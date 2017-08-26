#include "coreir.h"
#include "coreir-passes/transform/flatten.h"
#include "coreir-passes/transform/rungenerators.h"

#include <boost/graph/directed_graph.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>


using namespace CoreIR;
using namespace CoreIR::Passes;

bool isSelect(Wireable* fst) {
  return fst->getKind() == Wireable::WK_Select;
}

bool isSelect(const Wireable& fst) {
  return fst.getKind() == Wireable::WK_Select;
}

Select& toSelect(Wireable& fst) {
  assert(isSelect(fst));
  return static_cast<Select&>(fst);
}

bool isInstance(Wireable* fst) {
  return fst->getKind() == Wireable::WK_Instance;
}

void print_wireable_selects(Wireable* fst_select) {
  cout << "Wireable selects" << endl;
  for (auto& s : fst_select->getSelects()) {
    Wireable* w = s.second;
    assert(isSelect(w));
    Select* sel = static_cast<Select*>(w);
    Type* tp = sel->getType();
    
    cout << s.first << " matches " << sel->wireableKind2Str(sel->getKind()) << " with type = " << tp->toString() << endl;
  }
  cout << "End selects" << endl;
}

bool connection_is_ordered(const Connection& connection) {
  Wireable* fst = connection.first;
  Wireable* snd = connection.second;

  assert(isSelect(fst));
  assert(isSelect(snd));

  // Is this the same as fst->getParent()->getType() ?? No, I dont think so
  Type* fst_tp = fst->getType();
  Type* snd_tp = snd->getType();

  if ((fst_tp->isInput() && snd_tp->isOutput()) ||
      (fst_tp->isOutput() && snd_tp->isInput())) {
    return true;
  }

  return false;
}

vector<pair<Wireable*, Wireable*> > build_ordered_connections(Module* mod) {
  vector<pair<Wireable*, Wireable*> > conns;

  for (auto& connection : mod->getDef()->getConnections()) {

    assert(connection_is_ordered(connection));


    Wireable* fst = connection.first;
    Wireable* snd = connection.second;

    assert(isSelect(fst));
    assert(isSelect(snd));

    Select* fst_select = static_cast<Select*>(fst);
    Select* snd_select = static_cast<Select*>(snd);

    Type* fst_tp = fst_select->getType();
    Type* snd_tp = snd_select->getType();

    if (fst_tp->isInput()) {
      conns.push_back({snd, fst});
    } else {
      conns.push_back({fst, snd});
    }

  }

  assert(conns.size() == mod->getDef()->getConnections().size());

  return conns;
  
}

void print_ordered_connections(Module* add4_n) {
  cout << "Ordered connections" << endl;
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

    Type* fst_tp = fst_select->getType();
    Type* snd_tp = snd_select->getType();

    cout << "fst type = " << fst_tp->toString() << endl;
    cout << "snd type = " << snd_tp->toString() << endl;

    assert(connection_is_ordered(connection));

    // cout << "fst parent ptr = " << fst_select->getParent() << endl;
    // print_wireable_selects(fst_select->getParent());
    // cout << "snd parent ptr = " << snd_select->getParent() << endl;
    // print_wireable_selects(snd_select->getParent());
  }
  
}

void print_connections(Module* add4_n) {
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
    print_wireable_selects(fst_select->getParent());
    cout << "snd parent ptr = " << snd_select->getParent() << endl;
    print_wireable_selects(snd_select->getParent());
  }

}

string selectInfoString(Wireable* w) {
  assert(isSelect(w));

  Select* s = static_cast<Select*>(w);
  string ss = s->getSelStr();
  //Wireable* parent = s->getParent();
  
  
  return ss + " " + s->getType()->toString();
}

typedef std::pair<Wireable*, Wireable*> Conn;
typedef boost::property<boost::edge_name_t, Conn > EdgeProp;
typedef boost::directed_graph<boost::property<boost::vertex_name_t, Wireable*>, EdgeProp > NGraph;

typedef boost::graph_traits<NGraph>::vertex_descriptor vdisc;
typedef boost::graph_traits<NGraph>::edge_descriptor edisc;

Select* toSelect(Wireable* w) {
  assert(isSelect(w));
  return static_cast<Select*>(w);
}

std::vector<Conn> getOutputConnections(const vdisc vd, const NGraph& g) {
  vector<Conn> outConns;

  auto out_edge_pair = boost::out_edges(vd, g);
  Wireable* w = boost::get(boost::vertex_name, g, vd);

  for (auto it = out_edge_pair.first; it != out_edge_pair.second; it++) {
    auto out_edge_desc = *it;
    pair<Wireable*, Wireable*> edge_conn =
      boost::get(boost::edge_name, g, out_edge_desc);

    assert(isSelect(edge_conn.first));
    Select* sel = static_cast<Select*>(edge_conn.first);
    assert(sel->getParent() == w);

    outConns.push_back(edge_conn);
      
  }
  
  return outConns;
}

std::vector<Conn> getInputConnections(const vdisc vd, const NGraph& g) {
  vector<Conn> inConss;

  auto out_edge_pair = boost::in_edges(vd, g);
  Wireable* w = boost::get(boost::vertex_name, g, vd);

  for (auto it = out_edge_pair.first; it != out_edge_pair.second; it++) {
    auto out_edge_desc = *it;
    pair<Wireable*, Wireable*> edge_conn =
      boost::get(boost::edge_name, g, out_edge_desc);

    assert(isSelect(edge_conn.second));

    Select* sel = static_cast<Select*>(edge_conn.second);

    assert(sel->getParent() == w);

    inConss.push_back(edge_conn);
      
  }
  
  return inConss;
}

std::vector<Wireable*> getOutputs(const vdisc vd, const NGraph& g) {
  vector<Wireable*> outputs;

  auto out_edge_pair = boost::out_edges(vd, g);
  Wireable* w = boost::get(boost::vertex_name, g, vd);

  for (auto it = out_edge_pair.first; it != out_edge_pair.second; it++) {
    auto out_edge_desc = *it;
    pair<Wireable*, Wireable*> edge_conn =
      boost::get(boost::edge_name, g, out_edge_desc);

    assert(isSelect(edge_conn.first));
    Select* sel = static_cast<Select*>(edge_conn.first);
    assert(sel->getParent() == w);

    outputs.push_back(edge_conn.second);
      
  }

  return outputs;
}

std::vector<Wireable*> getInputs(const vdisc vd, const NGraph& g) {
  vector<Wireable*> inputs;
  Wireable* w = boost::get(boost::vertex_name, g, vd);
  auto in_edge_pair = boost::in_edges(vd, g);
  for (auto it = in_edge_pair.first; it != in_edge_pair.second; it++) {
    auto in_edge_desc = *it;
    pair<Wireable*, Wireable*> edge_conn =
      boost::get(boost::edge_name, g, in_edge_desc);

    assert(isSelect(edge_conn.second));
    Select* sel = static_cast<Select*>(edge_conn.second);
    assert(sel->getParent() == w);

    inputs.push_back(edge_conn.first);
      
  }

  return inputs;
}

unordered_map<string, Wireable*>
getOutputSelects(Wireable* inst) {
  unordered_map<string, Wireable*> outs;

  for (auto& select : inst->getSelects()) {
    if (select.second->getType()->isOutput()) {
      outs.insert(select);
    }
  }

  return outs;
}

unordered_map<string, Wireable*>
getInputSelects(Wireable* inst) {
  unordered_map<string, Wireable*> outs;

  for (auto& select : inst->getSelects()) {
    if (select.second->getType()->isInput()) {
      outs.insert(select);
    }
  }

  return outs;
}

string cVar(Wireable& w) {
  if (isSelect(w)) {
    Select& s = toSelect(w);
    return cVar(*(s.getParent())) + "_" + s.getSelStr();
  } else {
    return w.toString();
  }
}

void printOutput(Wireable* inst, const vdisc vd, const NGraph& g) {
  auto outSelects = getOutputSelects(inst);

  //assert(outSelects.size() == 1);

  pair<string, Wireable*> outPair = *std::begin(outSelects);

  //cout << inst->getInstname() << "_" << outPair.first << " = ";

}

void printBinop(Instance* inst, const vdisc vd, const NGraph& g) {
  assert(getInputs(vd, g).size() == 2);

  auto outSelects = getOutputSelects(inst);

  assert(outSelects.size() == 1);

  pair<string, Wireable*> outPair = *std::begin(outSelects);
  cout << inst->getInstname() << "_" << outPair.first << " = ";

  auto inSelects = getInputs(vd, g);

  assert(inSelects.size() == 2);

  cout << cVar(*(inSelects[0])) << " + " << cVar(*(inSelects[1])) << ";" << endl;

  //pair<string, Wireable*> outPair = *std::begin(outSelects);
  
  cout << endl;
}

bool fromSelf(Select* w) {
  Wireable* parent = w->getParent();
  if (isSelect(parent)) {
    return fromSelf(toSelect(parent));
  }

  return parent->toString() == "self";
}

void printCode(const std::deque<vdisc>& topo_order,
	       NGraph& g) {

  // Declare all variables
  cout << "// Variable declarations" << endl;
  for (auto& vd : topo_order) {
    Wireable* inst = get(boost::vertex_name, g, vd);

    cout << endl << "// Input variables" << endl;
    auto ins = getInputSelects(inst);
    for (auto& inSel : ins) {
      auto in = inSel.second;
      cout << in->getType()->toString() << " " << cVar(*in) << ";";
      if (fromSelf(toSelect(in))) {
	cout << " // FROM SELF";
      }
      cout << endl;
    }

    cout << endl << "// Output variables" << endl;
    auto outs = getOutputSelects(inst);
    for (auto& outSel : outs) {
      auto out = outSel.second;
      cout << out->getType()->toString() << " " << cVar(*out) << ";";
      if (fromSelf(toSelect(out))) {
	cout << " // FROM SELF";
      }
      cout << endl;
      
    }

  }

  // Print out operations in topological order
  cout << "// Simulation code" << endl;
  for (auto& vd : topo_order) {

    Wireable* inst = get(boost::vertex_name, g, vd);

    if (isInstance(inst)) {
      printBinop(static_cast<Instance*>(inst), vd, g);
    } else {

      // If not an instance copy the input values
      auto inConns = getInputConnections(vd, g);
      
      for (auto inConn : inConns) {
	cout << cVar(*(inConn.second)) << " = " << cVar(*(inConn.first)) << ";" << endl;
      }

    }
	
  }

}

void buildOrderedGraph(Module* mod) {
  auto ord_conns = build_ordered_connections(mod);

  cout << "Ordered connections" << endl;
  for (auto& conn : ord_conns) {
    cout << selectInfoString(conn.first) << " --> " << selectInfoString(conn.second) << endl;
  }
  
  
  NGraph g;

  // Add vertexes for all instances in the graph
  unordered_map<Wireable*, vdisc> imap;

  for (auto& conn : ord_conns) {

    Select* sel1 = toSelect(conn.first);
    Select* sel2 = toSelect(conn.second);

    cout << "sel1 = " << sel1->toString() << endl;
    cout << "sel2 = " << sel2->toString() << endl;

    Wireable* w1 = sel1->getParent();
    Wireable* w2 = sel2->getParent();

    if (imap.find(w1) == end(imap)) {
      cout << "Adding vertex " << w1->toString() << endl;

      vdisc v1 = g.add_vertex(w1);
      imap.insert({w1, v1});
    }

    if (imap.find(w2) == end(imap)) {
      cout << "Adding vertex " << w2->toString() << endl;

      vdisc v2 = g.add_vertex(w2);
      imap.insert({w2, v2});
    }

  }

  // Add edges to the graph
  for (pair<Wireable*, Wireable*> conn : ord_conns) {
    assert(isSelect(conn.first));
    assert(isSelect(conn.second));

    auto c1 = static_cast<Select*>(conn.first);
    auto c2 = static_cast<Select*>(conn.second);

    Wireable* p1 = static_cast<Instance*>(c1->getParent());
    auto c1_disc_it = imap.find(p1);

    assert(c1_disc_it != imap.end());

    Wireable* p2 = static_cast<Instance*>(c2->getParent());
    auto c2_disc_it = imap.find(p2);

    assert(c2_disc_it != imap.end());

    vdisc c1_disc = (*c1_disc_it).second;
    vdisc c2_disc = (*c2_disc_it).second;
      
    pair<edisc, bool> ed = g.add_edge(c1_disc, c2_disc);

    assert(ed.second);

    boost::put(boost::edge_name, g, ed.first, conn);
  }

  cout << "Topological ordering" << endl;
  deque<vdisc> topo_order;
  boost::topological_sort(g, std::front_inserter(topo_order));

  printCode(topo_order, g);


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

  RunGenerators rg;
  rg.runOnNamespace(g);

  buildOrderedGraph(add4_n);

  deleteContext(c);
  
  return 0;
}
