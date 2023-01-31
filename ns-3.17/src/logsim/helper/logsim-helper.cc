/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/bulk-send-application.h"
#include "ns3/logsim.h"
#include "ns3/logsim-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/logsim-module.h"
#include "ns3/packet-sink.h"
#include <string>
#include <utility>
#define DEBUG_PRINT 0

namespace ns3 {

/* ... */

    // Global Variables
    bool isTCP;
    uint16_t port = 9;  // well-known echo port number
    Ptr<FlowMonitor> flowMonitor;
    Ipv4InterfaceContainer i;
    NodeContainer nodes;
    int global_payload_size;
    std::unordered_map<std::string, BulkSendHelper> active_connections;
    std::unordered_map<std::string, MsgInfo> active_sends;
    bool ports[65000];
    graph_node_properties latest_receive_to_return;
    bool has_updated_recv = false;
    std::unordered_map<std::string, u_int64_t> start_time_flow;
    std::pair<Ipv4Address, int> ip_to_node_map;
    std::vector <std::pair<Ipv4Address, int>> vec_addresses;

    std::unordered_map<std::string, MsgInfo> get_active_sends() {
        return active_sends;
    }

    void update_node_map(int my_node,  Ipv4Address ip_addr) {
        std::pair<Ipv4Address, int> my_pair;
        my_pair.second = my_node;
        my_pair.first = ip_addr;
        vec_addresses.push_back(my_pair);
    }

    int get_node_from_ip(Ipv4Address ip_ad) {
        for(int i = 0; i<vec_addresses.size();i++) {
            if (vec_addresses[i].first == ip_ad) {
                return vec_addresses[i].second;
            }
        }
        return -1;
    }

    Ipv4Address get_ip_from_node(int node) {
        for(int i = 0; i<vec_addresses.size();i++) {
            if (vec_addresses[i].second == node) {
                return vec_addresses[i].first;
            }
        }
        Ipv4Address temp;
        printf("This is wrong");
        return temp;
    }

    void update_active_map(std::string to_hash, int size) {

        // Check that the flow actually exists
        active_sends[to_hash].bytes_left_to_recv = active_sends[to_hash].bytes_left_to_recv - size;
        //printf("Updated is %d\n", active_sends[to_hash].bytes_left_to_recv);
        if (active_sends[to_hash].bytes_left_to_recv <= 0) {
            active_sends.erase(to_hash);
        }
    }

    bool all_sends_delivered() {
        if (DEBUG_PRINT)
            printf("\t\t\t\t\tALL SENDS DELIVERED Size is %d\n", active_sends.size());
        return active_sends.size() == 0;
    }

    void SentPacketTrace(std::string context, Ptr<const Packet> pkt) {
        CustomDataTag tag;
        if (pkt->FindFirstMatchingByteTag(tag))
        {
        } else {
            return;
        }
        ns3::Ptr<ns3::Node> node = ns3::NodeList::GetNode(ns3::Simulator::GetContext());
        if (!node->isNic) {
            return;
        }
    }


    void DropTrace(std::string context, Ptr<const Packet> pkt) {
        CustomDataTag tag;
        if (pkt->FindFirstMatchingByteTag(tag))
        {
        } else {
            return;
        }
        ns3::Ptr<ns3::Node> node = ns3::NodeList::GetNode(ns3::Simulator::GetContext());
        if (!node->isNic) {
            return;
        }
        if (DEBUG_PRINT)
            printf("\nDropped a packet %d at %f, size %d\n", node->GetId(), Simulator::Now().GetSeconds(),  pkt->GetSize());
    }

    // Function to check if the key is present or not using count()
    bool check_key(std::unordered_map<std::string, BulkSendHelper> m, std::string key) {
        // Key is not present
        if (m.count(key) == 0)
            return false;
    
        return true;
    }

    void SinkRxTrace(std::string context, Ptr<const Packet> pkt)
    {
        ns3::Ptr<ns3::Node> node = ns3::NodeList::GetNode(ns3::Simulator::GetContext());
        if (!node->isNic) {
            if (DEBUG_PRINT)
                printf("I am Node %d - Am I am final NIC? %d - Size %d\n", node->GetId(), node->isNic, pkt->GetSize());
            return;
        }

        CustomDataTag tag;
        if (pkt->FindFirstMatchingByteTag(tag))
        {
            if (DEBUG_PRINT)
                printf("I am Node %d (from %d) - Am I am final NIC? %d - Size %d\n", tag.GetNodeId(), node->GetId(), node->isNic, pkt->GetSize());
        } else {

            if (DEBUG_PRINT)
                printf("No TAG I am Node %d (from %d) - Am I am final NIC? %d - Size %d\n",tag.GetNodeId(), node->GetId(), node->isNic, pkt->GetSize());
            return;
        }

        std::string to_hash = std::to_string(get_node_from_ip(Ipv4Address(tag.GetNodeId()))) + "@" + std::to_string(node->GetId()) + "@" + std::to_string(tag.GetPersonalTag());
        std::unordered_map<std::string, MsgInfo> active_sends_l = get_active_sends();
        if (DEBUG_PRINT)
            printf("Received Hash %s, tag %d, size %d - Left %d || Time %ld - \n", to_hash.c_str(), tag.GetPersonalTag(), pkt->GetSize(), get_active_sends()[to_hash].bytes_left_to_recv, Simulator::Now().GetNanoSeconds());
        int msg_size = get_active_sends()[to_hash].total_bytes_msg;
        update_active_map(to_hash, global_payload_size);

        if (start_time_flow.count(to_hash) == 0) {
            start_time_flow[to_hash] = Simulator::Now().GetNanoSeconds();
        }

        // Here we have received a message fully, we need to give control back to LGS
        if (get_active_sends()[to_hash].bytes_left_to_recv <= 0)
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
            latest_element.size = msg_size;
            latest_element.offset = get_active_sends()[to_hash].offset;
            latest_element.proc = 0;
            latest_element.nic = 0;

            update_latest_receive(latest_element);
            printf("Flow Bandwidth is %f - Size %d - Time %ld\n", (msg_size / (double)(Simulator::Now().GetNanoSeconds() - start_time_flow[to_hash])) * 8, msg_size, (Simulator::Now().GetNanoSeconds() - start_time_flow[to_hash]));
            printf("Msg received, NS-3 giving control to LGS \n");
            return;
        }

        graph_node_properties latest_element;
        update_latest_receive(latest_element);
    }

    void initialize_interface_var(NodeContainer nodesc, Ipv4InterfaceContainer ic, std::string pro, int payload_size) {
        i = ic;
        nodes = nodesc;
        global_payload_size = payload_size;
        Config::Connect("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/PhyRxEnd",MakeCallback(&SinkRxTrace));
        Config::Connect("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/PhyTxBegin",MakeCallback(&SentPacketTrace));
        Config::Connect("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/PhyTxDrop",MakeCallback(&DropTrace));
        Config::Connect("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/PhyRxDrop",MakeCallback(&DropTrace));
        Config::Connect("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/MacTxDrop",MakeCallback(&DropTrace));

        if (pro == "TCP") {
            isTCP = true;
            PacketSinkHelper sink("ns3::TcpSocketFactory",
                                  InetSocketAddress(Ipv4Address::GetAny(), 9));
            sink.Install(nodes);
        } else if (pro == "UDP") {
            isTCP = false;
        } else {
            exit(0);
        }
    }

    void send_event(int from, int to, int size, int tag, u_int64_t start_time_event) {
        if (!isTCP) {
            /*printf("Setting UDP");
            int pg = 1;
            int max_pack = (float)size / global_payload_size + 1;
            UdpServerHelper server0(port); //Add Priority
            ApplicationContainer apps0s = server0.Install(nodes.Get(to));
            apps0s.Start (Seconds (0)); // check also this, weird
            apps0s.Stop (Seconds (Simulator::Now().GetSeconds() + 10000)); // check this, seems weird we actually need it
            printf("Helper setting %d %d\n", to, get_ip_from_node(to));
            UdpClientHelper client0(get_ip_from_node(to), port); //Add Priority
            client0.SetAttribute("MaxPackets", UintegerValue(max_pack));
            client0.SetAttribute("Interval", TimeValue(Seconds (0)));
            client0.SetAttribute("PacketSize", UintegerValue(global_payload_size));
            ApplicationContainer apps0c = client0.Install(nodes.Get(from));
            client0.SetPairs(apps0c.Get(0), vec_addresses);
            apps0c.Start (Seconds (0)); // check also this, weird
            apps0c.Stop (Seconds (Simulator::Now().GetSeconds() + 10000)); // check this, seems weird we actually need it
            port++;*/
            //SeedManager::SetSeed(42);
            if (DEBUG_PRINT)
                printf("Setting UDP");
            Ptr<UniformRandomVariable> m_rand = CreateObject<UniformRandomVariable> ();
            /*while (1) {
                int random_port = m_rand->GetInteger(0, 65535);
                if (ports[random_port] == false) {
                    port = random_port;
                    ports[random_port] = true;
                    break;
                }
            }*/
            port = 9;

            int max_pack = (float)size / global_payload_size;
            if (size % global_payload_size  != 0) {
                max_pack++;
            }
            if (DEBUG_PRINT)
                printf("Sending %d packets, size %d, packet size %d, division %d, modulo \n", max_pack, size, global_payload_size, (float)size / global_payload_size);
            UdpEchoServerHelper server0(port, 1); //Add Priority
            ApplicationContainer apps0s = server0.Install(nodes.Get(to));
            apps0s.Start (Seconds (0)); // check also this, weird
            apps0s.Stop (Seconds (Simulator::Now().GetSeconds() + 10000)); // check this, seems weird we actually need it
            UdpEchoClientHelper client0((Address)get_ip_from_node(to), port, 1); //Add Priority
            client0.SetAttribute("MaxPackets", UintegerValue(max_pack));
            client0.SetAttribute("Interval", TimeValue(Seconds (0.0)));
            client0.SetAttribute("PacketSize", UintegerValue(global_payload_size));
            client0.SetAttribute("Tag", UintegerValue(tag));
            ApplicationContainer apps0c = client0.Install(nodes.Get(from));
            client0.SetPairs(apps0c.Get(0), vec_addresses);
            apps0c.Start (Seconds (0)); // check also this, weird
            apps0c.Stop (Seconds (Simulator::Now().GetSeconds() + 10000)); // check this, seems weird we actually need it
            //port++;
        } else {
            // Check if we have already enstablished a connection for the specific src/dest pair. If we have just get it and update how much data to send.
            std::string to_hash = std::to_string(from) + "@" + std::to_string(to) + "@" + std::to_string(tag);
            ApplicationContainer sourceApps;
            if (check_key(active_connections, to_hash) && false) {
                active_connections.at(to_hash).SetAttribute ("MaxBytes", UintegerValue (size));
                sourceApps = active_connections.at(to_hash).Install (nodes.Get(from));
                if (DEBUG_PRINT)
                    printf("Using Stored Connection %d\n", ns3::NodeList::GetNode(ns3::Simulator::GetContext())->GetNApplications());
                
                ns3::Ptr<ns3::Node> node = ns3::NodeList::GetNode(ns3::Simulator::GetContext());
                //node->GetApplication(0)->SetAttribute ("MaxBytes", UintegerValue (size));

                int num_apps = node->GetNApplications();
                for (int i = 0; i < num_apps; i++) {                
                    if ( node->GetApplication(i)->GetInstanceTypeId() == BulkSendApplication::GetTypeId() ) {
                        Ptr<BulkSendApplication> bulk_app = DynamicCast <BulkSendApplication> (node->GetApplication(i));
                        //printf("XXXXXXXXXX %s %s A\n", node->GetApplication(i)->GetInstanceTypeId().GetName().c_str(), bulk_app->app_id.c_str());
                        if (bulk_app->app_id == to_hash) {
                            bulk_app->prepare_new_send();
                            bulk_app->SetAttribute ("MaxBytes", UintegerValue (size));
                        }
                    }
                }

            // We need to first create the connection and then start sending.
            } else {
                if (DEBUG_PRINT)
                    printf("Installing App to %d -> %d\n",nodes.Get(from)->GetId(), get_ip_from_node(to).Get()); fflush(stdout);
                //uint16_t port = 9;  // well-known echo port number
                BulkSendHelper source ("ns3::TcpSocketFactory",
                                        InetSocketAddress (get_ip_from_node(to), port));
                                        
                // Set the amount of data to send in bytes.  Zero is unlimited.
                source.SetAttribute ("MaxBytes", UintegerValue (size));
                sourceApps = source.Install (nodes.Get(from));
                ns3::Ptr<ns3::Node> node = nodes.Get(from);
                int num_apps = node->GetNApplications();
                Ptr<BulkSendApplication> bulk_app;
                if ( node->GetApplication(num_apps - 1)->GetInstanceTypeId() == BulkSendApplication::GetTypeId() ) {
                    bulk_app = DynamicCast <BulkSendApplication> (node->GetApplication(num_apps - 1));
                    bulk_app->app_id = to_hash;
                } else {
                    printf("Error, exiting");
                    exit(0);
                }
                if (DEBUG_PRINT)
                    printf("Using New Connection\n");
                active_connections.emplace(to_hash, source);
                source.SetPairs(bulk_app, vec_addresses);
                sourceApps.Start (NanoSeconds (0)); 
                sourceApps.Stop (Seconds (Simulator::Now().GetSeconds() + 10000));
                //port++;
            }
        }
        
    }

    void ns3_schedule(u_int32_t host, int to, int size, int tag, u_int64_t start_time_event, int my_offset) {
        //size = 1000;
        Simulator::ScheduleNow(&send_event, host, to, size, tag, start_time_event);

        // Save Event 
        std::string to_hash = std::to_string(host) + "@" + std::to_string(to) + "@" + std::to_string(tag);
        MsgInfo entry;
        entry.start_time = start_time_event;
        entry.total_bytes_msg = size;
        entry.offset = my_offset;
        entry.bytes_left_to_recv = size;
        active_sends[to_hash] = entry;
        //printf("Hashing 1 is %s -> %d\n", to_hash.c_str(),  active_sends[to_hash]);
        if (DEBUG_PRINT)
            printf("Scheduling Event (%s) of size %d from %d to %d tag %d - Time is %" PRIu64 ", %f\n", to_hash.c_str(), size, host, to, tag, Simulator::Now().GetNanoSeconds(), Simulator::Now().GetSeconds());
    }

    int64_t ns3_simulate_until(u_int64_t until, graph_node_properties *received_op) {
        if (DEBUG_PRINT)
            printf("NS-3 Running. Time is %" PRIu64 ". Running until %" PRIu64 "\n", Simulator::Now().GetNanoSeconds(), Simulator::Now().GetNanoSeconds() + until);
        if (until == -1) {
            Simulator::Stop();
        } else {
            Simulator::Stop(NanoSeconds(until));
        }
        latest_receive_to_return.updated = false;
        Simulator::Run();
        if (DEBUG_PRINT)
            printf("Received updated is %d - Time %ld\n", latest_receive_to_return.updated, Simulator::Now().GetNanoSeconds());
        *received_op = latest_receive_to_return;
        return Simulator::Now().GetNanoSeconds();
    }


    void update_latest_receive(graph_node_properties recv_op) {
        latest_receive_to_return = recv_op;
    }

    void ns3_terminate(int64_t& current_time) {
        // Close all connections

        //flowMonitor->SerializeToXmlFile("NameOfFile.xml", true, true);
        //flowMonitor->StopRightNow();

        /*ns3::Ptr<ns3::Node> node = ns3::NodeList::GetNode(ns3::Simulator::GetContext());

        for (int j = 0; j < nodes.GetN(); j++) {
            int num_apps = nodes.Get(j)->GetNApplications();
            for (int i = 0; i < num_apps; i++) {          
                if ( nodes.Get(j)->GetApplication(i)->GetInstanceTypeId() == BulkSendApplication::GetTypeId() ) {
                    Ptr<BulkSendApplication> bulk_app = DynamicCast <BulkSendApplication> (nodes.Get(j)->GetApplication(i));
                    if (bulk_app->app_id.length() >= 2) {
                        bulk_app->termiante_conn();
                    }
                }
            }
        }*/
        if (DEBUG_PRINT)
            printf("Terminating this"); fflush(stdout);
        current_time = Simulator::Now().GetNanoSeconds();
        Simulator::Stop(Seconds(1));
        Simulator::Run();
    }

}

