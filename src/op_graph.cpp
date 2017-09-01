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
    return boost::get(boost::vertex_name, g, vd);
  }

  Conn getConn(const NGraph& g, const edisc ed) {
    return boost::get(boost::edge_name, g, ed);
  }

  std::vector<Conn> getInputConnections(const vdisc vd, const NGraph& g) {
    vector<Conn> inConss;

    auto out_edge_pair = boost::in_edges(vd, g);
    Wireable* w = getNode( g, vd).getWire();

    for (auto it = out_edge_pair.first; it != out_edge_pair.second; it++) {
      auto out_edge_desc = *it;

      Conn edge_conn =
	getConn( g, out_edge_desc);

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
    Wireable* w = getNode( g, vd).getWire();

    for (auto it = out_edge_pair.first; it != out_edge_pair.second; it++) {
      auto out_edge_desc = *it;

      Conn edge_conn =
	getConn( g, out_edge_desc);

      assert(isSelect(edge_conn.first.getWire()));
      Select* sel = static_cast<Select*>(edge_conn.first.getWire());
      assert(sel->getParent() == w);

      outputs.push_back(edge_conn.second.getWire());
      
    }

    return outputs;
  }

  std::vector<Wireable*> getInputs(const vdisc vd, const NGraph& g) {
    vector<Wireable*> inputs;
    Wireable* w = getNode( g, vd).getWire();
    auto in_edge_pair = boost::in_edges(vd, g);
    for (auto it = in_edge_pair.first; it != in_edge_pair.second; it++) {
      auto in_edge_desc = *it;
      //pair<Wireable*, Wireable*> edge_conn =
      Conn edge_conn =
	getConn( g, in_edge_desc);

      assert(isSelect(edge_conn.second.getWire()));
      Select* sel = static_cast<Select*>(edge_conn.second.getWire());
      assert(sel->getParent() == w);

      inputs.push_back(edge_conn.first.getWire());
      
    }

    return inputs;
  }

  vector<vdisc> vertsWithNoIncomingEdge(const NGraph& g) {
    auto vertex_it_pair = boost::vertices(g);

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
    auto vertex_it_pair = boost::vertices(g);

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

      
      auto edge_it_pair = boost::out_edges(vd, g);

      for (auto it = edge_it_pair.first; it != edge_it_pair.second; it++) {
	edisc ed = *it;

	deleted_edges.push_back(ed);
	
	vdisc src = source(ed, g);
	vdisc dest = target(ed, g);

	assert(src == vd);

	auto in_edge_pair = boost::in_edges(dest, g);

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
  
}
