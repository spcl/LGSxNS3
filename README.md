
# LGSxNS3

This work proposes a feeding engine for NS-3 in order to easily enable complex workloads with dependencies. The main motivation behind it is the growing importance of simulating complex deep learning workloads where the communication patterns are complex because of the dependencies between the operations.

Since NS-3 does not provide any support for complex workloads, we propose to use LogGOPSim (LGS) [1] as a feeding engine to NS-3. A custom version of LGS will take care of the dependencies and everything related to the workloads while NS-3 will run as network backend.

The implentation is lightway (adds <5% of extra overhead on average) and easily customizable with any protocol and/or topology. 

Note that this is an early release and there might be some problems or unexplored edge cases. In that case feel free to contact me directly via [e-mail](mailto:tommaso.bonato@inf.ethz.ch) or with an issue here.

## Components 
We have 3 main components in this architecture:

- LogGOPSim (LGS) which is a simulator for parallel applications and algorithms that uses the LogGOPS (or LogP, LogGP, or LogGPS) network model to simulate the execution of parallel algorithms and full applications. 

- Schedgen which is used to generate the GOAL input language that is then parsed by LGS.

- NS-3 which is the network backend but also the entry point of the application as we implement LGS as a module of NS-3.

## How to use
The current code already allows users to run any kind of GOAL workload using some simple topologies and protocols. Running a workload of any kind of complexity requires the user to only change the name of the GOAL input file to use.

### GOAL input language
The GOAL language was first proposed here [2] and it is basically a language consisting of 4 primitives representing different network operations: Send, Recv, Depends and Compute.  The following is a very basic example of a GOAL file witth two ranks and some basic operations.
```
num_ranks 2

rank 0 {
l1: send 2000b to 1 tag 0
l2: recv 3000b from 1 tag 0
l3: send 4000b to 1 tag 0
l3 requires l2
l4: recv 5000b from 1 tag 0
}

rank 1 {
l1: recv 2000b from 0 tag 0
l2: send 3000b to 0 tag 0
l2 requires l1
l3: recv 4000b from 0 tag 0
l4: send 5000b to 0 tag 0
l4 requires l3
}
```


The GOAL input file can be generated in Schedgen which supports already a number of MPI collectives and other network traffic patterns. Another option is to trace a MPI application and then use the traces to generate the GOAL input file. The Schegen folder has its own readME with more details.


### Running a Simulation

Once the repository has been downloaded and installed like a standard NS-3 instance (please check [this official guide for details](https://www.nsnam.org/wiki/Installation)), the repository provides a toy TCP or UDP application that can be run using the default run command of NS-3 such as:
```
./waf --run "scratch/ring --goal_filename='pingpong.bin' --protocol='UDP' --packet_size='512'
```
The main difference is that here we specificy a GOAL input file, a protocol and the wanted packet size.

Please note that the first time NS-3 gets downloaded and installed, it takes a decent amount of time to build and install everything (20-30 minutes on a modern machine).

### Creating a custom LGS simulation

Most people will probably be interested in creating their own application and/or protocol and use LGS as the feeding engine. In that case there are some important thigns to keep in mind to make it work. The most important and necessary things to do are the following:

#### Scratch main() differences
When creating a new application to run (inside scratch, per default NS-3 behaviour), the main function must contain the following code at the beginning of it:
```
// Start Code Reserved for LGS 
CommandLine cmd;
std::string filename_goal, protocol_name, packet_size, k;
cmd.AddValue("goal_filename", "GOAL binary input file", filename_goal);
cmd.AddValue("protocol", "TCP or UDP", protocol_name);
cmd.AddValue("packet_size", "Payload Size of a packet", packet_size);
cmd.Parse(argc, argv);
int payload_size = std::stoi(packet_size);
int k_ft = std::stoi(k);
// End Code Reserved for LGS 
```

Moreover, to actually start LGS we also need the two following line of code:
```
// Start Code Reserved for LGS 
initialize_interface_var(nodes, protocol_name, payload_size);
start_lgs(filename_goal);
// End Code Reserved for LGS 
```

Here `nodes` is simply the standard NS-3 data structure containing all the nodes of our topology and the other two variables are simply the command line options.

Finally, if the intent is to create a new or different topology, then it is necessary for each new node to be registered using these two lines of code:

```
// Start Code Reserved for LGS 
update_node_map(0, interface.GetAddress(0));
nodes.Get(0)->isNic = true;
// End Code Reserved for LGS 
```

Here we basically map the NS-3 node rank concept and address to the matching rank number of LGS. Finally we specify if the node created shall be intended as NIC (true) or switch (false).

We provide a skeleton main function code that can then be modificed for other needs.


#### Protocol and application differences
Most users will probably also want to use a custom protocol and application. To add another protocol the file `logsim-helper.cc` must be modified to add a custom case for the specific protocol inside the `send_event()` function. Theoretically any protocol should be supported but unusual implementations might require more changes to work properly.

#### Packet Metadata
We add to each packet (usually in the application layer) some metadata to it (not to be confused with the real header of the packet) to store some information needed for LGS such as the sending node ID or the tag of the message. Please refer to the `custom_tag.h` file to see how to add it and what fields are needed.

### Data Collection
By default, each simulation collects some statistics about the duration of the simulation, the individual running time of NS-3 and LGS in order to calculate its overhead. The thorughput of each message being sent in the network and the running time of each individual rank.