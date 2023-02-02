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
  std::string filename_goal, protocol_name;
  cmd.AddValue("goal_filename", "GOAL binary input file", filename_goal);
  cmd.AddValue("protocol", "TCP or UDP", protocol_name);
  cmd.Parse(argc, argv);
  printf("Filename is %s - Protocol %s\n\n", filename_goal.c_str(), protocol_name.c_str());
  // End Code Reserved for LGS 

  Time::SetResolution(Time::NS);

  NodeContainer nodes;
  nodes.Create (2);

  QbbHelper pointToPoint;

  Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
	Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
	rem->SetRandomVariable(uv);
	uv->SetStream(50);
	rem->SetAttribute("ErrorRate", DoubleValue(0.0));
	rem->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));
  pointToPoint.SetDeviceAttribute("ReceiveErrorModel", PointerValue(rem));
  
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1000Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("50ns"));



  bool enable_qcn = false, use_dynamic_pfc_threshold = true, packet_level_ecmp = false, flow_level_ecmp = false;
  uint32_t packet_payload_size = 512, l2_chunk_size = 0, l2_ack_interval = 0;
  double pause_time = 5;
  double cnp_interval = 50, alpha_resume_interval = 55, rp_timer, dctcp_gain = 1 / 16, np_sampling_interval = 0, pmax = 1;
  uint32_t byte_counter, fast_recovery_times = 5, kmax = 60, kmin = 60;

  bool clamp_target_rate = false, clamp_target_rate_after_timer = false, send_in_chunks = true, l2_wait_for_ack = false, l2_back_to_zero = false, l2_test_read = false;
  double error_rate_per_link = 0.0;
  Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(packet_level_ecmp));
	Config::SetDefault("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(flow_level_ecmp));
	Config::SetDefault("ns3::QbbNetDevice::PauseTime", UintegerValue(pause_time));
	Config::SetDefault("ns3::QbbNetDevice::QcnEnabled", BooleanValue(enable_qcn));
	//Config::SetDefault("ns3::QbbNetDevice::DynamicThreshold", BooleanValue(dynamicth));
	Config::SetDefault("ns3::QbbNetDevice::ClampTargetRate", BooleanValue(clamp_target_rate));
	Config::SetDefault("ns3::QbbNetDevice::ClampTargetRateAfterTimeInc", BooleanValue(clamp_target_rate_after_timer));
	Config::SetDefault("ns3::QbbNetDevice::CNPInterval", DoubleValue(cnp_interval));
	Config::SetDefault("ns3::QbbNetDevice::NPSamplingInterval", DoubleValue(np_sampling_interval));
	Config::SetDefault("ns3::QbbNetDevice::AlphaResumInterval", DoubleValue(alpha_resume_interval));
	Config::SetDefault("ns3::QbbNetDevice::RPTimer", DoubleValue(rp_timer));
	Config::SetDefault("ns3::QbbNetDevice::ByteCounter", UintegerValue(byte_counter));
	Config::SetDefault("ns3::QbbNetDevice::FastRecoveryTimes", UintegerValue(fast_recovery_times));
	Config::SetDefault("ns3::QbbNetDevice::DCTCPGain", DoubleValue(dctcp_gain));
	//Config::SetDefault("ns3::QbbNetDevice::RateAI", DataRateValue(DataRate(rate_ai)));
	//Config::SetDefault("ns3::QbbNetDevice::RateHAI", DataRateValue(DataRate(rate_hai)));
	Config::SetDefault("ns3::QbbNetDevice::L2BackToZero", BooleanValue(l2_back_to_zero));
	Config::SetDefault("ns3::QbbNetDevice::L2TestRead", BooleanValue(l2_test_read));
	Config::SetDefault("ns3::QbbNetDevice::L2ChunkSize", UintegerValue(l2_chunk_size));
	Config::SetDefault("ns3::QbbNetDevice::L2AckInterval", UintegerValue(l2_ack_interval));
	Config::SetDefault("ns3::QbbNetDevice::L2WaitForAck", BooleanValue(l2_wait_for_ack));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);
  printf("Got called here");
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

  //Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(40));
  //Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
  // Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue (524288));
  // Config::SetDefault("ns3::TcpSocket::SlowStartThreshold", UintegerValue (1000000));
  //Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(2));

  for (int i = 0; i < NodeList::GetNNodes(); i++) {
    update_node_map(i,  interfaces.GetAddress(i));
    printf(" interfaces.GetAddress(i) is %d\n\n",  interfaces.GetAddress(i).Get());
    nodes.Get(i)->isNic = true;
  }
  PointToPointHelper p2p;
  p2p.EnablePcapAll("saved_file"); // filename without .pcap extention

  //
  // Create a PacketSinkApplication and install it on node 1
  //
  //PacketSinkHelper sink("ns3::UdpSocketFactory",
  //                      InetSocketAddress(Ipv4Address::GetAny(), 9));
  // ApplicationContainer TcpSinkApps = sink.Install (NodeList::GEt());
  /*for (int i = 0; i < NodeList::GetNNodes(); i++) {
    ApplicationContainer TcpSinkApps = sink.Install (NodeList::GetNode(i));
  }*/
  // Ptr<PacketSink> pktSink = StaticCast<PacketSink> (TcpSinkApps.Get(0));
  //sink.Install(nodes);

  /*std::stringstream ss; ss << "Some information";
  pktSink->TraceConnect("Rx", ss.str(), MakeCallback (&SinkRxTrace));*/

  //Config::Connect("NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&SinkRxTrace));

  PacketSinkHelper sink("ns3::TcpSocketFactory",
                        InetSocketAddress(Ipv4Address::GetAny(), 9));
  // ApplicationContainer TcpSinkApps = sink.Install (NodeList::GEt());
  /*for (int i = 0; i < NodeList::GetNNodes(); i++) {
    ApplicationContainer TcpSinkApps = sink.Install (NodeList::GetNode(i));
  }*/
  // Ptr<PacketSink> pktSink = StaticCast<PacketSink> (TcpSinkApps.Get(0));
  //sink.Install(nodes);

  /*std::stringstream ss; ss << "Some information";
  pktSink->TraceConnect("Rx", ss.str(), MakeCallback (&SinkRxTrace));*/

  //Config::Connect("NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&SinkRxTrace));
  
  initialize_interface_var(nodes, interfaces, protocol_name);
  start_lgs(filename_goal);

  // std::cout << "Total Bytes Received Node(1): " << sink1->GetTotalRx () << std::endl;
  // std::cout << "Total Bytes Received Node(3): " << sink3->GetTotalRx () << std::endl;
  printf("End called at %" PRIu64 "\n", Simulator::Now().GetNanoSeconds());

  Simulator::Destroy();
  return 0;
}
