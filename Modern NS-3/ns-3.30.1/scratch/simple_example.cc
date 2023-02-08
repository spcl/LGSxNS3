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

    // Creating NS3 Nodes
    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper pointToPoint;
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
