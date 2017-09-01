#include "op_graph.hpp"

#include "algorithm.hpp"
#include "utils.hpp"

using namespace CoreIR;

namespace sim_core {

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
  
  WireNode getNode(const NGraph& g, const vdisc vd) {
    return g.getNode(vd);//boost::get(boost::vertex_name, g.g, vd);
  }

  Conn getConn(const NGraph& g, const edisc ed) {
    return g.getConn(ed);//boost::get(boost::edge_name, g.g, ed);
  }

  std::vector<Conn> getInputConnections(const vdisc vd, const NGraph& g) {
    return g.getInputConnections(vd);
  }

  std::vector<Conn> NGraph::getInputConnections(const vdisc vd) const {
    vector<Conn> inConss;

    auto out_edge_pair = boost::in_edges(vd, g);
    Wireable* w = getNode(vd).getWire();

    for (auto it = out_edge_pair.first; it != out_edge_pair.second; it++) {
      auto out_edge_desc = *it;

      Conn edge_conn =
	getConn(out_edge_desc);

      assert(isSelect(edge_conn.second.getWire()));

      CoreIR::Select* sel = static_cast<Select*>(edge_conn.second.getWire());

      assert(extractSource(sel) == w);

      inConss.push_back(edge_conn);
    }
  
    return inConss;

  }

  std::vector<Wireable*> NGraph::getOutputs(const vdisc vd) const {
    vector<Wireable*> outputs;

    auto out_edge_pair = boost::out_edges(vd, g);
    Wireable* w = getNode(vd).getWire();

    for (auto it = out_edge_pair.first; it != out_edge_pair.second; it++) {
      auto out_edge_desc = *it;

      Conn edge_conn =
	getConn(out_edge_desc);

      assert(isSelect(edge_conn.first.getWire()));
      Select* sel = static_cast<Select*>(edge_conn.first.getWire());
      assert(sel->getParent() == w);

      outputs.push_back(edge_conn.second.getWire());
      
    }

    return outputs;
  }
  
  std::vector<Wireable*> getOutputs(const vdisc vd, const NGraph& g) {
    return g.getOutputs(vd);
  }

  std::vector<Wireable*> NGraph::getInputs(const vdisc vd) const {
    vector<Wireable*> inputs;
    Wireable* w = getNode(vd).getWire();
    auto in_edge_pair = boost::in_edges(vd, g);
    for (auto it = in_edge_pair.first; it != in_edge_pair.second; it++) {
      auto in_edge_desc = *it;

      Conn edge_conn =
	getConn(in_edge_desc);

      assert(isSelect(edge_conn.second.getWire()));
      Select* sel = static_cast<Select*>(edge_conn.second.getWire());
      assert(sel->getParent() == w);

      inputs.push_back(edge_conn.first.getWire());
      
    }

    return inputs;

  }

  std::vector<Wireable*> getInputs(const vdisc vd, const NGraph& g) {
    return g.getInputs(vd);
  }

  vector<vdisc> vertsWithNoIncomingEdge(const NGraph& g) {
    auto vertex_it_pair = boost::vertices(g.g);

    vector<vdisc> vs;

    for (auto it = vertex_it_pair.first; it != vertex_it_pair.second; it++) {
      vdisc v = *it;
      if (getInputConnections(v, g).size() == 0) {
	vs.push_back(v);
      }
    }

    return vs;
    
  }

  int numVertices(const NGraph& g) {
    auto vertex_it_pair = boost::vertices(g.g);

    int numVerts = 0;
    for (auto it = vertex_it_pair.first; it != vertex_it_pair.second; it++) {
      numVerts++;
    }
    return numVerts;
  }

  std::deque<vdisc> topologicalSort(const NGraph& g) {
    deque<vdisc> topo_order;

    vector<vdisc> s = vertsWithNoIncomingEdge(g);

    vector<edisc> deleted_edges;

    while (s.size() > 0) {
      vdisc vd = s.back();
      topo_order.push_back(vd);
      s.pop_back();

      
      auto edge_it_pair = boost::out_edges(vd, g.g);

      for (auto it = edge_it_pair.first; it != edge_it_pair.second; it++) {
	edisc ed = *it;

	deleted_edges.push_back(ed);
	
	vdisc src = source(ed, g.g);
	vdisc dest = target(ed, g.g);

	assert(src == vd);

	auto in_edge_pair = boost::in_edges(dest, g.g);

	bool noOtherEdges = true;
	for (auto ie = in_edge_pair.first; ie != in_edge_pair.second; ie++) {
	  edisc in_ed = *ie;
	  if (!elem(in_ed, deleted_edges)) {
	    noOtherEdges = false;
	    break;
	  }
	}

	if (noOtherEdges){
	  s.push_back(dest);
	}
      }


    }

    assert(topo_order.size() == numVertices(g));

    return topo_order;
  }

  std::vector<Conn> getOutputConnections(const vdisc vd, const NGraph& g) {
    return g.getOutputConnections(vd);
  }

  std::vector<Conn> NGraph::getOutputConnections(const vdisc vd) const {
    vector<Conn> outConns;

    auto out_edge_pair = boost::out_edges(vd, g);
    Wireable* w = getNode(vd).getWire();

    for (auto it = out_edge_pair.first; it != out_edge_pair.second; it++) {
      auto out_edge_desc = *it;

      Conn edge_conn =
	getConn(out_edge_desc);

      assert(isSelect(edge_conn.first.getWire()));
      Select* sel = static_cast<Select*>(edge_conn.first.getWire());
      assert(sel->getParent() == w);

      outConns.push_back(edge_conn);
      
    }
  
    return outConns;
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
      
    edisc ed = g.addEdge(c1_disc, c2_disc);

    boost::put(boost::edge_name, g.g, ed, conn);
    
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
	  auto v1 = g.addVertex(wOutput);
	  imap.insert({wOutput, v1});
	}

	if (imap.find(wInput) == end(imap)) {
	  cout << "Adding register input" << endl;
	  auto v1 = g.addVertex(wInput);
	  imap.insert({wInput, v1});
	}

	return;
      }
    }

    if (imap.find({w1, false, false}) == end(imap)) {
      WireNode w{w1, false, false};
      vdisc v1 = g.addVertex(w); //g.g.add_vertex(w);
      imap.insert({w, v1});
    }

  }

  
}
