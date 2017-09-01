#pragma once

#include "wire_node.hpp"

#include <boost/graph/directed_graph.hpp>
#include <boost/graph/adjacency_list.hpp>

namespace sim_core {

  typedef WireNode WireableNode;

  typedef std::pair<WireableNode, WireableNode> Conn;
  typedef boost::property<boost::edge_name_t, Conn > EdgeProp;
  typedef boost::directed_graph<boost::property<boost::vertex_name_t, WireableNode>, EdgeProp > ONGraph;

  typedef boost::graph_traits<ONGraph>::vertex_descriptor vdisc;
  typedef boost::graph_traits<ONGraph>::edge_descriptor edisc;

  class NGraph {
  protected:
    std::vector<edisc> edges;
    std::vector<vdisc> verts;

    std::map<edisc, Conn> edgeNames;
    std::map<vdisc, WireNode> vertNames;

  public:
    ONGraph g;

    WireNode getNode(const vdisc vd) const {
      return boost::get(boost::vertex_name, g, vd);
    }

    Conn getConn(const edisc ed) const {
      return boost::get(boost::edge_name, g, ed);
    }
    
    edisc addEdge(const vdisc s, const vdisc e) {
      std::pair<edisc, bool> ed = g.add_edge(s, e);

      assert(ed.second);

      edges.push_back(ed.first);

      return ed.first;
    }

    vdisc addVertex(const WireNode& w) {
      vdisc v = g.add_vertex(w);
      verts.push_back(v);
      return v;
    }

    std::vector<Conn> getInputConnections(const vdisc vd) const;
    std::vector<Conn> getOutputConnections(const vdisc vd) const;
    std::vector<CoreIR::Wireable*> getInputs(const vdisc vd) const;
    std::vector<CoreIR::Wireable*> getOutputs(const vdisc vd) const;
  };

  int numVertices(const NGraph& g);
  std::deque<vdisc> topologicalSort(const NGraph& g);

  std::vector<Conn> getInputConnections(const vdisc vd, const NGraph& g);
  std::vector<CoreIR::Wireable*> getInputs(const vdisc vd, const NGraph& g);
  std::vector<CoreIR::Wireable*> getOutputs(const vdisc vd, const NGraph& g);

  Conn getConn(const NGraph& g, const edisc ed);
  WireNode getNode(const NGraph& g, const vdisc vd);

  CoreIR::Wireable* extractSource(CoreIR::Select* sel);

  std::vector<Conn> getOutputConnections(const vdisc vd, const NGraph& g);

  void addConnection(unordered_map<WireNode, vdisc>& imap,
		     Conn& conn,
		     NGraph& g);
  
  void addWireableToGraph(CoreIR::Wireable* w1,
			  unordered_map<WireNode, vdisc>& imap,
			  NGraph& g);

}
