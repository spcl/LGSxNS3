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
#include "ns3/node.h"
#include "ns3/packet-sink.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/csma-module.h"
#include <unordered_map>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpBulkSendExample");

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
  printf("Received Packet (S %d - D %d) of size %d at %f - Remaining %d and %d\n", node->GetId(), node->GetId(), pkt->GetSize(), Simulator::Now().GetSeconds(), get_active_sends()[to_hash].bytes_left_to_recv, get_active_sends()[to_hash].total_bytes_msg);

  // Here we imagine we have received a message fully, we need to give control back to LGS
  if (get_active_sends()[to_hash].bytes_left_to_recv == 0)
  {
    Simulator::Stop();

    graph_node_properties latest_element;
    latest_element.updated = true;
    latest_element.tag = tag.GetPersonalTag();
    latest_element.type = OP_MSG;
    latest_element.target = get_node_from_ip(Ipv4Address(tag.GetNodeId()));
    latest_element.host = get_node_from_ip(Ipv4Address(tag.GetReceivingNode()));
    latest_element.starttime = Simulator::Now().GetNanoSeconds();
    latest_element.time = Simulator::Now().GetNanoSeconds();
    latest_element.size = get_active_sends()[to_hash].total_bytes_msg;
    latest_element.offset = get_active_sends()[to_hash].offset;
    latest_element.proc = 0;
    ;

    update_latest_receive(latest_element);
    printf("Msg received, sim stopping -------------- %d %d \n", latest_element.host, latest_element.proc);
    return;
  }

  graph_node_properties latest_element;
  update_latest_receive(latest_element);
}

int main(int argc, char *argv[])
{
  CommandLine cmd;
  std::string filename_goal;
  cmd.AddValue("goal_filename", "GOAL binary input file", filename_goal);
  cmd.Parse(argc, argv);
  printf("Filename is %s\n\n", filename_goal.c_str());

  Time::SetResolution(Time::NS);
  LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);
  /*p2p.SetDeviceAttribute ("DataRate", StringValue ("15Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("5ms"));
  NetDeviceContainer d3d2 = p2p.Install (n3n2);*/

  // Later we add IP Addresses
  NS_LOG_INFO("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  // Create router nodes, initialize routing database and set up the routing
  // tables in the nodes.
  //
  // Ipv4GlobalRouting::RandomEcmpRouting:m_randomEcmpRouting (true);
  //  Ipv4GlobalRouting::RandomEcmpRouting (true);
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(40));
  Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
  // Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue (524288));
  // Config::SetDefault("ns3::TcpSocket::SlowStartThreshold", UintegerValue (1000000));

  for (int i = 0; i < NodeList::GetNNodes(); i++) {
    update_node_map(i,  interfaces.GetAddress(i));
    printf(" interfaces.GetAddress(i) is %d\n\n",  interfaces.GetAddress(i).Get());
  }
  PointToPointHelper p2p;
  p2p.EnablePcapAll("saved_file"); // filename without .pcap extention

  //
  // Create a PacketSinkApplication and install it on node 1
  //
  PacketSinkHelper sink("ns3::TcpSocketFactory",
                        InetSocketAddress(Ipv4Address::GetAny(), 9));
  // ApplicationContainer TcpSinkApps = sink.Install (NodeList::GEt());
  /*for (int i = 0; i < NodeList::GetNNodes(); i++) {
    ApplicationContainer TcpSinkApps = sink.Install (NodeList::GetNode(i));
  }*/
  // Ptr<PacketSink> pktSink = StaticCast<PacketSink> (TcpSinkApps.Get(0));
  sink.Install(nodes);

  /*std::stringstream ss; ss << "Some information";
  pktSink->TraceConnect("Rx", ss.str(), MakeCallback (&SinkRxTrace));*/

  Config::Connect("NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&SinkRxTrace));

  initialize_interface_var(nodes);
  start_lgs(filename_goal);

  // std::cout << "Total Bytes Received Node(1): " << sink1->GetTotalRx () << std::endl;
  // std::cout << "Total Bytes Received Node(3): " << sink3->GetTotalRx () << std::endl;
  //printf("End called at %" PRIu64 "\n", Simulator::Now().GetNanoSeconds());

  Simulator::Destroy();

  printf("End called at %d\n", 1);
  return 0;
}
