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
#include "ns3/logsim-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/node.h"
#include "ns3/packet-sink.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/csma-module.h"
#include <unordered_map>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpBulkSendExample");

std::unordered_map<std::string, u_int64_t> start_time_flow;
bool has_updated_recv = false;

void SinkRxTrace(std::string context, Ptr<const Packet> pkt, const Address &addr)
{
  // std::cout<<"Packet received at "<<Simulator::Now().GetSeconds()<<" s\n";

  CustomDataTag tag;
  if (pkt->FindFirstMatchingByteTag(tag))
  {
    // printf("@@@@@@@@@@@@TAG IS %d to %d, tag %d*******\n", tag.GetNodeId(), tag.GetReceivingNode(), tag.GetPersonalTag());
  }

  ns3::Ptr<ns3::Node> node = ns3::NodeList::GetNode(ns3::Simulator::GetContext());
  std::string to_hash = std::to_string(get_node_from_ip(Ipv4Address(tag.GetNodeId()))) + std::to_string(node->GetId());
  std::unordered_map<std::string, MsgInfo> active_sends_l = get_active_sends();
  update_active_map(to_hash, pkt->GetSize());

  if (start_time_flow.count(to_hash) == 0) {
    start_time_flow[to_hash] = Simulator::Now().GetNanoSeconds();
  }
  // printf("Hashing 2 is %s %s dd\n", to_hash.c_str(), pkt->ToString().c_str());
  /*Ptr<Packet> copy = pkt->Copy();

  // Headers must be removed in the order they're present.
  PppHeader pppHeader;
  copy->RemoveHeader(pppHeader);
  Ipv4Header ipHeader;
  copy->RemoveHeader(ipHeader);
  TcpHeader tcpHeader;
  copy->RemoveHeader(tcpHeader);

  std::cout << "Source IP: ";
  ipHeader.GetSource().Print(std::cout);
  std::cout << std::endl;
  std::cout << "Source Port: " << tcpHeader.GetSourcePort() << std::endl;
  std::cout << "Destination IP: ";
  ipHeader.GetDestination().Print(std::cout);
  std::cout << std::endl;
  std::cout << "Destination Port: " << tcpHeader.GetDestinationPort() << std::endl;
  // Headers must be removed in the order they're present.*/

  // printf("%d %d %d --  %d %d\n",ipHeader.GetSource().Get(), get_node_from_ip(ipHeader.GetSource().Get()), node->get, removed, removed2);
  //printf("Received Packet (S %d - D %d) of size %d at %f - Remaining %d and %d\n", tag.GetNodeId(), node->GetId(), pkt->GetSize(), Simulator::Now().GetSeconds(), get_active_sends()[to_hash].bytes_left_to_recv, get_active_sends()[to_hash].total_bytes_msg);

  // Here we imagine we have received a message fully, we need to give control back to LGS
  if (get_active_sends()[to_hash].bytes_left_to_recv <= 0)
  {
    Simulator::Stop();

    graph_node_properties latest_element;
    latest_element.updated = true;
    latest_element.type = OP_MSG;
    latest_element.target = get_node_from_ip(Ipv4Address(tag.GetNodeId()));
    latest_element.host = get_node_from_ip(Ipv4Address(tag.GetReceivingNode()));
    latest_element.starttime = Simulator::Now().GetNanoSeconds();
    latest_element.time = Simulator::Now().GetNanoSeconds();
    latest_element.size = get_active_sends()[to_hash].total_bytes_msg;
    latest_element.tag = 0;
    latest_element.proc = 0;
    latest_element.offset = get_active_sends()[to_hash].offset;
    

    printf("Flow Bandwidth is %f\n", (1117402 / (double)(Simulator::Now().GetNanoSeconds() - start_time_flow[to_hash])) * 8000);

    update_latest_receive(latest_element);
    printf("Msg received, sim stopping \n");
    return;
  }

  graph_node_properties latest_element;
  update_latest_receive(latest_element);
}

int main(int argc, char *argv[])
{
  // Start Code Reserved for LGS 
  CommandLine cmd;
  std::string filename_goal, protocol_name;
  cmd.AddValue("goal_filename", "GOAL binary input file", filename_goal);
  cmd.AddValue("protocol", "TCP or UDP", protocol_name);
  cmd.Parse(argc, argv);
  printf("Filename is %s - Protocol %s\n\n", filename_goal.c_str(), protocol_name.c_str());
  // End Code Reserved for LGS 

  Time::SetResolution(Time::NS);
  LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
  Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting",BooleanValue(true));
  /*NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);*/

  NodeContainer c;
  c.Create(4);
  NodeContainer l1, l2, l3, l4, l5, l6, l7, l8, l9, l10;
  l1.Create(1);
  l2.Create(1);
  l3.Create(1);
  l4.Create(1);
  l5.Create(1);
  l6.Create(1);
  l7.Create(1);
  l8.Create(1);
  l9.Create(1);
  l10.Create(1);


  NodeContainer n1 = NodeContainer(c.Get(0), l1);
  NodeContainer n2 = NodeContainer(c.Get(1), l2);
  NodeContainer n3 = NodeContainer(c.Get(2), l3);
  NodeContainer n4 = NodeContainer(c.Get(3), l4);

  NodeContainer n51 = NodeContainer(l5, l1);
  NodeContainer n52 = NodeContainer(l5, l2);

  NodeContainer n61 = NodeContainer(l6, l1);
  NodeContainer n62 = NodeContainer(l6, l2);

  NodeContainer n71 = NodeContainer(l7, l3);
  NodeContainer n72 = NodeContainer(l7, l4);

  NodeContainer n81 = NodeContainer(l8, l3);
  NodeContainer n82 = NodeContainer(l8, l4);

  NodeContainer n91 = NodeContainer(l9, l5);
  NodeContainer n92 = NodeContainer(l9, l7);

  NodeContainer nx = NodeContainer(l10, l6);
  NodeContainer ny = NodeContainer(l10, l8);

  InternetStackHelper internet;
  // internet.SetRoutingHelper (list);

  internet.Install(c.Get(0));
  internet.Install(c.Get(1));
  internet.Install(c.Get(2));
  internet.Install(c.Get(3));
  internet.Install(l1);
  internet.Install(l2);
  internet.Install(l3);
  internet.Install(l4);
  internet.Install(l5);
  internet.Install(l6);
  internet.Install(l7);
  internet.Install(l8);
  internet.Install(l9);
  internet.Install(l10);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("1000Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("50ns"));

  NetDeviceContainer d1 = p2p.Install(n1);
  NetDeviceContainer d2 = p2p.Install(n2);
  NetDeviceContainer d3 = p2p.Install(n3);
  NetDeviceContainer d4 = p2p.Install(n4);
  NetDeviceContainer d51 = p2p.Install(n51);
  NetDeviceContainer d52 = p2p.Install(n52);
  NetDeviceContainer d61 = p2p.Install(n61);
  NetDeviceContainer d62 = p2p.Install(n62);
  NetDeviceContainer d71 = p2p.Install(n71);
  NetDeviceContainer d72 = p2p.Install(n72);
  NetDeviceContainer d81 = p2p.Install(n81);
  NetDeviceContainer d82 = p2p.Install(n82);
  NetDeviceContainer d91 = p2p.Install(n91);
  NetDeviceContainer d92 = p2p.Install(n92);
  NetDeviceContainer dx = p2p.Install(nx);
  NetDeviceContainer dy = p2p.Install(ny);



  /*p2p.SetDeviceAttribute ("DataRate", StringValue ("15Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("5ms"));
  NetDeviceContainer d3d2 = p2p.Install (n3n2);*/

  // Later we add IP Addresses
  NS_LOG_INFO("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  
  
  /*ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign (d1);
  interfaces.Add(ipv4.Assign (d2));
  interfaces.Add(ipv4.Assign (d3));
  interfaces.Add(ipv4.Assign (d4));
  interfaces.Add(ipv4.Assign (d51));
  interfaces.Add(ipv4.Assign (d52));
  interfaces.Add(ipv4.Assign (d61));
  interfaces.Add(ipv4.Assign (d62));
  interfaces.Add(ipv4.Assign (d71));
  interfaces.Add(ipv4.Assign (d72));
  interfaces.Add(ipv4.Assign (d81));
  interfaces.Add(ipv4.Assign (d82));
  interfaces.Add(ipv4.Assign (d91));
  interfaces.Add(ipv4.Assign (d91));
  interfaces.Add(ipv4.Assign (dx));
  interfaces.Add(ipv4.Assign (dy));*/


  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i1 = ipv4.Assign(d1);

  
  
  ipv4.SetBase("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i2 = ipv4.Assign(d2);


  ipv4.SetBase("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i3 = ipv4.Assign(d3);

  ipv4.SetBase("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer i4 = ipv4.Assign(d4);

  ipv4.SetBase("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer i51 = ipv4.Assign(d51);

  ipv4.SetBase("10.1.6.0", "255.255.255.0");
  Ipv4InterfaceContainer i52 = ipv4.Assign(d52);
 
  ipv4.SetBase("10.1.7.0", "255.255.255.0");
  Ipv4InterfaceContainer i61 = ipv4.Assign(d61);

  ipv4.SetBase("10.1.8.0", "255.255.255.0");
  Ipv4InterfaceContainer i62 = ipv4.Assign(d62);

  ipv4.SetBase("10.1.9.0", "255.255.255.0");
  Ipv4InterfaceContainer i71 = ipv4.Assign(d71);

  ipv4.SetBase("10.1.10.0", "255.255.255.0");
  Ipv4InterfaceContainer i72 = ipv4.Assign(d72);

  ipv4.SetBase("10.1.11.0", "255.255.255.0");
  Ipv4InterfaceContainer i81 = ipv4.Assign(d81);

  ipv4.SetBase("10.1.12.0", "255.255.255.0");
  Ipv4InterfaceContainer i82 = ipv4.Assign(d82);

  ipv4.SetBase("10.1.13.0", "255.255.255.0");
  Ipv4InterfaceContainer i91 = ipv4.Assign(d91);

  ipv4.SetBase("10.1.14.0", "255.255.255.0");
  Ipv4InterfaceContainer i92 = ipv4.Assign(d92);

  ipv4.SetBase("10.1.15.0", "255.255.255.0");
  Ipv4InterfaceContainer ix = ipv4.Assign(dx);

  ipv4.SetBase("10.1.16.0", "255.255.255.0");
  Ipv4InterfaceContainer iy = ipv4.Assign(dy);
  
  // Create router nodes, initialize routing database and set up the routing
  // tables in the nodes.
  //
  printf("Reacher here\n"); fflush(stdout);
  Ipv4GlobalRoutingHelper:: PopulateRoutingTables();
  printf("Reacher here 22\n"); fflush(stdout);
  Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(2));
  //Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(1000));
  //Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue (5242880));
 // Config::SetDefault("ns3::TcpSocket::SlowStartThreshold", UintegerValue (10000000));
 //Config::SetDefault("ns3::ArpCache::PendingQueueSize", UintegerValue(10));   

  for (int i = 0; i < c.GetN(); i++) {
    c.Get(i)->isNic = true;
  }

  update_node_map(0, i1.GetAddress(0));
  printf(" interfaces.GetAddress(i) is %d\n\n", i1.GetAddress(0).Get());
  update_node_map(1, i2.GetAddress(0));
  printf(" interfaces.GetAddress(i) is %d\n\n", i2.GetAddress(0).Get());
  update_node_map(2, i3.GetAddress(0));
  printf(" interfaces.GetAddress(i) is %d\n\n", i3.GetAddress(0).Get());
  update_node_map(3, i4.GetAddress(0));
  printf(" interfaces.GetAddress(i) is %d\n\n", i4.GetAddress(0).Get());

  //PointToPointHelper p2p;
  p2p.EnablePcapAll("saved_file"); // filename without .pcap extention

  //
  // Create a PacketSinkApplication and install it on node 1
  //
  //PacketSinkHelper sink("ns3::TcpSocketFactory",
  //                      InetSocketAddress(Ipv4Address::GetAny(), 9));
  // ApplicationContainer TcpSinkApps = sink.Install (NodeList::GEt());
  /*for (int i = 0; i < NodeList::GetNNodes(); i++) {
    ApplicationContainer TcpSinkApps = sink.Install (NodeList::GetNode(i));
  }*/
  // Ptr<PacketSink> pktSink = StaticCast<PacketSink> (TcpSinkApps.Get(0));
  //sink.Install(c);

  /*std::stringstream ss; ss << "Some information";
  pktSink->TraceConnect("Rx", ss.str(), MakeCallback (&SinkRxTrace));*/

  //Config::Connect("NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&SinkRxTrace));

 
  // Create Fake Send to Synch at the beginning, run for ten seconds.
  Simulator::Stop(Seconds(10));
  Simulator::Run();

  initialize_interface_var(c, i2, protocol_name);
  start_lgs(filename_goal);

  // std::cout << "Total Bytes Received Node(1): " << sink1->GetTotalRx () << std::endl;
  // std::cout << "Total Bytes Received Node(3): " << sink3->GetTotalRx () << std::endl;
  printf("End called at %" PRIu64 "\n", Simulator::Now().GetNanoSeconds());
  //flowMonitor->SerializeToXmlFile("NameOfFile.xml", true, true);
  Simulator::Destroy();
  return 0;
}
