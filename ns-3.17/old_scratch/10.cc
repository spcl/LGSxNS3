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
  nodes.Create(15);

  bool enable_qcn = false, use_dynamic_pfc_threshold = false, packet_level_ecmp = false, flow_level_ecmp = true;
  uint32_t packet_payload_size = 512, l2_chunk_size = 4000, l2_ack_interval = 2048;
  double pause_time = 5, simulator_stop_time = 3.01, app_start_time = 1.0, app_stop_time = 9.0;

  double cnp_interval = 50, alpha_resume_interval = 55, rp_timer, dctcp_gain = 1 / 16, np_sampling_interval = 0, pmax = 1;
  uint32_t byte_counter, fast_recovery_times = 5, kmax = 60, kmin = 60;
  std::string rate_ai, rate_hai;

  bool clamp_target_rate = false, clamp_target_rate_after_timer = false, send_in_chunks = false, l2_wait_for_ack = false, l2_back_to_zero = false, l2_test_read = false;
  double error_rate_per_link = 0.0;
  
  for (int i = 8; i < nodes.GetN(); i++) {
    nodes.Get(i)->SetNodeType(1, true); //broadcom switch
    nodes.Get(i)->m_broadcom->SetMarkingThreshold(kmin, kmax, pmax);
  }
  QbbHelper pointToPoint;

  Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
	Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
	rem->SetRandomVariable(uv);
	uv->SetStream(50);
	rem->SetAttribute("ErrorRate", DoubleValue(0.0));
	rem->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));
  pointToPoint.SetDeviceAttribute("ReceiveErrorModel", PointerValue(rem));
  
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1000Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("5ns"));

  SeedManager::SetSeed(time(NULL));

  
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

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  NetDeviceContainer d1 = pointToPoint.Install(nodes.Get(0), nodes.Get(8));
  Ipv4InterfaceContainer i1 = address.Assign(d1);
  update_node_map(0, i1.GetAddress(0));
  nodes.Get(0)->isNic = true;
  printf(" interfaces.GetAddress(i) is %d\n\n",  i1.GetAddress(0).Get());

  NetDeviceContainer d2 = pointToPoint.Install(nodes.Get(1), nodes.Get(8));
  address.SetBase("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i2 = address.Assign(d2);
  update_node_map(1, i2.GetAddress(0));
  nodes.Get(1)->isNic = true;
  printf(" interfaces.GetAddress(i) is %d\n\n",  i2.GetAddress(0).Get());

  NetDeviceContainer d3 = pointToPoint.Install(nodes.Get(2), nodes.Get(8));
  address.SetBase("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i3 = address.Assign(d3);
  update_node_map(2, i3.GetAddress(0));
  nodes.Get(2)->isNic = true;
  printf(" interfaces.GetAddress(i) is %d\n\n",  i3.GetAddress(0).Get());

  NetDeviceContainer d4 = pointToPoint.Install(nodes.Get(3), nodes.Get(8));
  address.SetBase("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer i4 = address.Assign(d4);
  update_node_map(3, i4.GetAddress(0));
  nodes.Get(3)->isNic = true;
  printf(" interfaces.GetAddress(i) is %d\n\n",  i4.GetAddress(0).Get());

  NetDeviceContainer d5 = pointToPoint.Install(nodes.Get(5), nodes.Get(9));
  address.SetBase("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer i5 = address.Assign(d5);
  update_node_map(5, i5.GetAddress(0));
  nodes.Get(4)->isNic = true;
  printf(" interfaces.GetAddress(i) is %d\n\n",  i5.GetAddress(0).Get());

  NetDeviceContainer d6 = pointToPoint.Install(nodes.Get(6), nodes.Get(9));
  address.SetBase("10.1.6.0", "255.255.255.0");
  Ipv4InterfaceContainer i6 = address.Assign(d6);
  update_node_map(6, i6.GetAddress(0));
  nodes.Get(5)->isNic = true;
  printf(" interfaces.GetAddress(i) is %d\n\n",  i6.GetAddress(0).Get());

  NetDeviceContainer d7 = pointToPoint.Install(nodes.Get(7), nodes.Get(9));
  address.SetBase("10.1.7.0", "255.255.255.0");
  Ipv4InterfaceContainer i7 = address.Assign(d7);
  update_node_map(7, i7.GetAddress(0));
  nodes.Get(6)->isNic = true;
  printf(" interfaces.GetAddress(i) is %d\n\n",  i7.GetAddress(0).Get());

  NetDeviceContainer d8 = pointToPoint.Install(nodes.Get(8), nodes.Get(9));
  address.SetBase("10.1.8.0", "255.255.255.0");
  Ipv4InterfaceContainer i8 = address.Assign(d8);
  update_node_map(8, i8.GetAddress(0));
  nodes.Get(7)->isNic = true;
  printf(" interfaces.GetAddress(i) is %d\n\n",  i8.GetAddress(0).Get());

  // From Router 8
  NetDeviceContainer d11 = pointToPoint.Install(nodes.Get(8), nodes.Get(10));
  address.SetBase("10.1.9.0", "255.255.255.0");
  Ipv4InterfaceContainer i11 = address.Assign(d11);
  printf(" interfaces.GetAddress(i) is %d\n\n",  i11.GetAddress(0).Get());

  NetDeviceContainer d12 = pointToPoint.Install(nodes.Get(8), nodes.Get(11));
  address.SetBase("10.1.10.0", "255.255.255.0");
  Ipv4InterfaceContainer i12 = address.Assign(d12);
  printf(" interfaces.GetAddress(i) is %d\n\n",  i12.GetAddress(0).Get());

  NetDeviceContainer d13 = pointToPoint.Install(nodes.Get(8), nodes.Get(12));
  address.SetBase("10.1.11.0", "255.255.255.0");
  Ipv4InterfaceContainer i13 = address.Assign(d13);
  printf(" interfaces.GetAddress(i) is %d\n\n",  i13.GetAddress(0).Get());

  NetDeviceContainer d14 = pointToPoint.Install(nodes.Get(8), nodes.Get(13));
  address.SetBase("10.1.12.0", "255.255.255.0");
  Ipv4InterfaceContainer i14 = address.Assign(d14);
  printf(" interfaces.GetAddress(i) is %d\n\n",  i14.GetAddress(0).Get());

  NetDeviceContainer d22 = pointToPoint.Install(nodes.Get(8), nodes.Get(14));
  address.SetBase("10.1.17.0", "255.255.255.0");
  Ipv4InterfaceContainer i22 = address.Assign(d22);
  printf(" interfaces.GetAddress(i) is %d\n\n",  i22.GetAddress(0).Get());

  // From Router 9
  NetDeviceContainer d15 = pointToPoint.Install(nodes.Get(9), nodes.Get(10));
  address.SetBase("10.1.13.0", "255.255.255.0");
  Ipv4InterfaceContainer i15 = address.Assign(d15);
  printf(" interfaces.GetAddress(i) is %d\n\n",  i15.GetAddress(0).Get());

  NetDeviceContainer d16 = pointToPoint.Install(nodes.Get(9), nodes.Get(11));
  address.SetBase("10.1.14.0", "255.255.255.0");
  Ipv4InterfaceContainer i16 = address.Assign(d16);
  printf(" interfaces.GetAddress(i) is %d\n\n",  i16.GetAddress(0).Get());

  NetDeviceContainer d17 = pointToPoint.Install(nodes.Get(9), nodes.Get(12));
  address.SetBase("10.1.15.0", "255.255.255.0");
  Ipv4InterfaceContainer i17 = address.Assign(d17);
  printf(" interfaces.GetAddress(i) is %d\n\n",  i17.GetAddress(0).Get());

  NetDeviceContainer d18 = pointToPoint.Install(nodes.Get(9), nodes.Get(13));
  address.SetBase("10.1.16.0", "255.255.255.0");
  Ipv4InterfaceContainer i18 = address.Assign(d18);
  printf(" interfaces.GetAddress(i) is %d\n\n",  i18.GetAddress(0).Get());

  NetDeviceContainer d19 = pointToPoint.Install(nodes.Get(9), nodes.Get(14));
  address.SetBase("10.1.18.0", "255.255.255.0");
  Ipv4InterfaceContainer i19 = address.Assign(d19);
  printf(" interfaces.GetAddress(i) is %d\n\n",  i19.GetAddress(0).Get());




  NodeContainer nodes2;
  nodes2.Create(2);
  NetDeviceContainer f1 = pointToPoint.Install(nodes2.Get(0), nodes.Get(8));
  address.SetBase("10.1.19.0", "255.255.255.0");
  Ipv4InterfaceContainer g1 = address.Assign(f1);
  update_node_map(4, g1.GetAddress(0));
  nodes2.Get(0)->isNic = true;
  printf(" interfaces.GetAddress(i) is %d\n\n",  g1.GetAddress(0).Get());

  NetDeviceContainer f2 = pointToPoint.Install(nodes2.Get(1), nodes.Get(9));
  address.SetBase("10.1.20.0", "255.255.255.0");
  Ipv4InterfaceContainer g2 = address.Assign(f2);
  update_node_map(9, g2.GetAddress(0));
  nodes2.Get(1)->isNic = true;
  printf(" interfaces.GetAddress(i) is %d\n\n",  g2.GetAddress(0).Get());




  // Later we add IP Addresses
  NS_LOG_INFO("Assign IP Addresses.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  PointToPointHelper p2p;
  p2p.EnablePcapAll("saved_file"); // filename without .pcap extention



  PacketSinkHelper sink("ns3::TcpSocketFactory",
                        InetSocketAddress(Ipv4Address::GetAny(), 9));

  
  initialize_interface_var(nodes, i1, protocol_name);
  start_lgs(filename_goal);

  // std::cout << "Total Bytes Received Node(1): " << sink1->GetTotalRx () << std::endl;
  // std::cout << "Total Bytes Received Node(3): " << sink3->GetTotalRx () << std::endl;
  printf("End called at %" PRIu64 "\n", Simulator::Now().GetNanoSeconds());

  Simulator::Destroy();
  return 0;
}
