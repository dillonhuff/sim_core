#include "sim.h"

#include "coreir-passes/transform/flatten.h"
#include "coreir-passes/transform/rungenerators.h"

using namespace CoreIR;
using namespace CoreIR::Passes;

namespace sim_core {

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
      //Select* snd_select = static_cast<Select*>(snd);

      Type* fst_tp = fst_select->getType();
      //Type* snd_tp = snd_select->getType();

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

    }
  
  }

  void print_connections(Module* add4_n) {
    cout << "Instance ptrs" << endl;
    for (auto& inst : add4_n->getDef()->getInstances()) {
      cout << inst.first << " = " << inst.second << endl;

      Instance* ist = inst.second;

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
      if (isNumber(s.getSelStr())) {
	return cVar(*(s.getParent())) + "[" + s.getSelStr() + "]";
      } else {
	return cVar(*(s.getParent())) + "_" + s.getSelStr();
      }
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

  std::string getOpString(Instance& inst) {
    string genRefName = inst.getGeneratorRef()->getName();

    if (genRefName == "add") {
      return " + ";
    } else if (genRefName == "sub") {
      return " - ";
    } else if (genRefName == "mul") {
      return " * ";
    } else if (genRefName == "neg") {
      return "~";
    }

    assert(false);

  }

  string printUnop(Instance* inst, const vdisc vd, const NGraph& g) {
    auto outSelects = getOutputSelects(inst);

    assert(outSelects.size() == 1);

    string res = "";

    pair<string, Wireable*> outPair = *std::begin(outSelects);
    res += inst->getInstname() + "_" + outPair.first + " = ";

    //auto inSelects = getInputs(vd, g);
    auto inConns = getInputConnections(vd, g);

    assert(inConns.size() == 1);

    Conn cn = (*std::begin(inConns));
    Wireable* arg = cn.first;
    
    auto dest = inConns[0].second;
    assert(isSelect(dest));

    Select* destSel = toSelect(dest);
    assert(destSel->getParent() == inst);

    string opString = getOpString(*inst);

    res += opString + cVar(*arg) + ";\n";
    res += "\n";

    return res;
  }
  
  string printSub(Instance* inst, const vdisc vd, const NGraph& g) {
    auto outSelects = getOutputSelects(inst);

    assert(outSelects.size() == 1);

    string res = "";

    pair<string, Wireable*> outPair = *std::begin(outSelects);
    res += inst->getInstname() + "_" + outPair.first + " = ";

    //auto inSelects = getInputs(vd, g);
    auto inConns = getInputConnections(vd, g);

    assert(inConns.size() == 2);

    Wireable* arg1;
    Wireable* arg2;
    
    auto dest = inConns[0].second;
    assert(isSelect(dest));

    Select* destSel = toSelect(dest);
    assert(destSel->getParent() == inst);

    if (destSel->getSelStr() == "in0") {
      arg1 = inConns[0].first;
      arg2 = inConns[1].first;
    } else {
      arg1 = inConns[1].first;
      arg2 = inConns[0].first;
    }

    string opString = getOpString(*inst);

    res += cVar(*arg1) + opString + cVar(*arg2) + ";\n";
    res += "\n";

    return res;
  }

  string printBinop(Instance* inst, const vdisc vd, const NGraph& g) {
    assert(getInputs(vd, g).size() == 2);

    return printSub(inst, vd, g);

  }

  string printOp(Instance* inst, const vdisc vd, const NGraph& g) {
    auto ins = getInputs(vd, g);

    if (ins.size() == 2) {
      return printBinop(inst, vd, g);
    }

    if (ins.size() == 1) {
      return printUnop(inst, vd, g);
    }

    assert(false);
  }

  bool fromSelf(Select* w) {
    Wireable* parent = w->getParent();
    if (isSelect(parent)) {
      return fromSelf(toSelect(parent));
    }

    return parent->toString() == "self";
  }

  bool fromSelfInput(Select* w) {

    if (!fromSelf(w)) {
      return false;
    }

    if (w->getSelStr() == "in") {
      return true;
    }

    Wireable* parent = w->getParent();

    if (!isSelect(parent)) {
      return false;
    }

    return fromSelfInput(toSelect(parent));

  }

  bool fromSelfOutput(Select* w) {
    if (!fromSelf(w)) {
      return false;
    }

    if (w->getSelStr() == "out") {
      return true;
    }

    Wireable* parent = w->getParent();
    if (!isSelect(parent)) {
      return false;
    }

    return fromSelfOutput(toSelect(parent));

  }

  bool isArray(Type& t) {
    return t.getKind() == Type::TK_Array;
  }

  bool isBitArrayOfLength(Type& t, const uint len) {
    if (t.getKind() != Type::TK_Array) {
      return false;
    }

    ArrayType& tArr = static_cast<ArrayType&>(t);

    Type::TypeKind elemKind = (tArr.getElemType())->getKind();
    if ((elemKind == Type::TK_Bit || elemKind == Type::TK_BitIn) &&
	(tArr.getLen() == len)) {
      return true;
    }

    return false;
  }

  std::string cTypeString(Type& t) {
    if (isBitArrayOfLength(t, 8)) {
      return "uint8_t";
    }

    if (isBitArrayOfLength(t, 16)) {
      return "uint16_t";
    }
    
    if (isBitArrayOfLength(t, 32)) {
      return "uint32_t";
    }

    if (isBitArrayOfLength(t, 64)) {
      return "uint64_t";
    }
    
    if (isArray(t)) {
      ArrayType& tArr = static_cast<ArrayType&>(t);
      Type& underlying = *(tArr.getElemType());

      return cTypeString(underlying) + "*";
    }

    assert(false);

  }

  bool arrayAccess(Select* in) {
    return isNumber(in->getSelStr());
  }

  vector<Wireable*> collectInputVars(const std::deque<vdisc>& topo_order,
				     NGraph& g) {

    vector<Wireable*> self_inputs;
  
    for (auto& vd : topo_order) {
      Wireable* inst = get(boost::vertex_name, g, vd);

      auto ins = getInputSelects(inst);
      for (auto& inSel : ins) {
	auto in = inSel.second;
	if  (!arrayAccess(toSelect(in))) {
	  if (fromSelfInput(toSelect(in))) {
	    self_inputs.push_back(in);
	  }
	}
      }

      auto outs = getOutputSelects(inst);
      for (auto& outSel : outs) {
	auto out = outSel.second;
	if (!arrayAccess(toSelect(out))) {
	  if (fromSelfInput(toSelect(out))) {
	    self_inputs.push_back(out);
	  }
	}
      }

    }
  
    return self_inputs;
  }

  struct DeclaredWireables {
    vector<Wireable*> selfInputs;
    vector<Wireable*> selfOutputs;
    vector<Wireable*> internals;
  };

  
  DeclaredWireables getDeclaredWireables(const std::deque<vdisc>& topo_order,
					 NGraph& g) {
    vector<Wireable*> self_inputs;
    vector<Wireable*> self_outputs;
    vector<Wireable*> internals;

    for (auto& vd : topo_order) {
      Wireable* inst = get(boost::vertex_name, g, vd);

      auto ins = getInputSelects(inst);
      for (auto& inSel : ins) {
	auto in = inSel.second;
	if (!arrayAccess(toSelect(in))) {
	  if (fromSelfOutput(toSelect(in))) {
	    self_outputs.push_back(in);
	  } else if (fromSelfInput(toSelect(in))) {
	    self_inputs.push_back(in);
	  } else {
	    internals.push_back(in);
	  }
	}

      }

      auto outs = getOutputSelects(inst);
      for (auto& outSel : outs) {
	auto out = outSel.second;
	if (!arrayAccess(toSelect(out))) {
	  if (fromSelfOutput(toSelect(out))) {
	    self_outputs.push_back(out);
	  } else if (fromSelfInput(toSelect(out))) {
	    self_inputs.push_back(out);
	  } else {
	    internals.push_back(out);
	  }
	}
      }

    }

    return DeclaredWireables{self_inputs, self_outputs, internals};
  }


  string printSimFunctionBody(const std::deque<vdisc>& topo_order,
			      NGraph& g,
			      Module& mod) {
    string str = "";
    // Declare all variables
    str += "// Variable declarations\n";

    auto dw = getDeclaredWireables(topo_order, g);

    str += "// Outputs\n";

    Type* tp = mod.getType();

    cout << "module type = " << tp->toString() << endl;

    assert(tp->getKind() == Type::TK_Record);

    RecordType* modRec = static_cast<RecordType*>(tp);
    vector<string> declStrs;
    for (auto& name_type_pair : modRec->getRecord()) {
      Type* tp = name_type_pair.second;
      if (tp->isOutput()) {
	str += cTypeString(*tp) + " self_" + name_type_pair.first + ";\n";
      }
    }
    
    // for (auto& in : dw.selfOutputs) {
    //   str += cTypeString(*(in->getType())) + " " + cVar(*in) + ";\n";
    // }
  
    str += "// Internal variables\n";
    for (auto& in : dw.internals) {
      str += cTypeString(*(in->getType())) + " " + cVar(*in) + ";\n";
    }
    
  
    // Print out operations in topological order
    str += "// Simulation code\n";
    for (auto& vd : topo_order) {

      Wireable* inst = get(boost::vertex_name, g, vd);

      if (isInstance(inst)) {
	str += printOp(static_cast<Instance*>(inst), vd, g);
      } else {

	// If not an instance copy the input values
	auto inConns = getInputConnections(vd, g);
      
	for (auto inConn : inConns) {
	  str += cVar(*(inConn.second)) + " = " + cVar(*(inConn.first)) + ";\n";
	}

      }
	
    }

    // Copy outputs over to corresponding output pointers
    str += "// Copy results to output parameters\n";
    for (auto& out : dw.selfOutputs) {
      str += "*" + cVar(*out) + "_ptr = " + cVar(*out) + ";\n";
    }

    return str;
  }

  string printSimArguments(Module& mod,
			   const DeclaredWireables& dw) {
    Type* tp = mod.getType();

    cout << "module type = " << tp->toString() << endl;

    assert(tp->getKind() == Type::TK_Record);

    RecordType* modRec = static_cast<RecordType*>(tp);
    vector<string> declStrs;
    for (auto& name_type_pair : modRec->getRecord()) {
      Type* tp = name_type_pair.second;

      if (tp->isInput()) {
	declStrs.push_back(cTypeString(*tp) + " self_" + name_type_pair.first);
      } else {
	assert(tp->isOutput());
	declStrs.push_back(cTypeString(*tp) + "*" + " self_" + name_type_pair.first + "_ptr");
      }
    }

    string res;
    for (int i = 0; i < declStrs.size(); i++) {
      res += declStrs[i];
      if (i < declStrs.size() - 1) {
	res += ", ";
      }
    }

    // Print input list
    // for (auto& in : dw.selfInputs) {
    //   res += cTypeString(*(in->getType())) + " " + cVar(*in) + ", ";
    // }

    // assert(dw.selfOutputs.size() > 0);

    // for (int i = 0; i < dw.selfOutputs.size(); i++) {
    //   auto out = dw.selfOutputs[i];
    //   res += cTypeString(*(out->getType())) + "*" + " " + cVar(*out) + "_ptr";

    //   if (i < dw.selfOutputs.size() - 1) {
    // 	res += ", ";
    //   }
    // }

    return res;
  }

  string printCode(const std::deque<vdisc>& topoOrder,
		   NGraph& g,
		   CoreIR::Module* mod) {

    auto dw = getDeclaredWireables(topoOrder, g);

    string code = "";

    code += "#include <stdint.h>\n";
    code += "#include <stdio.h>\n";
    code += "#include <stdlib.h>\n";
    code += "void simulate( ";

    code += printSimArguments(*mod, dw);

    code += + " ) {\n";

    code += printSimFunctionBody(topoOrder, g, *mod);

    code += "}\n";

    return code;
  }

  void buildOrderedGraph(Module* mod, NGraph& g) {
    auto ord_conns = build_ordered_connections(mod);

    // Add vertexes for all instances in the graph
    unordered_map<Wireable*, vdisc> imap;

    for (auto& conn : ord_conns) {

      Select* sel1 = toSelect(conn.first);
      Select* sel2 = toSelect(conn.second);

      Wireable* w1 = sel1->getParent();
      Wireable* w2 = sel2->getParent();

      if (imap.find(w1) == end(imap)) {
	vdisc v1 = g.add_vertex(w1);
	imap.insert({w1, v1});
      }

      if (imap.find(w2) == end(imap)) {
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

  }

  std::deque<vdisc> topologicalSort(const NGraph& g) {
    deque<vdisc> topo_order;
    boost::topological_sort(g, std::front_inserter(topo_order));

    return topo_order;
  }

}
