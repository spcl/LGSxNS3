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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpBulkSendExample");



int main(int argc, char *argv[])
{

  // Start Code Reserved for LGS 
  CommandLine cmd;
  std::string filename_goal, protocol_name, packet_size, k;
  cmd.AddValue("goal_filename", "GOAL binary input file", filename_goal);
  cmd.AddValue("protocol", "TCP or UDP", protocol_name);
  cmd.AddValue("packet_size", "Payload Size of a packet", packet_size);
  cmd.AddValue("k", "Ports Per Switch of a Fattree", k);
  cmd.Parse(argc, argv);
  printf("Filename is %s - Protocol %s\n\n", filename_goal.c_str(), protocol_name.c_str());
  int payload_size = std::stoi(packet_size);
  int k_ft = std::stoi(k);
  // End Code Reserved for LGS 

  Time::SetResolution(Time::NS);

  NodeContainer nodes;
  nodes.Create(17);
  

  PointToPointHelper pointToPoint;

  Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
	Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
	rem->SetRandomVariable(uv);
	uv->SetStream(50);
	rem->SetAttribute("ErrorRate", DoubleValue(0.0));
	rem->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));
  pointToPoint.SetDeviceAttribute("ReceiveErrorModel", PointerValue(rem));
  
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("400Gbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("80ns"));


  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  NetDeviceContainer d1 = pointToPoint.Install(nodes.Get(0), nodes.Get(16));
  Ipv4InterfaceContainer i1 = address.Assign(d1);
  update_node_map(0, i1.GetAddress(0));
  nodes.Get(0)->isNic = true;
  //printf(" interfaces.GetAddress(i) is %d\n\n",  i1.GetAddress(0).Get());

  NetDeviceContainer d2 = pointToPoint.Install(nodes.Get(1), nodes.Get(16));
  address.SetBase("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i2 = address.Assign(d2);
  update_node_map(1, i2.GetAddress(0));
  nodes.Get(1)->isNic = true;
  //printf(" interfaces.GetAddress(i) is %d\n\n",  i2.GetAddress(0).Get());

  NetDeviceContainer d5 = pointToPoint.Install(nodes.Get(2), nodes.Get(16));
  address.SetBase("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i5 = address.Assign(d5);
  update_node_map(2, i5.GetAddress(0));
  nodes.Get(2)->isNic = true;
  //printf(" interfaces.GetAddress(i) is %d\n\n",  i5.GetAddress(0).Get());

  NetDeviceContainer d6 = pointToPoint.Install(nodes.Get(3), nodes.Get(16));
  address.SetBase("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer i6 = address.Assign(d6);
  update_node_map(3, i6.GetAddress(0));
  nodes.Get(3)->isNic = true;
  //printf(" interfaces.GetAddress(i) is %d\n\n",  i6.GetAddress(0).Get());

  NetDeviceContainer d7 = pointToPoint.Install(nodes.Get(4), nodes.Get(16));
  address.SetBase("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer i7 = address.Assign(d7);
  update_node_map(4, i7.GetAddress(0));
  nodes.Get(4)->isNic = true;
  //printf(" interfaces.GetAddress(i) is %d\n\n",  i7.GetAddress(0).Get());

  NetDeviceContainer d8 = pointToPoint.Install(nodes.Get(5), nodes.Get(16));
  address.SetBase("10.1.6.0", "255.255.255.0");
  Ipv4InterfaceContainer i8 = address.Assign(d8);
  update_node_map(5, i8.GetAddress(0));
  nodes.Get(5)->isNic = true;
  //printf(" interfaces.GetAddress(i) is %d\n\n",  i8.GetAddress(0).Get());

  NetDeviceContainer d9 = pointToPoint.Install(nodes.Get(6), nodes.Get(16));
  address.SetBase("10.1.7.0", "255.255.255.0");
  Ipv4InterfaceContainer i9 = address.Assign(d9);
  update_node_map(6, i9.GetAddress(0));
  nodes.Get(6)->isNic = true;
  //printf(" interfaces.GetAddress(i) is %d\n\n",  i9.GetAddress(0).Get());

  NetDeviceContainer d10 = pointToPoint.Install(nodes.Get(7), nodes.Get(16));
  address.SetBase("10.1.8.0", "255.255.255.0");
  Ipv4InterfaceContainer i10 = address.Assign(d10);
  update_node_map(7, i10.GetAddress(0));
  nodes.Get(7)->isNic = true;
  //printf(" interfaces.GetAddress(i) is %d\n\n",  i10.GetAddress(0).Get());

  NetDeviceContainer d11 = pointToPoint.Install(nodes.Get(8), nodes.Get(16));
    address.SetBase("10.1.9.0", "255.255.255.0");
  Ipv4InterfaceContainer i11 = address.Assign(d11);
  update_node_map(8, i11.GetAddress(0));
  nodes.Get(8)->isNic = true;
  //printf(" interfaces.GetAddress(i) is %d\n\n",  i11.GetAddress(0).Get());

  NetDeviceContainer d12 = pointToPoint.Install(nodes.Get(9), nodes.Get(16));
  address.SetBase("10.1.10.0", "255.255.255.0");
  Ipv4InterfaceContainer i12 = address.Assign(d12);
  update_node_map(9, i12.GetAddress(0));
  nodes.Get(9)->isNic = true;
  //printf(" interfaces.GetAddress(i) is %d\n\n",  i12.GetAddress(0).Get());

  NetDeviceContainer d13 = pointToPoint.Install(nodes.Get(10), nodes.Get(16));
  address.SetBase("10.1.11.0", "255.255.255.0");
  Ipv4InterfaceContainer i13 = address.Assign(d13);
  update_node_map(10, i13.GetAddress(0));
  nodes.Get(10)->isNic = true;
  //printf(" interfaces.GetAddress(i) is %d\n\n",  i13.GetAddress(0).Get());

  NetDeviceContainer d14 = pointToPoint.Install(nodes.Get(11), nodes.Get(16));
  address.SetBase("10.1.12.0", "255.255.255.0");
  Ipv4InterfaceContainer i14 = address.Assign(d14);
  update_node_map(11, i14.GetAddress(0));
  nodes.Get(11)->isNic = true;
  //printf(" interfaces.GetAddress(i) is %d\n\n",  i14.GetAddress(0).Get());

  NetDeviceContainer d15 = pointToPoint.Install(nodes.Get(12), nodes.Get(16));
  address.SetBase("10.1.13.0", "255.255.255.0");
  Ipv4InterfaceContainer i15 = address.Assign(d15);
  update_node_map(12, i15.GetAddress(0));
  nodes.Get(12)->isNic = true;
  //printf(" interfaces.GetAddress(i) is %d\n\n",  i15.GetAddress(0).Get());

  NetDeviceContainer d16 = pointToPoint.Install(nodes.Get(13), nodes.Get(16));
  address.SetBase("10.1.14.0", "255.255.255.0");
  Ipv4InterfaceContainer i16 = address.Assign(d16);
  update_node_map(13, i16.GetAddress(0));
  nodes.Get(13)->isNic = true;
  //printf(" interfaces.GetAddress(i) is %d\n\n",  i16.GetAddress(0).Get());

  NetDeviceContainer d17 = pointToPoint.Install(nodes.Get(14), nodes.Get(16));
  address.SetBase("10.1.15.0", "255.255.255.0");
  Ipv4InterfaceContainer i17 = address.Assign(d17);
  update_node_map(14, i17.GetAddress(0));
  nodes.Get(14)->isNic = true;
  //printf(" interfaces.GetAddress(i) is %d\n\n",  i17.GetAddress(0).Get());

  NetDeviceContainer d18 = pointToPoint.Install(nodes.Get(15), nodes.Get(16));
  address.SetBase("10.1.16.0", "255.255.255.0");
  Ipv4InterfaceContainer i18 = address.Assign(d18);
  update_node_map(15, i18.GetAddress(0));
  nodes.Get(15)->isNic = true;
  //printf(" interfaces.GetAddress(i) is %d\n\n",  i18.GetAddress(0).Get());
  


  // Later we add IP Addresses
  NS_LOG_INFO("Assign IP Addresses.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  PointToPointHelper p2p;
  p2p.EnablePcapAll("saved_file"); // filename without .pcap extention



  PacketSinkHelper sink("ns3::TcpSocketFactory",
                        InetSocketAddress(Ipv4Address::GetAny(), 9));

  
  initialize_interface_var(nodes, i1, protocol_name, payload_size);
  start_lgs(filename_goal);

  // std::cout << "Total Bytes Received Node(1): " << sink1->GetTotalRx () << std::endl;
  // std::cout << "Total Bytes Received Node(3): " << sink3->GetTotalRx () << std::endl;

  Simulator::Destroy();
  return 0;
}
