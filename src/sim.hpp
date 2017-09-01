#pragma once

#include "wire_node.hpp"
#include "op_graph.hpp"

namespace sim_core {

  void buildOrderedGraph(CoreIR::Module* mod, NGraph& g);

  std::deque<vdisc> topologicalSort(const NGraph& g);

  std::string printCode(const std::deque<vdisc>& topoOrder,
			NGraph& g,
			CoreIR::Module* mod);

  int numVertices(const NGraph& g);

}
