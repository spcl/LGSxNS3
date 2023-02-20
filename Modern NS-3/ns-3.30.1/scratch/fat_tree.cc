/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/logsim.h"
#include "ns3/logsim-helper.h"
#include "ns3/lgs-module.h"
#include "ns3/error-model.h"
#include "ns3/node.h"
#include "ns3/packet-sink.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/csma-module.h"
#include "ns3/flow-monitor-module.h"
#include <unordered_map>
#include <chrono>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpBulkSendExample");



int main(int argc, char *argv[])
{

  // Start Code Reserved for LGS and Fat Tree input
  CommandLine cmd;
  std::string filename_goal, protocol_name, packet_size, k;
  cmd.AddValue("goal_filename", "GOAL binary input file", filename_goal);
  cmd.AddValue("protocol", "TCP or UDP", protocol_name);
  cmd.AddValue("packet_size", "Payload Size of a packet", packet_size);
  cmd.AddValue("k", "Ports Per Switch of a Fattree", k);
  cmd.Parse(argc, argv);
  printf("Filename is %s - Protocol %s - Packet Size %s - K Fat Tree %s\n\n", filename_goal.c_str(), protocol_name.c_str(), packet_size.c_str(), k.c_str());
  int payload_size = std::stoi(packet_size);
  int k_ft = std::stoi(k);
  // End Code Reserved for LGS 

  // Protocol Parameters
  Config::SetDefault ("ns3::Ipv4GlobalRouting::RandomEcmpRouting",BooleanValue(false)); 
  Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpVeno"));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
  //Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (1));



  // Fat Tree Parameters
  int ports_per_switch = k_ft;
  int num_pods = ports_per_switch;
  int core_switches = pow(ports_per_switch / 2, 2);
  int aggr_switches = ports_per_switch / 2;
  int edge_switches = ports_per_switch / 2;
  int host_per_pod = pow(ports_per_switch / 2, 2);
  int total_supported_hosts = pow(ports_per_switch, 3) / 4;
  int total_nodes_to_create = total_supported_hosts + core_switches + aggr_switches * num_pods + edge_switches * num_pods;
  printf("Creating %d nodes, %d hosts, %d edge switches, %d aggregation switches, %d core switches\n\n", total_nodes_to_create, total_supported_hosts, edge_switches * num_pods, aggr_switches * num_pods, core_switches);

  // Creating NS3 Nodes
  NodeContainer fat_tree_nodes;
  fat_tree_nodes.Create(total_nodes_to_create);
  for (int i = total_supported_hosts; i < fat_tree_nodes.GetN(); i++) {
    fat_tree_nodes.Get(i)->isNic = false;
  }

  // Install Internet Stack and set base addr
  InternetStackHelper internet_stack;
  internet_stack.Install (fat_tree_nodes);
  Ipv4AddressHelper address_helper;
  char ipstring[14];
  int ip2 = 1; int ip1 = 1;
  sprintf(ipstring, "10.%d.%d.0", ip1, ip2);
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Gbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("100ns"));
  //pointToPoint.SetQueue ("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue (QueueSize ("1000p"))); // p in 100p stands for packets

  // Connect Hosts to edge switches (edge switches start right after hosts in the node list)
  int edge_host_idx = 0 + total_supported_hosts;
  for (int host_idx = 0; host_idx < total_supported_hosts; host_idx++) {
    NetDeviceContainer device = pointToPoint.Install(fat_tree_nodes.Get(host_idx), fat_tree_nodes.Get(edge_host_idx));
    address_helper.SetBase(ipstring, "255.255.255.0");
    Ipv4InterfaceContainer my_interface = address_helper.Assign(device);
    update_node_map(host_idx, my_interface.GetAddress(0));
    fat_tree_nodes.Get(host_idx)->isNic = true;
    printf("Host<->Edge Connecting node %d to node %d - IP are %d and %d | Base Addr %s\n", host_idx, edge_host_idx, my_interface.GetAddress(0).Get(), my_interface.GetAddress(1).Get(), ipstring); fflush(stdout);
    printf("Assigning IP %d to node %d\n", my_interface.GetAddress(0).Get(), host_idx);
    if (host_idx != 0 && (host_idx + 1) %  (ports_per_switch / 2) == 0) {
      edge_host_idx++;
    }
    ip2++;
    if (ip2 == 256) {
      ip1++;
      ip2 = 0;
    }
    sprintf(ipstring, "10.%d.%d.0", ip1, ip2);
  }

  // Connect edge switches to aggregation switches
  for (int pod_idx = 0; pod_idx < num_pods; pod_idx++) { // Tot pods
    for (int edge_idx = 0; edge_idx < edge_switches; edge_idx++) { // Edge switch per pod
      for (int aggr_idx = 0; aggr_idx < aggr_switches; aggr_idx++) { // First half of ports
        int id_edge_switch = total_supported_hosts + (pod_idx * edge_switches) + edge_idx;
        int id_aggr_switch = total_supported_hosts + (edge_switches * num_pods) + (aggr_switches * pod_idx) + aggr_idx;
        NetDeviceContainer device = pointToPoint.Install(fat_tree_nodes.Get(id_edge_switch), fat_tree_nodes.Get(id_aggr_switch));
        address_helper.SetBase(ipstring, "255.255.255.0");
        Ipv4InterfaceContainer my_interface = address_helper.Assign(device);
        printf("Edge<->Aggr Connecting node %d to node %d - IP are %d and %d | Base Addr %s\n", id_edge_switch, id_aggr_switch, my_interface.GetAddress(0).Get(), my_interface.GetAddress(1).Get(), ipstring); fflush(stdout);
        ip2++;
        if (ip2 == 256) {
          ip1++;
          ip2 = 0;
        }
        sprintf(ipstring, "10.%d.%d.0", ip1, ip2);
      }
    }
  }

  // Connect aggregation switches to core switches
  int offset_core = 0;
  for (int pod_idx = 0; pod_idx < num_pods; pod_idx++) { // Tot pods
    offset_core = 0;
    for (int aggr_idx = 0; aggr_idx < aggr_switches; aggr_idx++) { // Edge switch per pod
      for (int aggr_idx_2 = 0; aggr_idx_2 < aggr_switches; aggr_idx_2++) { // First half of ports
        int id_aggr_switch = total_supported_hosts + (edge_switches * num_pods) + (aggr_switches * pod_idx) + aggr_idx;
        int id_core_switch = total_supported_hosts + (edge_switches * num_pods) + (aggr_switches * num_pods) + aggr_idx_2 + offset_core;
        //printf("Two values %d and %d\n", id_aggr_switch, id_core_switch); fflush(stdout);
        NetDeviceContainer device = pointToPoint.Install(fat_tree_nodes.Get(id_aggr_switch), fat_tree_nodes.Get(id_core_switch));
        address_helper.SetBase(ipstring, "255.255.255.0");
        Ipv4InterfaceContainer my_interface = address_helper.Assign(device);
        printf("Aggr<->Core Connecting node %d to node %d - IP are %d and %d | Base Addr %s\n", id_aggr_switch, id_core_switch, my_interface.GetAddress(0).Get(), my_interface.GetAddress(1).Get(), ipstring); fflush(stdout);
        ip2++;
        if (ip2 == 256) {
          ip1++;
          ip2 = 0;
        }
        sprintf(ipstring, "10.%d.%d.0", ip1, ip2);
      }
      // Update Core Switch offset
      offset_core += aggr_switches;
    }
  }

  // Add routing and tracing
  printf("PopulateRoutingTables\n"); fflush(stdout);
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  PointToPointHelper p2p;
  p2p.EnablePcapAll("saved_file"); 
  // Trace routing tables 
  Ipv4GlobalRoutingHelper g;
  //Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("dynamic-global-routing.routes", std::ios::out);
  //g.PrintRoutingTableAllAt (Seconds (0), routingStream);

  // Start LGS
  printf("Starting LGS\n"); fflush(stdout);
  printf("\n");
  using std::chrono::high_resolution_clock;
  using std::chrono::duration_cast;
  using std::chrono::duration;
  using std::chrono::milliseconds;

  auto t1 = high_resolution_clock::now();
  PacketSinkHelper sink("ns3::TcpSocketFactory",
                        InetSocketAddress(Ipv4Address::GetAny(), 9));
  Ipv4InterfaceContainer dummy;
  initialize_interface_var(fat_tree_nodes, dummy, protocol_name, payload_size);
  start_lgs(filename_goal);

  auto t2 = high_resolution_clock::now();
  /* Getting number of milliseconds as an integer. */
  auto ms_int = duration_cast<milliseconds>(t2 - t1);
  /* Getting number of milliseconds as a double. */
  duration<double, std::milli> ms_double = t2 - t1;
  std::cout << "\n\nRunTime Total -> " << ms_int.count() << "ms\n";
  std::cout << ms_double.count() << "ms\n";

  Simulator::Destroy();
  return 0;
}
