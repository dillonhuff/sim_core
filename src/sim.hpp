#pragma once

#include "op_graph.hpp"

namespace sim_core {

  void buildOrderedGraph(CoreIR::Module* mod, NGraph& g);

  std::string printCode(const std::deque<vdisc>& topoOrder,
			NGraph& g,
			CoreIR::Module* mod);

}
