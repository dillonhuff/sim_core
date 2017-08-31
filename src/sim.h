#pragma once

#include "coreir.h"

#include <boost/graph/directed_graph.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>

namespace sim_core {

  class WireNode {
  public:
    CoreIR::Wireable* wire;

    bool isSequential;
    bool isReceiver;

    CoreIR::Wireable* getWire() const { return wire; }

    bool operator==(const WireNode& other) const {
      return (wire == other.wire) &&
	(isSequential == other.isSequential) &&
	(isReceiver == other.isReceiver);
    }

    std::string toString() const {
      return getWire()->toString() + ", sequential ? " + std::to_string(isSequential) + ", isReceiver ? " + std::to_string(isReceiver);
    }

  };

  typedef WireNode WireableNode;

  typedef std::pair<WireableNode, WireableNode> Conn;
  typedef boost::property<boost::edge_name_t, Conn > EdgeProp;
  typedef boost::directed_graph<boost::property<boost::vertex_name_t, WireableNode>, EdgeProp > NGraph;

  typedef boost::graph_traits<NGraph>::vertex_descriptor vdisc;
  typedef boost::graph_traits<NGraph>::edge_descriptor edisc;

  void buildOrderedGraph(CoreIR::Module* mod, NGraph& g);

  std::deque<vdisc> topologicalSort(const NGraph& g);

  std::string printCode(const std::deque<vdisc>& topoOrder,
			NGraph& g,
			CoreIR::Module* mod);

  int numVertices(const NGraph& g);

}
