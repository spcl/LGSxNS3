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


  bool enable_qcn = true, use_dynamic_pfc_threshold = true, packet_level_ecmp = false, flow_level_ecmp = true;
  uint32_t packet_payload_size = 512, l2_chunk_size = 4000, l2_ack_interval = 4096;
  double pause_time = 5, simulator_stop_time = 3.01, app_start_time = 1.0, app_stop_time = 9.0;

  double cnp_interval = 50, alpha_resume_interval = 55, rp_timer, dctcp_gain = 1 / 16, np_sampling_interval = 0, pmax = 1;
  uint32_t byte_counter, fast_recovery_times = 5, kmax = 60, kmin = 60;
  std::string rate_ai, rate_hai;

  bool clamp_target_rate = true, clamp_target_rate_after_timer = false, send_in_chunks = false, l2_wait_for_ack = false, l2_back_to_zero = false, l2_test_read = false;
  double error_rate_per_link = 0.0;

  NodeContainer c;
  c.Create(8);
  NodeContainer l1, l2, l5, l6, l7, l8;
  l1.Create(1);
  l2.Create(1);
  l5.Create(1);
  l6.Create(1);
  l7.Create(1);
  l8.Create(1);

  l1.Get(0)->SetNodeType(1, true); //broadcom switch
  l1.Get(0)->m_broadcom->SetMarkingThreshold(kmin, kmax, pmax);

  l2.Get(0)->SetNodeType(1, true); //broadcom switch
  l2.Get(0)->m_broadcom->SetMarkingThreshold(kmin, kmax, pmax);

  l5.Get(0)->SetNodeType(1, true); //broadcom switch
  l5.Get(0)->m_broadcom->SetMarkingThreshold(kmin, kmax, pmax);

  l6.Get(0)->SetNodeType(1, true); //broadcom switch
  l6.Get(0)->m_broadcom->SetMarkingThreshold(kmin, kmax, pmax);

  l7.Get(0)->SetNodeType(1, true); //broadcom switch
  l7.Get(0)->m_broadcom->SetMarkingThreshold(kmin, kmax, pmax);

  l8.Get(0)->SetNodeType(1, true); //broadcom switch
  l8.Get(0)->m_broadcom->SetMarkingThreshold(kmin, kmax, pmax);


  NodeContainer n1 = NodeContainer(c.Get(0), l1);
  NodeContainer n2 = NodeContainer(c.Get(1), l1);
  NodeContainer n3 = NodeContainer(c.Get(2), l1);
  NodeContainer n4 = NodeContainer(c.Get(3), l1);
  NodeContainer n5 = NodeContainer(c.Get(4), l2);
  NodeContainer n6 = NodeContainer(c.Get(5), l2);
  NodeContainer n7 = NodeContainer(c.Get(6), l2);
  NodeContainer n8 = NodeContainer(c.Get(7), l2);

  NodeContainer n15 = NodeContainer(l1, l5);
  NodeContainer n16 = NodeContainer(l1, l6);
  NodeContainer n17 = NodeContainer(l1, l7);
  NodeContainer n18 = NodeContainer(l1, l8);

  NodeContainer n25 = NodeContainer(l2, l5);
  NodeContainer n26 = NodeContainer(l2, l6);
  NodeContainer n27 = NodeContainer(l2, l7);
  NodeContainer n28 = NodeContainer(l2, l8);

  InternetStackHelper internet;
  internet.Install(c.Get(0));
  internet.Install(c.Get(1));
  internet.Install(c.Get(2));
  internet.Install(c.Get(3));
  internet.Install(c.Get(4));
  internet.Install(c.Get(5));
  internet.Install(c.Get(6));
  internet.Install(c.Get(7));
  internet.Install(l1);
  internet.Install(l2);
  internet.Install(l5);
  internet.Install(l6);
  internet.Install(l7);
  internet.Install(l8);

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

  QbbHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("1000Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("50ns"));

  NetDeviceContainer d1 = p2p.Install(n1);
  NetDeviceContainer d2 = p2p.Install(n2);
  NetDeviceContainer d3 = p2p.Install(n3);
  NetDeviceContainer d4 = p2p.Install(n4);
  NetDeviceContainer d5 = p2p.Install(n5);
  NetDeviceContainer d6 = p2p.Install(n6);
  NetDeviceContainer d7 = p2p.Install(n7);
  NetDeviceContainer d8 = p2p.Install(n8);
  
  NetDeviceContainer d15 = p2p.Install(n15);
  NetDeviceContainer d16 = p2p.Install(n16);
  NetDeviceContainer d17 = p2p.Install(n17);
  NetDeviceContainer d18 = p2p.Install(n18);

  NetDeviceContainer d25 = p2p.Install(n25);
  NetDeviceContainer d26 = p2p.Install(n26);
  NetDeviceContainer d27 = p2p.Install(n27);
  NetDeviceContainer d28 = p2p.Install(n28);

  // Later we add IP Addresses
  NS_LOG_INFO("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  

  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i1 = ipv4.Assign(d1);
  ipv4.SetBase("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i2 = ipv4.Assign(d2);
  ipv4.SetBase("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i3 = ipv4.Assign(d3);
  ipv4.SetBase("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer i4 = ipv4.Assign(d4);
  ipv4.SetBase("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer i5 = ipv4.Assign(d5);
  ipv4.SetBase("10.1.6.0", "255.255.255.0");
  Ipv4InterfaceContainer i6 = ipv4.Assign(d6);
  ipv4.SetBase("10.1.7.0", "255.255.255.0");
  Ipv4InterfaceContainer i7 = ipv4.Assign(d7);
  ipv4.SetBase("10.1.8.0", "255.255.255.0");
  Ipv4InterfaceContainer i8 = ipv4.Assign(d8);

  ipv4.SetBase("10.1.9.0", "255.255.255.0");
  Ipv4InterfaceContainer i11 = ipv4.Assign(d15);
  ipv4.SetBase("10.1.10.0", "255.255.255.0");
  Ipv4InterfaceContainer i12 = ipv4.Assign(d16);
  ipv4.SetBase("10.1.11.0", "255.255.255.0");
  Ipv4InterfaceContainer i13 = ipv4.Assign(d17);
  ipv4.SetBase("10.1.12.0", "255.255.255.0");
  Ipv4InterfaceContainer i14 = ipv4.Assign(d18);
  ipv4.SetBase("10.1.13.0", "255.255.255.0");
  Ipv4InterfaceContainer i15 = ipv4.Assign(d25);
  ipv4.SetBase("10.1.14.0", "255.255.255.0");
  Ipv4InterfaceContainer i16 = ipv4.Assign(d26);
  ipv4.SetBase("10.1.15.0", "255.255.255.0");
  Ipv4InterfaceContainer i17 = ipv4.Assign(d27);
  ipv4.SetBase("10.1.16.0", "255.255.255.0");
  Ipv4InterfaceContainer i18 = ipv4.Assign(d28);

  
  // Create router nodes, initialize routing database and set up the routing
  // tables in the nodes.
  //
  Ipv4GlobalRoutingHelper:: PopulateRoutingTables();
  Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(2));

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
  update_node_map(4, i5.GetAddress(0));
  printf(" interfaces.GetAddress(i) is %d\n\n", i5.GetAddress(0).Get());
  update_node_map(5, i6.GetAddress(0));
  printf(" interfaces.GetAddress(i) is %d\n\n", i6.GetAddress(0).Get());
  update_node_map(6, i7.GetAddress(0));
  printf(" interfaces.GetAddress(i) is %d\n\n", i7.GetAddress(0).Get());
  update_node_map(7, i8.GetAddress(0));
  printf(" interfaces.GetAddress(i) is %d\n\n", i8.GetAddress(0).Get());

  //PointToPointHelper p2p;
  p2p.EnablePcapAll("saved_file"); // filename without .pcap extention
 
  // Create Fake Send to Synch at the beginning, run for ten seconds.
  Simulator::Stop(Seconds(0));
  Simulator::Run();

  initialize_interface_var(c, i2, protocol_name);
  start_lgs(filename_goal);

  printf("End called at %" PRIu64 "\n", Simulator::Now().GetNanoSeconds());
  Simulator::Destroy();
  return 0;
}
