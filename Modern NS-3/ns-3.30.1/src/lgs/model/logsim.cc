/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "logsim.h"
/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *            Timo Schneider <timoschn@cs.indiana.edu>
 *
 */


#define STRICT_ORDER // this is needed to keep order between Send/Recv and LocalOps in NBC case :-/
#define LIST_MATCH // enables debugging the queues (check if empty)
#define HOSTSYNC // this is experimental to count synchronization times induced by message transmissions

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <list>
#include <algorithm>

#ifndef LIST_MATCH
#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif
#endif

#include <queue>
#include "cmdline.h"

#include <sys/time.h>

#include "LogGOPSim.hpp"
#include "Network.hpp"
//#include "TimelineVisualization.hpp"
#include "Noise.hpp"
#include "Parser.hpp"
#include <chrono>
#include "logsim.h"
#include "cmdline.h"
#include "ns3/logsim-helper.h"

#define DEBUG_PRINT 0

namespace ns3 {

    static bool print=true;


    // TODO: should go to .hpp file

    typedef struct {
    // TODO: src and tag can go in case of hashmap matching
    btime_t starttime; // only for visualization
    uint32_t size, src, tag, offset;
    } ruqelem_t;

    typedef unsigned int uint;
    typedef unsigned long int ulint;
    typedef unsigned long long int ullint;

    #ifdef LIST_MATCH
    // TODO this is really slow - reconsider design of rq and uq!
    // matches and removes element from list if found, otherwise returns
    // false
    typedef std::list<ruqelem_t> ruq_t;
    static inline int match(const graph_node_properties &elem, ruq_t *q, ruqelem_t *retelem=NULL) {

    // MATCH attempts (i.e., number of elements searched to find a matching element)
    int match_attempts = 0;
    
    //std::cout << "UQ size " << q->size() << "\n";

    if(0) printf("++ size is %d,  [%i] searching matching queue for src %i tag %i\n", q->size(), elem.host, elem.target, elem.tag);
    for(ruq_t::iterator iter=q->begin(); iter!=q->end(); ++iter) {
        match_attempts++;
        if (DEBUG_PRINT)
            printf("Compared element is -> %d %d vs %d %d\n", iter->src, iter->tag, elem.target, elem.tag); fflush(stdout);
        if(elem.target == ANY_SOURCE || iter->src == ANY_SOURCE || iter->src == elem.target) {
        if(elem.tag == ANY_TAG || iter->tag == ANY_TAG || iter->tag == elem.tag) {
            if(retelem) *retelem=*iter;
            q->erase(iter);
            return match_attempts;        
        }
        }
    }
    return -1;
    }
    #else
    class myhash { // I WANT LAMBDAS! :)
    public:
    size_t operator()(const std::pair<int,int>& x) const {
        return (x.first>>16)+x.second;
    }
    };
    typedef std::hash_map< std::pair</*tag*/int,int/*src*/>, std::queue<ruqelem_t>, myhash > ruq_t;
    static inline int match(const graph_node_properties &elem, ruq_t *q, ruqelem_t *retelem=NULL) {
    
    if(print) printf("++ [%i] searching matching queue for src %i tag %i\n", elem.host, elem.target, elem.tag);

    ruq_t::iterator iter = q->find(std::make_pair(elem.tag, elem.target));
    if(iter == q->end()) {
        return -1;
    }
    std::queue<ruqelem_t> *tq=&iter->second;
    if(tq->empty()) {
        return -1;
    }
    if(retelem) *retelem=tq->front();
    tq->pop();
    return 0;
    }
    #endif


    // Returns highest queue size between processes
    int size_queue( std::vector<ruq_t> my_queue, int num_proce) {
        int max = 0;
        for (int i = 0; i < num_proce; i++) {
            if (my_queue[i].size() > max) {
                max = my_queue[i].size();
            }
        }
        return max;
    }


    int start_lgs(std::string filename_goal) {

        // Time Inside LGS
        using std::chrono::high_resolution_clock;
        using std::chrono::duration_cast;
        using std::chrono::duration;
        using std::chrono::milliseconds;
        auto start = high_resolution_clock::now();
        using std::chrono::high_resolution_clock;
        std::chrono::milliseconds global_time = std::chrono::milliseconds::zero();

        // Temp Only
        filename_goal = "src/lgs/model/" + filename_goal;
        char tmp[256];
        getcwd(tmp, 256);
        //std::cout << "Current working directory: " << tmp << std::endl;
        // End Temp Only

        #ifdef STRICT_ORDER
        btime_t aqtime=0; 
        #endif

        #ifndef LIST_MATCH
        #endif

        // read input parameters
        // For now we use hardcoded constants
        const int o=1500;
        const int O=0;
        const int g=1000;
        const int L=2500;
        const int G=6;
        print=0;
        const uint32_t S=65535;
        int64_t ns3_time = 0;

        Parser parser(filename_goal, 0);
        Network net;

        const uint p = parser.schedules.size();
        const int ncpus = parser.GetNumCPU();
        const int nnics = parser.GetNumNIC();

        printf("size: %i (%i CPUs, %i NICs); L=%i, o=%i g=%i, G=%i, O=%i, P=%i, S=%u\n", 
                p, ncpus, nnics, L, o, g, G, O, p, S);

        //TimelineVisualization tlviz(&args_info, p);
        Noise osnoise(p);

        // DATA structures for storing MPI matching statistics
        std::vector<int> rq_max(0);
        std::vector<int> uq_max(0); 

        std::vector< std::vector< std::pair<int,btime_t> > > rq_matches(0);
        std::vector< std::vector< std::pair<int,btime_t> > > uq_matches(0);

        std::vector< std::vector< std::pair<int,btime_t> > > rq_misses(0);
        std::vector< std::vector< std::pair<int,btime_t> > > uq_misses(0);

        std::vector< std::vector<btime_t> > rq_times(0);
        std::vector< std::vector<btime_t> > uq_times(0); 

        if( 0 ){
            // Initialize MPI matching data structures
            rq_max.resize(p);
            uq_max.resize(p); 

            for( int i : rq_max ) {
            rq_max[i] = 0;
            uq_max[i] = 0;
            }

            rq_matches.resize(p);
            uq_matches.resize(p);

            rq_misses.resize(p);
            uq_misses.resize(p);

            rq_times.resize(p);
            uq_times.resize(p);
        }

        // the active queue 
        std::priority_queue<graph_node_properties,std::vector<graph_node_properties>,aqcompare_func> aq;
        // the queues for each host 
        std::vector<ruq_t> rq(p), uq(p); // receive queue, unexpected queue
        // next available time for o, g(receive) and g(send)
        std::vector<std::vector<btime_t> > nexto(p), nextgr(p), nextgs(p); 
        #ifdef HOSTSYNC
        std::vector<btime_t> hostsync(p);
        #endif

        // initialize o and g for all PEs and hosts
        for(uint i=0; i<p; ++i) {
            nexto[i].resize(ncpus);
            std::fill(nexto[i].begin(), nexto[i].end(), 0);
            nextgr[i].resize(nnics);
            std::fill(nextgr[i].begin(), nextgr[i].end(), 0);
            nextgs[i].resize(nnics);
            std::fill(nextgs[i].begin(), nextgs[i].end(), 0);
        }

            struct timeval tstart, tend;
            gettimeofday(&tstart, NULL);

        int host=0; 
        uint64_t num_events=0;
        for(Parser::schedules_t::iterator sched=parser.schedules.begin(); sched!=parser.schedules.end(); ++sched, ++host) {
            // initialize free operations (only once per run!)
            //sched->init_free_operations();

            // retrieve all free operations
            //typedef std::vector<std::string> freeops_t;
            //freeops_t free_ops = sched->get_free_operations();
            SerializedGraph::nodelist_t free_ops;
            sched->GetExecutableNodes(&free_ops);
            // ensure that the free ops are ordered by type
            std::sort(free_ops.begin(), free_ops.end(), gnp_op_comp_func());

            num_events += sched->GetNumNodes();

            // walk all new free operations and throw them in the queue 
            for(SerializedGraph::nodelist_t::iterator freeop=free_ops.begin(); freeop != free_ops.end(); ++freeop) {
            //if(print) std::cout << *freeop << " " ;

            freeop->host = host;
            freeop->time = 0;
        #ifdef STRICT_ORDER
            freeop->ts=aqtime++;
        #endif

            switch(freeop->type) {
                case OP_LOCOP:
                if(0) printf("init %i (%i,%i) loclop: %lu\n", host, freeop->proc, freeop->nic, (long unsigned int) freeop->size);
                break;
                case OP_SEND:
                if(0) printf("init %i (%i,%i) send to: %i, tag: %i, size: %lu\n", host, freeop->proc, freeop->nic, freeop->target, freeop->tag, (long unsigned int) freeop->size);
                break;
                case OP_RECV:
                if(0) printf("init %i (%i,%i) recvs from: %i, tag: %i, size: %lu\n", host, freeop->proc, freeop->nic, freeop->target, freeop->tag, (long unsigned int) freeop->size);
                break;
                default:
                printf("not implemented!\n");
            }
            freeop->time = ns3_time;
            aq.push(*freeop);
            //std::cout << "AQ size after push: " << aq.size() << "\n";
            }
        }

        //printf("Initial AQ Size is %d\n\n", aq.size());

        bool new_events=true;
        bool first_cycle = true;
        uint lastperc=0;
        int cycles = 0;




        while (!aq.empty() || (size_queue(rq, p) > 0) || (size_queue(uq, p) > 0) /*|| (all_sends_delivered() == false || first_cycle)*/) {
            if (cycles > 2000) {
                //printf("\nERROR: We are in some sort of loop in the main WHILE. Breaking after 100k cycles\n\n");
                break;
            }
            //printf("----------------------------    ENTERING WHILE ---------------------------- | %ld %ld -  %d %d - %d %d %d\n", aq.top().time, ns3_time, all_sends_delivered(), first_cycle, size_queue(rq, p), size_queue(uq, p), aq.size() );


            graph_node_properties temp_elem = aq.top();
            while(!aq.empty() && aq.top().time <= ns3_time) {
                if (cycles > 2000) {
                    printf("\nERROR: We are in some sort of loop in the main WHILE. Breaking after 100k cycles\n\n");
                    exit(0);
                }
                cycles++;
                

                graph_node_properties elem = aq.top();
                //printf("Entered while loop - Elem is %d to %d, size %d - Type %d - Time %lu - Size AQ is %d - Proc %d\n", elem.host, elem.target, elem.size, elem.type, (ulint)elem.time, aq.size(), elem.proc);
                aq.pop();
                //std::cout << "AQ size after pop: " << aq.size() << "\n";
                
                // the lists of hosts that actually finished someting -- a host is
                // added whenever an irequires or requires is satisfied
                std::vector<int> check_hosts;

                /*if(elem.host == 0) print = 1;
                else print = 0;*/

                // the BIG switch on element type that we just found 
                switch(elem.type) {
                case OP_LOCOP: {
                    if(print) printf("[%i] found loclop of length %lu - t: %lu (CPU: %i)\n", elem.host, (ulint)elem.size, (ulint)elem.time, elem.proc);
                    if(nexto[elem.host][elem.proc] <= elem.time) {
                    // check if OS Noise occurred
                    btime_t noise = osnoise.get_noise(elem.host, elem.time, elem.time+elem.size);
                    nexto[elem.host][elem.proc] = elem.size + ns3_time;
                    //printf("====================== Updated time is %ld ===============================1\n",  nexto[elem.host][elem.proc]);
                    // satisfy irequires
                    //parser.schedules[elem.host].issue_node(elem.node);
                    // satisfy requires+irequires
                    //parser.schedules[elem.host].remove_node(elem.node);
                    parser.schedules[elem.host].MarkNodeAsStarted(elem.offset);
                            //parser.schedules[elem.host].MarkNodeAsDone(elem.offset);
                    elem.type = OP_LOCOP_IN_PROGRESS;
                            //check_hosts.push_back(elem.host);
                    // add to timeline
                    //tlviz.add_loclop(elem.host, elem.time, elem.time+elem.size, elem.proc);
                    } else {
                    if(print) printf("-- locop local o not available -- reinserting\n");
                    elem.time=nexto[elem.host][elem.proc];
                    
                    } 
                    aq.push(elem);
                } break;

                case OP_LOCOP_IN_PROGRESS:  {
                    parser.schedules[elem.host].MarkNodeAsDone(elem.offset);
                } break;

                case OP_SEND: { // a send op
                                                    
                    if(0) printf("[%i] found send to %i - t: %lu (CPU: %i)\n", elem.host, elem.target, (ulint)elem.time, elem.proc);
                    //printf("Avail is %ld %ld\n", nexto[elem.host][elem.proc], nextgs[elem.host][elem.nic]);
                    if(std::max(nexto[elem.host][elem.proc],nextgs[elem.host][elem.nic]) <= elem.time) { // local o,g available!
                    if(print) printf("-- satisfy local irequires\n");
                    parser.schedules[elem.host].MarkNodeAsStarted(elem.offset);
                            //check_hosts.push_back(elem.host);
                    
                    // check if OS Noise occurred
                            //btime_t noise = osnoise.get_noise(elem.host, elem.time, elem.time+o);
                            //nexto[elem.host][elem.proc] = elem.time + o + (elem.size-1)*O+ noise;
                            //nextgs[elem.host][elem.nic] = elem.time + g +(elem.size-1)*G; // TODO: G should be charged in network layer only
                            //tlviz.add_osend(elem.host, elem.time, elem.time+o+ (elem.size-1)*O, elem.proc);
                            //tlviz.add_noise(elem.host, elem.time+o+ (elem.size-1)*O, elem.time + o + (elem.size-1)*O+ noise, elem.proc);

                    // insert packet into network layer
                            //net.insert(elem.time, elem.host, elem.target, elem.size, &elem.handle);
                    //printf("==================== NIC1 is %d %d %d\n", elem.nic, elem.proc, elem.offset); fflush(stdout);
                    ns3_schedule(elem.host, elem.target, elem.size, elem.tag, elem.starttime, elem.offset);
                    //printf("==================== NIC is %d %d %d\n", elem.nic, elem.proc, elem.offset);
                    parser.schedules[elem.host].MarkNodeAsDone(elem.offset);
                            //elem.type = OP_MSG;
                            //int h = elem.host;
                            //elem.host = elem.target;
                            //elem.target = h;
                            //elem.starttime = elem.time;
                            //elem.time = elem.time+o+L; // arrival time of first byte only (G will be charged at receiver ... +(elem.size-1)*G; // arrival time
        /* #ifdef STRICT_ORDER
                    num_events++; // MSG is a new event
                    elem.ts = aqtime++; // new element in queue 
            #endif
                    if(print) printf("-- [%i] send inserting msg to %i, t: %lu\n", h, elem.host, (ulint)elem.time);
                    aq.push(elem);

                    if(elem.size <= S) { // eager message
                        if(print) printf("-- [%i] eager -- satisfy local requires at t: %lu\n", h, (long unsigned int) elem.starttime+o);
                        // satisfy local requires (host and target swapped!) 
                        parser.schedules[h].MarkNodeAsDone(elem.offset);
                        //tlviz.add_transmission(elem.target, elem.host, elem.starttime+o, elem.time-o, elem.size, G);
                    } else {
                        if(print) printf("-- [%i] start rendezvous message to: %i (offset: %i)\n", h, elem.host, elem.offset);
                    }

                    } else { // local o,g unavailable - retry later
                    if(print) printf("-- send local o,g not available -- reinserting\n");
                    elem.time = std::max(nexto[elem.host][elem.proc],nextgs[elem.host][elem.nic]);
                    aq.push(elem);*/
                    }
                                                    
                    } break;
                case OP_RECV: {
                    if(0) printf("[%i] found recv from %i - t: %lu (CPU: %i)\n", elem.host, elem.target, (ulint)elem.time, elem.proc);

                    parser.schedules[elem.host].MarkNodeAsStarted(elem.offset);
                    check_hosts.push_back(elem.host);
                    if(print) printf("-- satisfy local irequires\n");
                    
                    ruqelem_t matched_elem; 
                    // NUMBER of elements that were searched during message matching
                    int32_t match_attempts;

                    match_attempts = match(elem, &uq[elem.host], &matched_elem);
                    if(match_attempts >= 0)  { // found it in local UQ 
                    if(print) printf("-- found in local UQ\n");
                    // satisfy local requires
                    parser.schedules[elem.host].MarkNodeAsDone(elem.offset);
                    if(print) printf("-- satisfy local requires\n");
                    } else { // not found in local UQ - add to RQ
        
                    if(print) printf("-- not found in local UQ -- add to RQ\n");
                    ruqelem_t nelem; 
                    nelem.size = elem.size;
                    nelem.src = elem.target;
                    nelem.tag = elem.tag;
                    nelem.offset = elem.offset;
            #ifdef LIST_MATCH
                    //printf("############ Host added is %d\n\n", elem.host);
                    rq[elem.host].push_back(nelem);
            #else
                    rq[elem.host][std::make_pair(nelem.tag,nelem.src)].push(nelem);
            #endif
                    }
                    } break;

                case OP_MSG: {
                    if(0) printf("[%i] found msg from %i, t: %lu (CPU: %i) - %d %d %d\n", elem.host, elem.target, (ulint)elem.time, elem.proc, elem.nic, elem.proc, elem.offset);
                    uint64_t earliestfinish;
                    // NUMBER of elements that were searched during message matching
                    int32_t match_attempts;
                    //printf("nexto[elem.host][elem.proc] %ld (%d %d) - nextgr[elem.host][elem.nic] %ld (%d %d) - time %ld\n",nexto[elem.host][elem.proc], elem.host, elem.proc, nextgr[elem.host][elem.nic],elem.host, elem.nic,elem.time);
                    if(std::max(nexto[elem.host][elem.proc],nextgr[elem.host][elem.nic]) <= elem.time /* local o,g available! */) { 
                    //if (1) {
                        //printf("Reaching here %d %d - %d %d\n", nexto.size(), nextgr.size(), nexto[0].size(), nextgr[0].size()); fflush(stdout);
                        //if(0) printf("-- msg o,g available (nexto: %lu, nextgr: %lu)\n", (long unsigned int) nexto[elem.host][elem.proc], (long unsigned int) nextgr[elem.host][elem.nic]);
                        // check if OS Noise occurred
                        //btime_t noise = osnoise.get_noise(elem.host, elem.time, elem.time+o);
                        nexto[elem.host][elem.proc] = elem.time+0; /* message is only received after G is charged !! TODO: consuming o seems a bit odd in the LogGP model but well in practice */;
                        nextgr[elem.host][elem.nic] = elem.time+0;
                        //printf("====================== Updated time is %ld ===============================2\n",  nexto[elem.host][elem.proc]);

                        //nexto[elem.host][elem.proc] = elem.time+o+0+std::max((elem.size-1)*O,(elem.size-1)*G) /* message is only received after G is charged !! TODO: consuming o seems a bit odd in the LogGP model but well in practice */;
                        //nextgr[elem.host][elem.nic] = elem.time+g+(elem.size-1)*G;
                        //printf("Reaching here\n"); fflush(stdout);
                
                        ruqelem_t matched_elem; 
                        match_attempts = match(elem, &rq[elem.host], &matched_elem);
                        if(match_attempts >= 0) { // found it in RQ
                            if(0) {
                            // RECORD match queue statistics
                            std::pair<int,btime_t> match = std::make_pair(match_attempts, elem.time);
                            rq_matches[elem.host].push_back(match);
                            /* Amount of time spent in queue */
                            rq_times[elem.host].push_back(elem.time - matched_elem.starttime);
                            }

                            if(0) printf("-- Found in RQ\n");
                            parser.schedules[elem.host].MarkNodeAsDone(matched_elem.offset);
                            //check_hosts.push_back(elem.host);
                            //printf("Reached after DONE \n"); fflush(stdout);


                        } else { // not in RQ

                            if(0) printf("-- not found in RQ - add to UQ\n");
                            ruqelem_t nelem;
                            nelem.size = elem.size;
                            nelem.src = elem.target;
                            nelem.tag = elem.tag;
                            nelem.offset = elem.offset;
                            nelem.starttime = elem.time; // when it was started
                #ifdef LIST_MATCH
                            uq[elem.host].push_back(nelem);
                #else
                            uq[elem.host][std::make_pair(nelem.tag,nelem.src)].push(nelem);
                #endif
                        }
                    } else {
                    elem.time=std::max(std::max(nexto[elem.host][elem.proc],nextgr[elem.host][elem.nic]), earliestfinish);
                    if(0) printf("-- msg o,g not available -- reinserting\n");
                    aq.push(elem);
                    }
                    
                } break;
                    
                default:
                    printf("not supported\n");
                    break;
                }
                
                // do only ask hosts that actually completed something in this round!
                //        new_events=false;
                //        std::sort(check_hosts.begin(), check_hosts.end());
                //        check_hosts.erase(unique(check_hosts.begin(), check_hosts.end()), check_hosts.end());
                
                
            }

            // How much ime inside NS3
            auto start_ns3 = high_resolution_clock::now();

            // Need support for RECV
            graph_node_properties recev_msg;
            recev_msg.updated = false;
            // We Run NS-3 if we have a compute message or if we have still some data in the network
            //printf("Current elem time is %ld while NS3 %ld\n", temp_elem.time, ns3_time); fflush(stdout);
            if (!all_sends_delivered()) {
                if (temp_elem.time > ns3_time) {
                    printf("Running until\n");
                    ns3_time = ns3_simulate_until(temp_elem.time, &recev_msg);
                } else {
                    printf("Running infinite\n");
                    ns3_time = ns3_simulate_until(std::numeric_limits<int64_t>::max() - Simulator::Now().GetNanoSeconds() - 1, &recev_msg);
                }
            }
            // How much time inside NS3
            auto t2 = high_resolution_clock::now();
            /* Getting number of milliseconds as an integer. */
            auto ms_int = duration_cast<milliseconds>(t2 - start_ns3);
            global_time += ms_int;
            //std::cout << "\n\nRunTime Run LGS -> " << ms_int.count() << "ms\n";
            //std::cout << "\n\nRunTime Total Partial LGS -> " << global_time.count() << "ms\n";

            // If the OP is NULL then we just continue
            if (recev_msg.updated) {
                //printf("..... Received a MSG -- %d to %d, proc %d\n", recev_msg.host, recev_msg.target, recev_msg.proc);
                aq.push(recev_msg);
            } else {
            // If not NULL, we add it to the AQ
                //printf("..... NOT Received a MSG\n");
            }

            host = 0;
            for(Parser::schedules_t::iterator sched=parser.schedules.begin(); sched!=parser.schedules.end(); ++sched, ++host) {
                //printf("Starting to parse new ops %d\n", 1); fflush(stdout);
            //host = *iter;
            //for(host = 0; host < p; host++) 
            //SerializedGraph *sched=&parser.schedules[host];

            // retrieve all free operations
            SerializedGraph::nodelist_t free_ops;
            sched->GetExecutableNodes(&free_ops);
            // ensure that the free ops are ordered by type
            std::sort(free_ops.begin(), free_ops.end(), gnp_op_comp_func());

            //printf("Free Op Size %d\n", free_ops.size());
            
            // walk all new free operations and throw them in the queue 
            for(SerializedGraph::nodelist_t::iterator freeop=free_ops.begin(); freeop != free_ops.end(); ++freeop) {
                //if(print) std::cout << *freeop << " " ;
                //new_events = true;

                // assign host that it starts on
                freeop->host = host;

        #ifdef STRICT_ORDER
                freeop->ts=aqtime++;
        #endif
                //printf("We arrive here %d\n", freeop->type); fflush(stdout);
                switch(freeop->type) {
                case OP_LOCOP:
                    freeop->time = nexto[host][freeop->proc];
                    if(print) printf("%i (%i,%i) loclop: %lu, time: %lu, offset: %i\n", host, freeop->proc, freeop->nic, (long unsigned int) freeop->size, (long unsigned int)freeop->time, freeop->offset);
                    break;
                case OP_SEND:
                    freeop->time = ns3_time;
                    //freeop->time = std::max(nexto[host][freeop->proc], nextgs[host][freeop->nic]);
                    if(0) printf("%i (%i,%i) send to: %i, tag: %i, size: %lu, time: %lu, offset: %i\n", host, freeop->proc, freeop->nic, freeop->target, freeop->tag, (long unsigned int) freeop->size, (long unsigned int)freeop->time, freeop->offset);
                    break;
                case OP_RECV:
                    freeop->time = nexto[host][freeop->proc];
                    if(print) printf("%i (%i,%i) recvs from: %i, tag: %i, size: %lu, time: %lu, offset: %i\n", host, freeop->proc, freeop->nic, freeop->target, freeop->tag, (long unsigned int) freeop->size, (long unsigned int)freeop->time, freeop->offset);
                    break;
                default:
                    printf("not implemented!\n");
                }
                freeop->time = ns3_time;
                aq.push(*freeop);
            }
            }

            first_cycle = false;
            cycles++;
        }
        std::cout << "\n\nRunTime Total LGS -> " << global_time.count() << "ms\n";
        ns3_terminate(ns3_time);
        printf("\n\n\nNS3 Terminates at %ld\n\n\n", ns3_time);
            
        gettimeofday(&tend, NULL);
            unsigned long int diff = tend.tv_sec - tstart.tv_sec;

        #ifndef STRICT_ORDER
        ulint aqtime=0;
        #endif
            printf("PERFORMANCE: Processes: %i \t Events: %lu \t Time: %lu s \t Speed: %.2f ev/s\n", p, (long unsigned int)aqtime, (long unsigned int)diff, (float)aqtime/(float)diff);
        printf("AQ is %d\n",aq.size());
        // check if all queues are empty!!
        bool ok=true;
        for(uint i=0; i<p; ++i) {
            
        #ifdef LIST_MATCH
            if(!uq[i].empty()) {
            printf("unexpected queue on host %i contains %lu elements!\n", i, (ulint)uq[i].size());
            for(ruq_t::iterator iter=uq[i].begin(); iter != uq[i].end(); ++iter) {
                printf(" src: %i, tag: %i\n", iter->src, iter->tag);
            }
            ok=false;
            }
            if(!rq[i].empty()) {
            printf("receive queue on host %i contains %lu elements!\n", i, (ulint)rq[i].size());
            for(ruq_t::iterator iter=rq[i].begin(); iter != rq[i].end(); ++iter) {
                printf(" src: %i, tag: %i\n", iter->src, iter->tag);
            }
            ok=false;
            }
        #endif

        }
        
        if (ok) {
            if(p <= 16 && !0) { // print all hosts
            printf("Times: \n");
            host = 0;
            for(uint i=0; i<p; ++i) {
                btime_t maxo=*(std::max_element(nexto[i].begin(), nexto[i].end()));
                //btime_t maxgr=*(std::max_element(nextgr[i].begin(), nextgr[i].end()));
                //btime_t maxgs=*(std::max_element(nextgs[i].begin(), nextgs[i].end()));
                //std::cout << "Host " << i <<": "<< std::max(std::max(maxgr,maxgs),maxo) << "\n";
                std::cout << "Host " << i <<": "<< maxo << "\n";
            }
            } else { // print only maximum
            long long unsigned int max=0;
            int host=0;
            for(uint i=0; i<p; ++i) { // find maximum end time
                btime_t maxo=*(std::max_element(nexto[i].begin(), nexto[i].end()));
                //btime_t maxgr=*(std::max_element(nextgr[i].begin(), nextgr[i].end()));
                //btime_t maxgs=*(std::max_element(nextgs[i].begin(), nextgs[i].end()));
                //btime_t cur = std::max(std::max(maxgr,maxgs),maxo);
                btime_t cur = maxo;
                if(cur > max) {
                host=i;
                max=cur;
                }
            }
            std::cout << "Maximum finishing time at host " << host << ": " << max << " ("<<(double)max/1e9<< " s)\n";
            }

            // WRITE match queue statistics
            if( 0 ){
            char filename[1024];

            // Maximum RQ depth
            snprintf(filename, sizeof filename, "%d-%s",0 , "rq-max.data");
            std::ofstream rq_max_file(filename);

            if( !rq_max_file.is_open() ) {
                std::cerr << "Can't open rq-max data file" << std::endl;
            } else {
                // WRITE one line per rank
                for( int n : rq_max ) {
                rq_max_file << n << std::endl;
                }
            }
            rq_max_file.close();

            // Maximum UQ depth
            snprintf(filename, sizeof filename, "%d-%s", 0, "uq-max.data");
            std::ofstream uq_max_file(filename);

            if( !uq_max_file.is_open() ) {
                std::cerr << "Can't open uq-max data file" << std::endl;
            } else {
                // WRITE one line per rank
                for( int n : uq_max ) {
                uq_max_file << n << std::endl;
                }
            }
            uq_max_file.close();

            // RQ hit depth (number of elements searched for each successful search)
            snprintf(filename, sizeof filename, "%d-%s", 0, "rq-hit.data");
            std::ofstream rq_hit_file(filename);

            if( !rq_hit_file.is_open() ) {
                std::cerr << "Can't open rq-hit data file (" << filename << ")" << std::endl;
            } else {
                // WRITE one line per rank
                for( auto per_rank_matches = rq_matches.begin(); 
                        per_rank_matches != rq_matches.end();  
                        per_rank_matches++ ) 
                {
                for( auto match_pair = (*per_rank_matches).begin(); 
                            match_pair != (*per_rank_matches).end(); 
                            match_pair++ ) 
                {
                    rq_hit_file << (*match_pair).first << "," << (*match_pair).second << " ";
                }
                rq_hit_file << std::endl;
                }
            }
            rq_hit_file.close();

            // UQ hit depth (number of elements searched for each successful search)
            snprintf(filename, sizeof filename, "%d-%s", 0, "uq-hit.data");
            std::ofstream uq_hit_file(filename);

            if( !uq_hit_file.is_open() ) {
                std::cerr << "Can't open uq-hit data file (" << filename << ")" << std::endl;
            } else {
                // WRITE one line per rank
                for( auto per_rank_matches = uq_matches.begin(); 
                        per_rank_matches != uq_matches.end();  
                        per_rank_matches++ ) 
                {
                for( auto match_pair = (*per_rank_matches).begin(); 
                            match_pair != (*per_rank_matches).end(); 
                            match_pair++ ) 
                {
                    uq_hit_file << (*match_pair).first << "," << (*match_pair).second << " ";
                }
                uq_hit_file << std::endl;
                }
            }
            uq_hit_file.close();

            // RQ miss depth (number of elements searched for each unsuccessful search)
            snprintf(filename, sizeof filename, "%d-%s", 0, "rq-miss.data");
            std::ofstream rq_miss_file(filename);

            if( !rq_miss_file.is_open() ) {
                std::cerr << "Can't open rq-miss data file (" << filename << ")" << std::endl;
            } else {
                // WRITE one line per rank
                for( auto per_rank_misses = rq_misses.begin(); 
                        per_rank_misses != rq_misses.end();  
                        per_rank_misses++ ) 
                {
                for( auto miss_pair = (*per_rank_misses).begin(); 
                            miss_pair != (*per_rank_misses).end(); 
                            miss_pair++ ) 
                {
                    rq_miss_file << (*miss_pair).first << "," << (*miss_pair).second << " ";
                }
                rq_miss_file << std::endl;
                }
            }
            rq_miss_file.close();

            // UQ miss depth (number of elements searched for each unsuccessful search)
            snprintf(filename, sizeof filename, "%d-%s", 0, "uq-miss.data");
            std::ofstream uq_miss_file(filename);

            if( !uq_miss_file.is_open() ) {
                std::cerr << "Can't open uq-miss data file (" << filename << ")" << std::endl;
            } else {
                // WRITE one line per rank
                for( auto per_rank_misses = uq_misses.begin(); 
                        per_rank_misses != uq_misses.end();  
                        per_rank_misses++ ) 
                {
                for( auto miss_pair = (*per_rank_misses).begin(); 
                            miss_pair != (*per_rank_misses).end(); 
                            miss_pair++ ) 
                {
                    uq_miss_file << (*miss_pair).first << "," << (*miss_pair).second << " ";
                }
                uq_miss_file << std::endl;
                }
            }
            uq_miss_file.close();
            }
        }
        return 0;
    }



}

