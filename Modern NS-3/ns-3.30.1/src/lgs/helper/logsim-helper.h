/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef LOGSIM_HELPER_H
#define LOGSIM_HELPER_H

#include "ns3/logsim.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/logsim.h"
#include <string>
#include <unordered_map>


namespace ns3 {

/* ... */
    class MsgInfo {
        // Access specifier
    public:
        // Data  Members
        int total_bytes_msg;
        int bytes_left_to_recv;
        int identifier;
        u_int64_t start_time;
        int offset;
    };


    int start_lgs(std::string);
    void ns3_schedule(u_int32_t, int, int, int, u_int64_t, int);
    int64_t ns3_simulate_until(u_int64_t, graph_node_properties*);
    void initialize_interface_var(NodeContainer nodesc, Ipv4InterfaceContainer ic, std::string, int);
    std::unordered_map<std::string, MsgInfo> get_active_sends();
    void update_active_map(std::string, int);
    void update_node_map(int,  Ipv4Address);
    int get_node_from_ip(Ipv4Address);
    bool all_sends_delivered();
    graph_node_properties check_new_recv();
    void update_latest_receive(graph_node_properties recv_op);
    Ipv4Address get_ip_from_node(int node);
    void ns3_terminate(int64_t& current_time);
}

#endif /* LOGSIM_HELPER_H */

