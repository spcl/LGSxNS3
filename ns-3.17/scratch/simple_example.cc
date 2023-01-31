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

int main(int argc, char *argv[])
{

    // Start Code Reserved for LGS and Fat Tree input
    CommandLine cmd;
    std::string filename_goal, protocol_name, packet_size, k;
    cmd.AddValue("goal_filename", "GOAL binary input file", filename_goal);
    cmd.AddValue("protocol", "TCP or UDP", protocol_name);
    cmd.AddValue("packet_size", "Payload Size of a packet", packet_size);
    cmd.Parse(argc, argv);
    int payload_size = std::stoi(packet_size);
    // End Code Reserved for LGS 


  // RoCE parameters
  Time::SetResolution(Time::NS);
  bool enable_qcn = true, use_dynamic_pfc_threshold = false, packet_level_ecmp = false, flow_level_ecmp = true;
  uint32_t packet_payload_size = payload_size, l2_chunk_size = 4000, l2_ack_interval = 2048;
  double pause_time = 5, simulator_stop_time = 3.01, app_start_time = 1.0, app_stop_time = 9.0;

  double cnp_interval = 50, alpha_resume_interval = 55, rp_timer, dctcp_gain = 1 / 16, np_sampling_interval = 0, pmax = 1;
  uint32_t byte_counter, fast_recovery_times = 5, kmax = 60, kmin = 60;
  std::string rate_ai, rate_hai;

  bool clamp_target_rate = true, clamp_target_rate_after_timer = false, send_in_chunks = false, l2_wait_for_ack = false, l2_back_to_zero = false, l2_test_read = false;
  double error_rate_per_link = 0.0;

  Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(packet_level_ecmp));
  Config::SetDefault("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(flow_level_ecmp));
  Config::SetDefault("ns3::QbbNetDevice::PauseTime", UintegerValue(pause_time));
  Config::SetDefault("ns3::QbbNetDevice::QcnEnabled", BooleanValue(enable_qcn));
  Config::SetDefault("ns3::QbbNetDevice::ClampTargetRate", BooleanValue(clamp_target_rate));
  Config::SetDefault("ns3::QbbNetDevice::ClampTargetRateAfterTimeInc", BooleanValue(clamp_target_rate_after_timer));
  Config::SetDefault("ns3::QbbNetDevice::CNPInterval", DoubleValue(cnp_interval));
  Config::SetDefault("ns3::QbbNetDevice::NPSamplingInterval", DoubleValue(np_sampling_interval));
  Config::SetDefault("ns3::QbbNetDevice::AlphaResumInterval", DoubleValue(alpha_resume_interval));
  Config::SetDefault("ns3::QbbNetDevice::RPTimer", DoubleValue(rp_timer));
  Config::SetDefault("ns3::QbbNetDevice::ByteCounter", UintegerValue(byte_counter));
  Config::SetDefault("ns3::QbbNetDevice::FastRecoveryTimes", UintegerValue(fast_recovery_times));
  Config::SetDefault("ns3::QbbNetDevice::DCTCPGain", DoubleValue(dctcp_gain));
  Config::SetDefault("ns3::QbbNetDevice::L2BackToZero", BooleanValue(l2_back_to_zero));
  Config::SetDefault("ns3::QbbNetDevice::L2TestRead", BooleanValue(l2_test_read));
  Config::SetDefault("ns3::QbbNetDevice::L2ChunkSize", UintegerValue(l2_chunk_size));
  Config::SetDefault("ns3::QbbNetDevice::L2AckInterval", UintegerValue(l2_ack_interval));
  Config::SetDefault("ns3::QbbNetDevice::L2WaitForAck", BooleanValue(l2_wait_for_ack));

    // Creating NS3 Nodes
    NodeContainer nodes;
    nodes.Create(2);

    QbbHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
    NetDeviceContainer devices;
    devices = pointToPoint.Install (nodes);

    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces = address.Assign (devices);

    // Later we add IP Addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");


   // Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    for (int i = 0; i < NodeList::GetNNodes(); i++) {
        update_node_map(i,  interfaces.GetAddress(i));
        nodes.Get(i)->isNic = true;
    }

    PointToPointHelper p2p;
    p2p.EnablePcapAll("saved_file");

    PacketSinkHelper sink("ns3::TcpSocketFactory",
                        InetSocketAddress(Ipv4Address::GetAny(), 9));

    Ipv4InterfaceContainer dummy;
    initialize_interface_var(nodes, dummy, protocol_name, payload_size);
    start_lgs(filename_goal);

    Simulator::Destroy();
    return 0;
}
