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
  nodes.Create(4);

  bool enable_qcn = false, use_dynamic_pfc_threshold = false, packet_level_ecmp = false, flow_level_ecmp = true;
  uint32_t packet_payload_size = 512, l2_chunk_size = 4000, l2_ack_interval = 2048;
  double pause_time = 5, simulator_stop_time = 3.01, app_start_time = 1.0, app_stop_time = 9.0;

  double cnp_interval = 50, alpha_resume_interval = 55, rp_timer, dctcp_gain = 1 / 16, np_sampling_interval = 0, pmax = 1;
  uint32_t byte_counter, fast_recovery_times = 5, kmax = 60, kmin = 60;
  std::string rate_ai, rate_hai;

  bool clamp_target_rate = true, clamp_target_rate_after_timer = false, send_in_chunks = false, l2_wait_for_ack = false, l2_back_to_zero = false, l2_test_read = false;
  double error_rate_per_link = 0.0;
  
  int sid = 3;
  nodes.Get(sid)->SetNodeType(1, true); //broadcom switch
  nodes.Get(sid)->m_broadcom->SetMarkingThreshold(kmin, kmax, pmax);

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

  NetDeviceContainer d1 = pointToPoint.Install(nodes.Get(0), nodes.Get(3));
  Ipv4InterfaceContainer i1 = address.Assign(d1);
  update_node_map(0, i1.GetAddress(0));
  nodes.Get(0)->isNic = true;
  printf(" interfaces.GetAddress(i) is %d\n\n",  i1.GetAddress(0).Get());

  NetDeviceContainer d2 = pointToPoint.Install(nodes.Get(1), nodes.Get(3));
  Ipv4InterfaceContainer i2 = address.Assign(d2);
  update_node_map(1, i2.GetAddress(0));
  nodes.Get(1)->isNic = true;
  printf(" interfaces.GetAddress(i) is %d\n\n",  i2.GetAddress(0).Get());
  
  NetDeviceContainer d3 = pointToPoint.Install(nodes.Get(2), nodes.Get(3));
  Ipv4InterfaceContainer i3 = address.Assign(d3);
  update_node_map(2, i3.GetAddress(0));
  nodes.Get(2)->isNic = true;
  printf(" interfaces.GetAddress(i) is %d\n\n",  i3.GetAddress(0).Get());

  nodes.Get(3)->isNic = false;
  // Later we add IP Addresses
  NS_LOG_INFO("Assign IP Addresses.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  PointToPointHelper p2p;
  p2p.EnablePcapAll("saved_file"); // filename without .pcap extention



  PacketSinkHelper sink("ns3::TcpSocketFactory",
                        InetSocketAddress(Ipv4Address::GetAny(), 9));

  
  initialize_interface_var(nodes, i3, protocol_name);
  start_lgs(filename_goal);

  // std::cout << "Total Bytes Received Node(1): " << sink1->GetTotalRx () << std::endl;
  // std::cout << "Total Bytes Received Node(3): " << sink3->GetTotalRx () << std::endl;
  printf("End called at %" PRIu64 "\n", Simulator::Now().GetNanoSeconds());

  Simulator::Destroy();
  return 0;
}
