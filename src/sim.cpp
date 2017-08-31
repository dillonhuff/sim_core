#include "sim.h"

#include "coreir-passes/transform/flatten.h"
#include "coreir-passes/transform/rungenerators.h"

#include "utils.h"

using namespace CoreIR;
using namespace CoreIR::Passes;

  namespace std {

  template <>
  struct hash<sim_core::WireNode>
  {
    std::size_t operator()(const sim_core::WireNode& k) const
    {
      using std::size_t;
      using std::hash;
      using std::string;

      return ((hash<Wireable*>()(k.getWire())) ^
	      hash<bool>()(k.isSequential) ^
	      hash<bool>()(k.isReceiver));
    }
  };

}  

namespace sim_core {


  ArrayType& toArray(Type& tp) {
    assert(isArray(tp));

    return static_cast<ArrayType&>(tp);
  }

  string parens(const std::string& expr) {
    return "(" + expr + ")";
  }

  uint typeWidth(Type& tp) {
    assert(isPrimitiveType(tp));

    if ((tp.getKind() == Type::TK_BitIn) ||
	(tp.getKind() == Type::TK_Bit)) {
      return 1;
    }

    if (isBitArrayOfLengthLEQ(tp, 64)) {
      ArrayType& arrTp = toArray(tp);
      return arrTp.getLen();
    }

    cout << "ERROR: No type width for " << tp.toString() << endl;
    assert(false);
  }

  string bitMaskString(Type& tp) {
    uint w = typeWidth(tp);

    assert(w > 0);

    return parens(parens("1ULL << " + to_string(w)) + " - 1");
  }

  bool standardWidth(Type& tp) {
    uint w = typeWidth(tp);

    if ((w == 8) || (w == 16) || (w == 32) || (w == 64)) {
      return true;
    }

    return false;
  }

  string maskResult(Type& tp, const std::string& expr) {
    if (standardWidth(tp)) {
      return expr;
    }

    return parens( bitMaskString(tp) +  " & " + parens(expr));
  }
  
  Wireable* extractSource(Select* sel) {
    Wireable* p = sel->getParent();

    // Every select off of self gets its own node
    if (fromSelf(sel) && (!isSelect(p))) {
      return sel;
    }

    // Base case for non self connections
    if (!isSelect(p)) {
      return p;
    }

    return extractSource(toSelect(p));
  }
  
  std::string cVar(const WireNode& w) {
    string cv = cVar(*(w.getWire()));
    if (w.isSequential) {
      if (w.isReceiver) {
	return cv += "_receiver";
      } else {
	return cv += "_source";
      }

    }
    return cv;
  }

  std::string cVar(const WireNode& w, const std::string& suffix) {
    string cv = cVar(*(w.getWire()), suffix);
    if (w.isSequential) {
      if (w.isReceiver) {
	return cv += "_receiver";
      } else {
	return cv += "_source";
      }

    }
    return cv;
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

  bool connectionIsOrdered(const Connection& connection) {
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

  vector<Conn> buildOrderedConnections(Module* mod) {
    vector<Conn> conns;

    assert(mod->hasDef());

    cout << "Building connections" << endl;

    for (auto& connection : mod->getDef()->getConnections()) {

      cout << "Connection = " << (connection.first)->toString() << " " << (connection.second)->toString() << endl;

      assert(connectionIsOrdered(connection));


      Wireable* fst = connection.first;
      Wireable* snd = connection.second;

      assert(isSelect(fst));
      assert(isSelect(snd));

      Wireable* fst_p = toSelect(*fst).getParent();
      Wireable* snd_p = toSelect(*snd).getParent();

      Select* fst_select = static_cast<Select*>(fst);

      Type* fst_tp = fst_select->getType();

      WireNode w_fst{fst, false, false};
      WireNode w_snd{snd, false, false};

      if (fst_tp->isInput()) {

	if (isRegisterInstance(fst_p)) {
	  w_fst = {fst, true, true};
	}

	if (isRegisterInstance(snd_p)) {
	  w_snd = {snd, true, false};
	}

	
	conns.push_back({w_snd, w_fst});
      } else {

	if (isRegisterInstance(fst_p)) {
	  w_fst = {fst, true, false};
	}

	if (isRegisterInstance(snd_p)) {
	  w_snd = {snd, true, true};
	}

	conns.push_back({w_fst, w_snd});
      }

    }

    assert(conns.size() == mod->getDef()->getConnections().size());

    return conns;
  
  }

  std::vector<Conn> getOutputConnections(const vdisc vd, const NGraph& g) {
    vector<Conn> outConns;

    auto out_edge_pair = boost::out_edges(vd, g);
    Wireable* w = boost::get(boost::vertex_name, g, vd).getWire();

    for (auto it = out_edge_pair.first; it != out_edge_pair.second; it++) {
      auto out_edge_desc = *it;

      Conn edge_conn =
	boost::get(boost::edge_name, g, out_edge_desc);

      assert(isSelect(edge_conn.first.getWire()));
      Select* sel = static_cast<Select*>(edge_conn.first.getWire());
      assert(sel->getParent() == w);

      outConns.push_back(edge_conn);
      
    }
  
    return outConns;
  }

  int numVertices(const NGraph& g) {
    auto vertex_it_pair = boost::vertices(g);

    int numVerts = 0;
    for (auto it = vertex_it_pair.first; it != vertex_it_pair.second; it++) {
      numVerts++;
    }
    return numVerts;
  }

  std::vector<Conn> getInputConnections(const vdisc vd, const NGraph& g) {
    vector<Conn> inConss;

    auto out_edge_pair = boost::in_edges(vd, g);
    Wireable* w = boost::get(boost::vertex_name, g, vd).getWire();

    for (auto it = out_edge_pair.first; it != out_edge_pair.second; it++) {
      auto out_edge_desc = *it;

      Conn edge_conn =
	boost::get(boost::edge_name, g, out_edge_desc);

      assert(isSelect(edge_conn.second.getWire()));

      Select* sel = static_cast<Select*>(edge_conn.second.getWire());

      assert(extractSource(sel) == w);

      inConss.push_back(edge_conn);
    }
  
    return inConss;
  }

  std::vector<Wireable*> getOutputs(const vdisc vd, const NGraph& g) {
    vector<Wireable*> outputs;

    auto out_edge_pair = boost::out_edges(vd, g);
    Wireable* w = boost::get(boost::vertex_name, g, vd).getWire();

    for (auto it = out_edge_pair.first; it != out_edge_pair.second; it++) {
      auto out_edge_desc = *it;

      Conn edge_conn =
	boost::get(boost::edge_name, g, out_edge_desc);

      assert(isSelect(edge_conn.first.getWire()));
      Select* sel = static_cast<Select*>(edge_conn.first.getWire());
      assert(sel->getParent() == w);

      outputs.push_back(edge_conn.second.getWire());
      
    }

    return outputs;
  }

  std::vector<Wireable*> getInputs(const vdisc vd, const NGraph& g) {
    vector<Wireable*> inputs;
    Wireable* w = boost::get(boost::vertex_name, g, vd).getWire();
    auto in_edge_pair = boost::in_edges(vd, g);
    for (auto it = in_edge_pair.first; it != in_edge_pair.second; it++) {
      auto in_edge_desc = *it;
      //pair<Wireable*, Wireable*> edge_conn =
      Conn edge_conn =
	boost::get(boost::edge_name, g, in_edge_desc);

      assert(isSelect(edge_conn.second.getWire()));
      Select* sel = static_cast<Select*>(edge_conn.second.getWire());
      assert(sel->getParent() == w);

      inputs.push_back(edge_conn.first.getWire());
      
    }

    return inputs;
  }

  std::string getOpString(Instance& inst) {
    string genRefName = inst.getGeneratorRef()->getName();

    if (genRefName == "add") {
      return " + ";
    } else if (genRefName == "sub") {
      return " - ";
    } else if (genRefName == "mul") {
      return " * ";
    } else if (genRefName == "and") {
      return " & ";
    } else if (genRefName == "not") {
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

    auto inConns = getInputConnections(vd, g);

    assert(inConns.size() == 1);

    Conn cn = (*std::begin(inConns));
    Wireable* arg = cn.first.getWire();
    
    Wireable* dest = inConns[0].second.getWire();
    assert(isSelect(dest));

    Select* destSel = toSelect(dest);
    assert(destSel->getParent() == inst);

    string opString = getOpString(*inst);

    if (opString == "~") {
      res += maskResult(*((outPair.second)->getType()), opString + cVar(*arg));
    } else {
      res += opString + cVar(*arg);
    }

    res += ";\n\n";

    return res;
  }

  string printConstant(Instance* inst, const vdisc vd, const NGraph& g) {
    auto outSelects = getOutputSelects(inst);

    assert(outSelects.size() == 1);

    string res = "";

    bool foundValue = false;

    string argStr = "";
    for (auto& arg : inst->getConfigArgs()) {
      if (arg.first == "value") {
	foundValue = true;
	Arg* valArg = arg.second;

	assert(valArg->getKind() == AINT);

	ArgInt* valInt = static_cast<ArgInt*>(valArg);
	argStr = valInt->toString();
      }
    }

    assert(foundValue);

    pair<string, Wireable*> outPair = *std::begin(outSelects);
    res += inst->getInstname() + "_" + outPair.first + " = " + argStr + ";\n";

    return res;
  }

  string printSub(Instance* inst, const vdisc vd, const NGraph& g) {
    auto outSelects = getOutputSelects(inst);

    assert(outSelects.size() == 1);

    string res = "";

    pair<string, Wireable*> outPair = *std::begin(outSelects);
    res += inst->getInstname() + "_" + outPair.first + " = ";

    auto inConns = getInputConnections(vd, g);

    assert(inConns.size() == 2);

    WireNode arg1;
    WireNode arg2;
    
    auto dest = inConns[0].second.getWire();
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

    res += maskResult(*(outPair.second->getType()),
		      cVar(arg1) + opString + cVar(arg2)) + ";\n";
    res += "\n";

    return res;
  }

  string printBinop(Instance* inst, const vdisc vd, const NGraph& g) {
    assert(getInputs(vd, g).size() == 2);

    return printSub(inst, vd, g);

  }

  string varSuffix(const WireNode& wd) {
    if (wd.isSequential) {
      if (wd.isReceiver) {
	return "_receiver";
      } else {
	return "_source";
      }
    }

    return "";
  }

  bool hasEnable(Wireable* w) {
    assert(isRegisterInstance(w));

    return recordTypeHasField("en", w->getType());
  }

  string enableRegReceiver(const WireNode& wd, const vdisc vd, const NGraph& g) {

    auto outSel = getOutputSelects(wd.getWire());

    assert(outSel.size() == 1);
    Select* sl = toSelect((*(begin(outSel))).second);

    assert(isInstance(sl->getParent()));

    Instance* r = toInstance(sl->getParent());
    string rName = r->getInstname();

    auto ins = getInputConnections(vd, g);

    assert(ins.size() == 3);

    string s = "*" + rName + "_new_value = ";
    WireNode clk;
    WireNode en;
    WireNode add;

    for (auto& conn : ins) {
      WireNode arg = conn.first;
      WireNode placement = conn.second;
      string selName = toSelect(placement.getWire())->getSelStr();
      if (selName == "en") {
	en = arg;
      } else if (selName == "clk") {
	clk = arg;
      } else {
	add = arg;
      }
    }

    string oldValName = rName + "_old_value";

    s += "(((" + cVar(clk, "_last") + " == 0) && (" + cVar(clk) + " == 1)) && " +
      cVar(en) + ") ? " +
      cVar(add) + " : " + oldValName + ";\n";

    return s;
  }

  string noEnableRegReceiver(const WireNode& wd, const vdisc vd, const NGraph& g) {

    auto outSel = getOutputSelects(wd.getWire());

    assert(outSel.size() == 1);
    Select* sl = toSelect((*(begin(outSel))).second);

    assert(isInstance(sl->getParent()));

    Instance* r = toInstance(sl->getParent());
    string rName = r->getInstname();

    auto ins = getInputConnections(vd, g);

    assert(ins.size() == 2);

    string s = "*" + rName + "_new_value = ";
    WireNode clk;
    WireNode add;

    for (auto& conn : ins) {
      WireNode arg = conn.first;
      WireNode placement = conn.second;
      string selName = toSelect(placement.getWire())->getSelStr();
      if (selName == "clk") {
	clk = arg;
      } else {
	add = arg;
      }
    }

    string oldValName = rName + "_old_value";
    s += "((" + cVar(clk, "_last") + " == 0) && (" + cVar(clk) + " == 1)) ? " + cVar(add) + " : " + oldValName + ";\n";

    return s;
  }

  string printRegister(const WireNode& wd, const vdisc vd, const NGraph& g) {
    assert(wd.isSequential);

    auto outSel = getOutputSelects(wd.getWire());

    assert(outSel.size() == 1);
    Select* s = toSelect((*(begin(outSel))).second);

    assert(isInstance(s->getParent()));

    Instance* r = toInstance(s->getParent());
    string rName = r->getInstname();

    if (!wd.isReceiver) {
      return cVar(*s) + varSuffix(wd) + " = " + rName + "_old_value" + " ;\n";
    } else {
      if (hasEnable(wd.getWire())) {
	return enableRegReceiver(wd, vd, g);
      } else {
	return noEnableRegReceiver(wd, vd, g);
      }
    }
  }
  
  string printOp(const WireNode& wd, const vdisc vd, const NGraph& g) {
    Instance* inst = toInstance(wd.getWire());
    auto ins = getInputs(vd, g);
    
    if (isRegisterInstance(inst)) {
      return printRegister(wd, vd, g);
    }

    if (ins.size() == 2) {
      return printBinop(inst, vd, g);
    }

    if (ins.size() == 1) {
      return printUnop(inst, vd, g);
    }

    if (ins.size() == 0) {
      return printConstant(inst, vd, g);
    }

    cout << "Unsupported instance = " << inst->toString() << endl;
    assert(false);
  }

  bool fromSelfInterface(Select* w) {
    if (!fromSelf(w)) {
      return false;
    }

    Wireable* parent = w->getParent();
    if (isInterface(parent)) {
      return true;
    } else if (isInstance(parent)) {
      return false;
    }

    assert(isSelect(parent));

    return fromSelf(toSelect(parent));
  }

  std::string cArrayTypeDecl(Type& t, const std::string& varName) {
    if (isBitArrayOfLengthLEQ(t, 8)) {
      return "uint8_t " + varName;
    }

    if (isBitArrayOfLengthLEQ(t, 16)) {
      return "uint16_t " + varName;
    }
    
    if (isBitArrayOfLengthLEQ(t, 32)) {
      return "uint32_t " + varName;
    }

    if (isBitArrayOfLengthLEQ(t, 64)) {
      return "uint64_t " + varName;
    }
    
    if (isArray(t)) {
      ArrayType& tArr = static_cast<ArrayType&>(t);
      Type& underlying = *(tArr.getElemType());

      return cArrayTypeDecl(underlying, varName + "[ " + std::to_string(tArr.getLen()) + " ]");
    }

    if (!isArray(t)) {
      return cTypeString(t) + " " + varName;
    }

    
    assert(false);

  }
  
  bool arrayAccess(Select* in) {
    return isNumber(in->getSelStr());
  }

  std::unordered_map<string, Type*>
  outputs(Module& mod) {
    Type* tp = mod.getType();

    assert(tp->getKind() == Type::TK_Record);

    unordered_map<string, Type*> outs;

    RecordType* modRec = static_cast<RecordType*>(tp);
    vector<string> declStrs;
    for (auto& name_type_pair : modRec->getRecord()) {
      Type* tp = name_type_pair.second;

      if (tp->isOutput()) {
	outs.insert(name_type_pair);
      }
    }

    return outs;
    
  }

  string copyTypeFromInternal(Type* tp,
			      const std::string& toName,
			      const std::string& fromName) {
    if (isPrimitiveType(*tp)) {
      return toName + " = " + fromName + ";\n";
    }

    if (isArray(*tp)) {
      ArrayType* arr = static_cast<ArrayType*>(tp);

      string res = "";
      for (uint i = 0; i < arr->getLen(); i++) {
	string acc = "[ " + std::to_string(i) + " ]";
	string accName = toName + acc;
	string accFrom = fromName + acc;
	res += copyTypeFromInternal(arr->getElemType(), accName, accFrom) + "\n";
      }

      return res;
    }

    cout << "Failed for type = " << tp->toString() << endl;
    cout << "         toName = " << toName << endl;
    cout << "       fromName = " << fromName << endl;
    assert(false);
  }  

  string copyTypeFromInternal(Type* tp,
			      const std::string& field_name) {
    return copyTypeFromInternal(tp,
				"(*self_" + field_name + "_ptr)",
				"self_" + field_name);
  }

  string printInternalVariables(const std::deque<vdisc>& topo_order,
				NGraph& g,
				Module& mod) {
    string str = "";
    for (auto& vd : topo_order) {
      WireNode wd = boost::get(boost::vertex_name, g, vd);
      Wireable* w = wd.getWire();

      //auto inSelects = getInputSelects(w);
      for (auto inSel : w->getSelects()) { //inSelects) {
	Select* in = toSelect(inSel.second);

	if (!fromSelfInterface(in)) {
	  if (!arrayAccess(in)) {

	    if (!wd.isSequential) {
	      str += cTypeString(*(in->getType())) + " " + cVar(*in) + ";\n";
	    } else {
	      if (wd.isReceiver) {
		str += cTypeString(*(in->getType())) + " " + cVar(*in) + "_receiver;\n";
	      } else {
		str += cTypeString(*(in->getType())) + " " + cVar(*in) + "_source;\n";
	      }
	    }
	  }
	}
      }
    }

    return str;
  }

  string printSimFunctionBody(const std::deque<vdisc>& topo_order,
			      NGraph& g,
			      Module& mod) {
    string str = "";
    // Declare all variables
    str += "\n// Variable declarations\n";

    str += "\n// Outputs\n";

    for (auto& name_type_pair : outputs(mod)) {
      Type* tp = name_type_pair.second;
      str += cArrayTypeDecl(*tp, "self_" + name_type_pair.first) + ";\n";
    }
  
    str += "\n// Internal variables\n";
    str += printInternalVariables(topo_order, g, mod);

    // Print out operations in topological order
    str += "\n// Simulation code\n";
    for (auto& vd : topo_order) {

      WireNode wd = get(boost::vertex_name, g, vd);
      Wireable* inst = wd.getWire();

      if (isInstance(inst)) {
	str += printOp(wd, vd, g);
      } else {

	// If not an instance copy the input values
	auto inConns = getInputConnections(vd, g);
      
	for (auto inConn : inConns) {
	  str += cVar(*(inConn.second.getWire())) + " = " + cVar(inConn.first) + ";\n";
	}

      }
	
    }

    // Copy outputs over to corresponding output pointers
    str += "\n// Copy results to output parameters\n";
    for (auto& name_type_pair : outputs(mod)) {
      Type* tp = name_type_pair.second;
      str += copyTypeFromInternal(tp, name_type_pair.first);
    }

    return str;
  }

  bool underlyingTypeIsClkIn(Type& tp) {
    if (isClkIn(tp)) {
      return true;
    }

    if (isArray(tp)) {
      ArrayType& tarr = toArray(tp);
      return underlyingTypeIsClkIn(*(tarr.getElemType()));
    }

    return false;

  } 
  string printSimArguments(Module& mod) {

    Type* tp = mod.getType();

    cout << "module type = " << tp->toString() << endl;

    assert(tp->getKind() == Type::TK_Record);

    RecordType* modRec = static_cast<RecordType*>(tp);
    vector<string> declStrs;
    for (auto& name_type_pair : modRec->getRecord()) {
      Type* tp = name_type_pair.second;

      if (tp->isInput()) {
	if (!underlyingTypeIsClkIn(*tp)) { //(!isClkIn(*tp)) {
	  declStrs.push_back(cArrayTypeDecl(*tp, " self_" + name_type_pair.first));
	} else {
	  declStrs.push_back(cArrayTypeDecl(*tp, + " self_" + name_type_pair.first));
	  declStrs.push_back(cArrayTypeDecl(*tp, + " self_" + name_type_pair.first + "_last"));
	}
      } else {
	assert(tp->isOutput());
	declStrs.push_back(cArrayTypeDecl(*tp, "(*self_" + name_type_pair.first + "_ptr)"));
      }
    }

    // Add register inputs
    for (auto& inst : mod.getDef()->getInstances()) {
      if (isRegisterInstance(inst.second)) {
	Instance* is = inst.second;

	Select* in = is->sel("in");
	Type* itp = in->getType();

	string regName = is->getInstname();

	declStrs.push_back(cTypeString(*itp) + " " + regName + "_old_value");
	declStrs.push_back(cTypeString(*itp) + "* " + regName + "_new_value");
      }
    }
    
    // Print out declstrings
    string res = commaSepList(declStrs);

    return res;
  }

  string printCode(const std::deque<vdisc>& topoOrder,
		   NGraph& g,
		   CoreIR::Module* mod) {

    string code = "";

    code += "#include <stdint.h>\n";
    code += "#include <stdio.h>\n";
    code += "#include <stdlib.h>\n";
    code += "void simulate( ";

    code += printSimArguments(*mod);

    code += + " ) {\n";

    code += printSimFunctionBody(topoOrder, g, *mod);

    code += "}\n";

    return code;
  }

  void addWireableToGraph(Wireable* w1,
			  unordered_map<WireNode, vdisc>& imap,
			  NGraph& g) {

    if (isInstance(w1)) {
      Instance* inst = toInstance(w1);
      string genRefName = inst->getGeneratorRef()->getName();

      if (genRefName == "reg") {
	WireNode wOutput{w1, true, false};
	WireNode wInput{w1, true, true};

	if (imap.find(wOutput) == end(imap)) {
	  cout << "Adding register output" << endl;
	  auto v1 = g.add_vertex(wOutput);
	  imap.insert({wOutput, v1});
	}

	if (imap.find(wInput) == end(imap)) {
	  cout << "Adding register input" << endl;
	  auto v1 = g.add_vertex(wInput);
	  imap.insert({wInput, v1});
	}

	return;
      }
    }

    if (imap.find({w1, false, false}) == end(imap)) {
      WireNode w{w1, false, false};
      vdisc v1 = g.add_vertex(w);
      imap.insert({w, v1});
    }

  }


  void addConnection(unordered_map<WireNode, vdisc>& imap,
		     Conn& conn,
		     NGraph& g) {

    assert(isSelect(conn.first.getWire()));
    assert(isSelect(conn.second.getWire()));

    auto c1 = static_cast<Select*>(conn.first.getWire());
    auto c2 = static_cast<Select*>(conn.second.getWire());

    Wireable* p1 = extractSource(c1);

    vdisc c1_disc;
    if (isRegisterInstance(p1)) {
      auto c1_disc_it = imap.find({p1, true, false});

      assert(c1_disc_it != imap.end());

      c1_disc = (*c1_disc_it).second;

    } else {
      assert(!isRegisterInstance(p1));

      auto c1_disc_it = imap.find({p1, false, false});

      assert(c1_disc_it != imap.end());

      c1_disc = (*c1_disc_it).second;
    }
      
    Wireable* p2 = extractSource(c2);

    vdisc c2_disc;
    if (isRegisterInstance(p2)) {
      auto c2_disc_it = imap.find({p2, true, true});

      assert(c2_disc_it != imap.end());

      c2_disc = (*c2_disc_it).second;
    } else {
      assert(!isRegisterInstance(p2));

      auto c2_disc_it = imap.find({p2, false, false});

      assert(c2_disc_it != imap.end());

      c2_disc = (*c2_disc_it).second;
    }
      
    pair<edisc, bool> ed = g.add_edge(c1_disc, c2_disc);

    assert(ed.second);

    boost::put(boost::edge_name, g, ed.first, conn);
    
  }

  void buildOrderedGraph(Module* mod, NGraph& g) {

    cout << "Building ordered conns" << endl;

    auto ord_conns = buildOrderedConnections(mod);

    cout << "Built ordered connections" << endl;

    // Add vertexes for all instances in the graph
    unordered_map<WireNode, vdisc> imap;

    for (auto& conn : ord_conns) {

      Select* sel1 = toSelect(conn.first.getWire());
      Select* sel2 = toSelect(conn.second.getWire());

      Wireable* w1 = extractSource(sel1);
      Wireable* w2 = extractSource(sel2);

      addWireableToGraph(w1, imap, g);
      addWireableToGraph(w2, imap, g);

    }

    cout << "Adding edges" << endl;

    // Add edges to the graph
    for (Conn conn : ord_conns) {
      addConnection(imap, conn, g);
    }

    cout << "Done adding edges" << endl;

  }

  std::deque<vdisc> topologicalSort(const NGraph& g) {
    deque<vdisc> topo_order;
    boost::topological_sort(g, std::front_inserter(topo_order));

    return topo_order;
  }

}
