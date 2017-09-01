#pragma once

#include "wire_node.hpp"

#include <boost/graph/directed_graph.hpp>
#include <boost/graph/adjacency_list.hpp>

namespace sim_core {

  typedef WireNode WireableNode;

  typedef std::pair<WireableNode, WireableNode> Conn;
  typedef boost::property<boost::edge_name_t, Conn > EdgeProp;
  typedef boost::directed_graph<boost::property<boost::vertex_name_t, WireableNode>, EdgeProp > NGraph;

  typedef boost::graph_traits<NGraph>::vertex_descriptor vdisc;
  typedef boost::graph_traits<NGraph>::edge_descriptor edisc;

}