/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/bulk-send-application.h"
#include "ns3/logsim.h"
#include "ns3/logsim-helper.h"
#include "ns3/logsim-module.h"
#include "ns3/packet-sink.h"
#include <string>
#include <utility>

namespace ns3 {

/* ... */

    //Ipv4InterfaceContainer i;
    NodeContainer nodes;
    std::unordered_map<std::string, BulkSendHelper> active_connections;
    std::unordered_map<std::string, MsgInfo> active_sends;
    graph_node_properties latest_receive_to_return;

    //std::unordered_map<Ipv4Address, int> ip_to_node_map;
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
        return temp;
    }


    /*
    void update_node_map(int my_node,  Ipv4Address ip_addr) {
        ip_to_node_map[ip_addr] = my_node;
    }

    int get_node_from_ip(Ipv4Address ip_ad) {
        return ip_to_node_map[ip_ad];
    }

    Ipv4Address get_ip_from_node(int node) {
        for (std::unordered_map<Ipv4Address, int>::const_iterator it = ip_to_node_map.begin(); it != ip_to_node_map.end(); ++it) {
            if (it->second == node) {
                return it->first;
            }
        } 
    }
    */
    void update_active_map(std::string to_hash, int size) {
        active_sends[to_hash].bytes_left_to_recv = active_sends[to_hash].bytes_left_to_recv - size;
        printf("Updated is %d\n", active_sends[to_hash].bytes_left_to_recv);
        if (active_sends[to_hash].bytes_left_to_recv <= 0) {
            active_sends.erase(to_hash);
        }
    }

    bool all_sends_delivered() {
        printf("\t\t\t\t\tALL SENDS DELIVERED Size is %d\n", active_sends.size());
        /*for (auto const &pair: active_sends) {
            std::cout << "{" << pair.first << ": " << pair.second << "}\n";
        }*/
        return active_sends.size() == 0;
    }


    void initialize_interface_var(/*Ipv4InterfaceContainer ic, */NodeContainer nodesc) {
        //i = ic;
        nodes = nodesc;
    }

    void start_lgss(std::string test) {
        
        printf("Test\n");
        return;
    }

    void SentPacketTrace(std::string context, Ptr<const Packet> pkt) {
        //printf("\n\nSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSent at %f, size %d\n", Simulator::Now().GetSeconds(), pkt->GetSize());
    }

    // Function to check if the key is present or not using count()
    bool check_key(std::unordered_map<std::string, BulkSendHelper> m, std::string key) {
        // Key is not present
        if (m.count(key) == 0)
            return false;
    
        return true;
    }

    void send_event(int from, int to, int size, u_int64_t start_time_event) {
        //
        // Create a BulkSendApplication and install it on node 0
        //
        Config::Connect("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/PhyTxBegin",MakeCallback(&SentPacketTrace));


        // Check if we have already enstablished a connection for the specific src/dest pair. If we have just get it and update how much data to send.
        std::string to_hash = std::to_string(from) + std::to_string(to);
        ApplicationContainer sourceApps;
        if (check_key(active_connections, to_hash) && true) {
            active_connections.at(to_hash).SetAttribute ("MaxBytes", UintegerValue (size));
            //sourceApps = active_connections.at(to_hash).Install (nodes.Get(from));
            printf("?????????????????? Using Stored Connection %d ????????????????? \n", ns3::NodeList::GetNode(ns3::Simulator::GetContext())->GetNApplications());
            
            ns3::Ptr<ns3::Node> node = ns3::NodeList::GetNode(ns3::Simulator::GetContext());
            //node->GetApplication(0)->SetAttribute ("MaxBytes", UintegerValue (size));

            int num_apps = node->GetNApplications();
            for (int i = 0; i < num_apps; i++) {                
                if ( node->GetApplication(i)->GetInstanceTypeId() == BulkSendApplication::GetTypeId() ) {
                    Ptr<BulkSendApplication> bulk_app = DynamicCast <BulkSendApplication> (node->GetApplication(i));
                    printf("XXXXXXXXXX %s %s A\n", node->GetApplication(i)->GetInstanceTypeId().GetName().c_str(), bulk_app->app_id.c_str());

                    if (bulk_app->app_id == to_hash) {
                        printf("YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n");
                        bulk_app->prepare_new_send();
                        bulk_app->SetAttribute ("MaxBytes", UintegerValue (size));
                    }
                }
            }

        // We need to first create the connection and then start sending.
        } else {
            uint16_t port = 9;  // well-known echo port number
            BulkSendHelper source ("ns3::TcpSocketFactory",
                                    InetSocketAddress (get_ip_from_node(to), port));
                                    
            // Set the amount of data to send in bytes.  Zero is unlimited.
            source.SetAttribute ("MaxBytes", UintegerValue (size));
            sourceApps = source.Install (nodes.Get(from));
            ns3::Ptr<ns3::Node> node = nodes.Get(from);
            int num_apps = node->GetNApplications();
            if ( node->GetApplication(num_apps - 1)->GetInstanceTypeId() == BulkSendApplication::GetTypeId() ) {
                Ptr<BulkSendApplication> bulk_app = DynamicCast <BulkSendApplication> (node->GetApplication(num_apps - 1));
                bulk_app->app_id = to_hash;
                printf("PPPPPPPPPPP %d\n", num_apps);
            }
            printf("?????????????????? Using New Connection ????????????????? \n");
           // active_connections[to_hash] = source;
            active_connections.emplace(to_hash, source);
            sourceApps.Start (NanoSeconds (Simulator::Now().GetNanoSeconds())); // check also this, weird
            sourceApps.Stop (Seconds (Simulator::Now().GetSeconds() + 10000)); // check this, seems weird we actually need it
        }
        
        

        
        
    }

    void ns3_schedule(u_int32_t host, int to, int size, u_int64_t start_time_event, int my_offset) {
        //size = 1000;
        printf("Scheduling Event of size %d from %d to %d - Time is %" PRIu64 "\n", size, host, to, Simulator::Now().GetNanoSeconds());
        Simulator::Schedule(NanoSeconds(1), &send_event, host, to, size, start_time_event);

        // Save Event 
        std::string to_hash = std::to_string(host) + std::to_string(to);
        MsgInfo entry;
        entry.start_time = start_time_event;
        entry.total_bytes_msg = size;
        entry.offset = my_offset;
        entry.bytes_left_to_recv = size;
        active_sends[to_hash] = entry;
        //printf("Hashing 1 is %s -> %d\n", to_hash.c_str(),  active_sends[to_hash]);
    }

    int ns3_simulate_until(u_int64_t until, graph_node_properties *received_op) {
        printf("NS-3 Running. Time is %" PRIu64 ". Running until %" PRIu64 "\n", Simulator::Now().GetNanoSeconds(), Simulator::Now().GetNanoSeconds() + until);
        if (until == -1) {
            Simulator::Stop();
        } else {
            Simulator::Stop(NanoSeconds(until));
        }
        latest_receive_to_return.updated = false;
        Simulator::Run();
        //printf("\n////////////////////\n");
        printf("Received updated is %d\n", latest_receive_to_return.updated);
        *received_op = latest_receive_to_return;
        return Simulator::Now().GetNanoSeconds();
    }


    void update_latest_receive(graph_node_properties recv_op) {
        latest_receive_to_return = recv_op;
    }

    void ns3_terminate(int64_t& current_time) {
        // Close all TCP connections
        ns3::Ptr<ns3::Node> node = ns3::NodeList::GetNode(ns3::Simulator::GetContext());

        int num_apps = node->GetNApplications();


        for (int j = 0; j < nodes.GetN(); j++) {
            for (int i = 0; i < num_apps; i++) {                
                if ( nodes.Get(j)->GetApplication(i)->GetInstanceTypeId() == BulkSendApplication::GetTypeId() ) {
                    Ptr<BulkSendApplication> bulk_app = DynamicCast <BulkSendApplication> (nodes.Get(j)->GetApplication(i));

                    if (bulk_app->app_id.length() >= 2) {
                        bulk_app->termiante_conn();
                    }
                }
            }
        }
        current_time = Simulator::Now().GetNanoSeconds();
        Simulator::Stop();
        Simulator::Run();
        //Simulator::Destroy();
    }

}

