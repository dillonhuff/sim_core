#include "coreir.h"

#include <boost/graph/directed_graph.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>

namespace sim_core {

  typedef std::pair<CoreIR::Wireable*, CoreIR::Wireable*> Conn;
  typedef boost::property<boost::edge_name_t, Conn > EdgeProp;
  typedef boost::directed_graph<boost::property<boost::vertex_name_t, CoreIR::Wireable*>, EdgeProp > NGraph;

  typedef boost::graph_traits<NGraph>::vertex_descriptor vdisc;
  typedef boost::graph_traits<NGraph>::edge_descriptor edisc;

  void buildOrderedGraph(CoreIR::Module* mod, NGraph& g);

  std::deque<vdisc> topologicalSort(const NGraph& g);

  void printCode(const std::deque<vdisc>& topoOrder,
		 NGraph& g);
  
}
