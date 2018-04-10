
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unordered_map>

#include "ns3/flow-monitor-module.h"
#include "ns3/bridge-helper.h"
#include "ns3/bridge-net-device.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/csma-module.h"
#include "ns3/ipv4-nix-vector-helper.h"


/*
- The code is constructed in the following order:
1. Creation of Node Containers
2. Initialize settings for UdpEcho Client/Server Application
3. Connect hosts to edge switches
4. Connect edge switches to aggregate switches
5. Connect aggregate switches to core switches
6. Start Simulation

- Addressing scheme:
1. Address of host: 10.pod.edge.0 /24
2. Address of edge and aggregation switch: 10.pod.(agg + pod/2).0 /24
3. Address of core switch: 10.(group + pod).core.0 /24
(Note: there are k/2 group of core switch)

- Application Setting:
- PacketSize: (Send packet data size (byte))
- Interval: (Send packet interval time (s))
- MaxPackets: (Send packet max number)

- Channel Setting:
- DataRate: (Gbps)
- Delay: (ms)

- Simulation Settings:
- Number of pods (k): 4-24 (run the simulation with varying values of k)
- Number of hosts: 16-3456
- UdpEchoClient/Server Communication Pairs:
[k] client connect with [hostNum-k-1] server (k from 0 to halfHostNum-1)
- Simulation running time: 100 seconds
- Traffic flow pattern:
- Routing protocol: Nix-Vector
*/


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("P4Example");

unsigned long getTickCount(void)
{
	unsigned long currentTime = 0;
#ifdef WIN32
	currentTime = GetTickCount();
#endif
	struct timeval current;
	gettimeofday(&current, NULL);
	currentTime = current.tv_sec * 1000 + current.tv_usec / 1000;
#ifdef OS_VXWORKS
	ULONGA timeSecond = tickGet() / sysClkRateGet();
	ULONGA timeMilsec = tickGet() % sysClkRateGet() * 1000 / sysClkRateGet();
	currentTime = timeSecond * 1000 + timeMilsec;
#endif
	return currentTime;
}

// Function to create address string from numbers
//
char * toString(int a, int b, int c, int d) {

	int first = a;
	int second = b;
	int third = c;
	int fourth = d;

	char *address = new char[30];
	char firstOctet[30], secondOctet[30], thirdOctet[30], fourthOctet[30];

	bzero(address, 30);

	snprintf(firstOctet, 10, "%d", first);
	strcat(firstOctet, ".");
	snprintf(secondOctet, 10, "%d", second);
	strcat(secondOctet, ".");
	snprintf(thirdOctet, 10, "%d", third);
	strcat(thirdOctet, ".");
	snprintf(fourthOctet, 10, "%d", fourth);

	strcat(thirdOctet, fourthOctet);
	strcat(secondOctet, thirdOctet);
	strcat(firstOctet, secondOctet);
	strcat(address, firstOctet);

	return address;
}

//typedef std::unordered_map<uint64_t, uint32_t>::iterator TupleIter_t;

//Output switch received packet info
//
void ShowSwitchInfos(Ptr<Node> s)
{
	std::cout << "Receive Packet Sum: " << s->m_packetNum << std::endl;
	/*std::cout << "Receive Tuple Packet Num (TupleHash Num):" << std::endl;
	for (TupleIter_t iter = s->m_tupleNum.begin(); iter != s->m_tupleNum.end(); iter++)
	{
		std::cout << iter->first << " " << iter->second << std::endl;
	}*/
}

int
main(int argc, char *argv[])
{
	unsigned long start = getTickCount();
	LogComponentEnable("P4Example", LOG_LEVEL_LOGIC);
	//LogComponentEnable("BridgeNetDevice", LOG_LEVEL_LOGIC);
	//LogComponentEnable("PointToPointNetDevice", LOG_LEVEL_LOGIC);
	//LogComponentEnable("CsmaNetDevice",LOG_LEVEL_LOGIC);

	int pod = 4;
	// Initialize parameters for UdpEcho Client/Server application
	//
	int port = 9;
	unsigned int packetSize = 20;		// send packet data size (byte)
	double interval = 100; // send packet interval time (ms)
	unsigned maxPackets = 100; // send packet max number


	// Initialize parameters for Csma and PointToPoint protocol
	//
	char dataRate[] = "40Gbps";	// 40Gbps
	double delay = 0.001;		// 0.001 (ms)

	// Initalize parameters for UdpEcho Client/Server Appilication 
	//
	int clientStartTime = 1; // UdpEchoClient Start Time (s)
	int clientStopTime = 11;
	int serverStartTime = 0; // UdpEchoServer Start Time (s)
	int serverStopTime = 12;

	CommandLine cmd;
	cmd.AddValue("podnum", "Numbers of pod [default 4]", pod);
	cmd.AddValue("inter", "UdpEchoClient send packet interval time/s [default 100ms]", interval);
	cmd.AddValue("delay", " Csma channel delay time/ms [default 0.001ms]", delay);
	cmd.AddValue("cs", " Client application start time/s [default 1s]", clientStartTime);
	cmd.AddValue("ce", " Client application stop time/s [default 11s]", clientStopTime);
	cmd.AddValue("ss", " Server application start time/s [default 0s]", serverStartTime);
	cmd.AddValue("se", " Server application stop time/s [default 12s]", serverStopTime);
	cmd.Parse(argc, argv);

	maxPackets = (clientStopTime - clientStartTime) / interval * 1000;

	std::cout << "------------Network Parameters Setting-----------------" << std::endl;
	std::cout << "NS3 Simulate Mode:" << std::endl;
	std::cout << "Pod Num: " << pod << std::endl;
	std::cout << "Simulate Time: " << clientStopTime - clientStartTime << " s" << std::endl;
	std::cout << "Csma Channel DataRate: " << dataRate << std::endl;
	std::cout << "Csma Channel Delay: " << delay << " ms" << std::endl;
	std::cout << "Send Packet Interval: " << interval << " ms" << std::endl;
	std::cout << "Send Packet MaxNum: " << maxPackets << std::endl;
	std::cout << "Send Packet Data Size: " << packetSize << " byte" << std::endl;


	//=========== Define parameters based on value of k ===========//
	//
	int k = pod;			// number of ports per switch
	int num_pod = k;		// number of pod
	int num_host = (k / 2);		// number of hosts under a switch
	int num_edge = (k / 2);		// number of edge switch in a pod
	int num_bridge = num_edge;	// number of bridge in a pod
	int num_agg = (k / 2);		// number of aggregation switch in a pod
	int num_group = k / 2;		// number of group of core switches
	int num_core = (k / 2);		// number of core switch in a group
	int total_host = k*k*k / 4;	// number of hosts in the entire network	

								// Initialize other variables
								//
	int i = 0;
	int j = 0;
	int h = 0;

	// Output some useful information
	//	
	std::cout << "Value of k =  " << k << "\n";
	std::cout << "Total number of hosts =  " << total_host << "\n";
	std::cout << "Number of hosts under each switch =  " << num_host << "\n";
	std::cout << "Number of edge switch under each pod =  " << num_edge << "\n";
	std::cout << "------------- " << "\n";

	// Initialize Internet Stack and Routing Protocols
	//	
	InternetStackHelper internet;
	Ipv4NixVectorHelper nixRouting;
	Ipv4StaticRoutingHelper staticRouting;
	Ipv4ListRoutingHelper list;
	list.Add(staticRouting, 0);
	list.Add(nixRouting, 10);
	internet.SetRoutingHelper(list);

	//=========== Creation of Node Containers ===========//
	//
	NodeContainer core[num_group];				// NodeContainer for core switches
	for (i = 0; i<num_group; i++) {
		core[i].Create(num_core);
		internet.Install(core[i]);
	}
	NodeContainer agg[num_pod];				// NodeContainer for aggregation switches
	for (i = 0; i<num_pod; i++) {
		agg[i].Create(num_agg);
		internet.Install(agg[i]);
	}
	NodeContainer edge[num_pod];				// NodeContainer for edge switches
	for (i = 0; i<num_pod; i++) {
		edge[i].Create(num_edge);
		internet.Install(edge[i]);
	}
	NodeContainer bridge[num_pod];				// NodeContainer for edge bridges
	for (i = 0; i<num_pod; i++) {
		bridge[i].Create(num_bridge);
		internet.Install(bridge[i]);
	}
	NodeContainer host[num_pod][num_bridge];		// NodeContainer for hosts
	for (i = 0; i<k; i++) {
		for (j = 0; j<num_bridge; j++) {
			host[i][j].Create(num_host);
			internet.Install(host[i][j]);
		}
	}

	std::cout << "Finished creating Node Containers" << "\n";

	//=========== Initialize settings for UdpEcho Client/Server Application ===========//
	//

	// Generate traffics for the simulation
	//	

	Config::SetDefault("ns3::Ipv4RawSocketImpl::Protocol", StringValue("2"));

	ApplicationContainer app[total_host];

	char *addr;
	int half_host_num = total_host / 2;
	//half_host_num = 1;
	for (int client_i = 0; client_i < half_host_num; client_i++)
	{
		int server_i = total_host - client_i - 1;

		int s_p, s_q, s_t;
		s_p = server_i / (num_edge*num_host);
		s_q = (server_i - s_p*num_edge*num_host) / num_host;
		s_t = server_i%num_host;

		int p, q, t;
		p = client_i / (num_edge*num_host);
		q = (client_i - p*num_edge*num_host) / num_host;
		t = client_i%num_host;

		//specify dst ip (server ip)
		addr = toString(10, s_p, s_q, s_t + 2);
		Ipv4Address dstIp = Ipv4Address(addr);

		//install server app
		UdpEchoServerHelper echoServer(port);
		ApplicationContainer serverApp = echoServer.Install(host[s_p][s_q].Get(s_t));
		serverApp.Start(Seconds(serverStartTime));
		serverApp.Stop(Seconds(serverStopTime));

		//install client app
		UdpEchoClientHelper echoClient(dstIp, port);
		echoClient.SetAttribute("MaxPackets", UintegerValue(maxPackets));
		echoClient.SetAttribute("Interval", TimeValue(MilliSeconds(interval)));
		echoClient.SetAttribute("PacketSize", UintegerValue(packetSize));
		app[client_i] = echoClient.Install(host[p][q].Get(t));
	}

	std::cout << "Finished creating UdpEchoClient/UdpEchoServer traffic" << "\n";


	// Inintialize Address Helper
	//	
	Ipv4AddressHelper address;

	// Initialize PointtoPoint helper
	//	
	PointToPointHelper p2p;
	p2p.SetDeviceAttribute("DataRate", StringValue(dataRate));
	p2p.SetChannelAttribute("Delay", TimeValue(MilliSeconds(delay)));

	// Initialize Csma helper
	//
	CsmaHelper csma;
	csma.SetChannelAttribute("DataRate", StringValue(dataRate));
	csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(delay)));

	//=========== Connect edge switches to hosts ===========//
	//	
	NetDeviceContainer hostSw[num_pod][num_bridge];
	NetDeviceContainer bridgeDevices[num_pod][num_bridge];
	Ipv4InterfaceContainer ipContainer[num_pod][num_bridge];

	for (i = 0; i<num_pod; i++) {
		for (j = 0; j<num_bridge; j++) {
			NetDeviceContainer link1 = csma.Install(NodeContainer(edge[i].Get(j), bridge[i].Get(j)));
			hostSw[i][j].Add(link1.Get(0));
			bridgeDevices[i][j].Add(link1.Get(1));

			for (h = 0; h< num_host; h++) {
				NetDeviceContainer link2 = csma.Install(NodeContainer(host[i][j].Get(h), bridge[i].Get(j)));
				hostSw[i][j].Add(link2.Get(0));
				bridgeDevices[i][j].Add(link2.Get(1));
			}

			BridgeHelper bHelper;
			bHelper.Install(bridge[i].Get(j), bridgeDevices[i][j]);
			//Assign address
			char *subnet;
			subnet = toString(10, i, j, 0);
			address.SetBase(subnet, "255.255.255.0");
			ipContainer[i][j] = address.Assign(hostSw[i][j]);
		}
	}
	std::cout << "Finished connecting edge switches and hosts  " << "\n";

	//=========== Connect aggregate switches to edge switches ===========//
	//
	NetDeviceContainer ae[num_pod][num_agg][num_edge];
	Ipv4InterfaceContainer ipAeContainer[num_pod][num_agg][num_edge];
	for (i = 0; i<num_pod; i++) {
		for (j = 0; j<num_agg; j++) {
			for (h = 0; h<num_edge; h++) {
				ae[i][j][h] = p2p.Install(agg[i].Get(j), edge[i].Get(h));

				int second_octet = i;
				int third_octet = j + (k / 2);
				int fourth_octet;
				if (h == 0) fourth_octet = 1;
				else fourth_octet = h * 2 + 1;
				//Assign subnet
				char *subnet;
				subnet = toString(10, second_octet, third_octet, 0);
				//Assign base
				char *base;
				base = toString(0, 0, 0, fourth_octet);
				address.SetBase(subnet, "255.255.255.0", base);
				ipAeContainer[i][j][h] = address.Assign(ae[i][j][h]);
			}
		}
	}
	std::cout << "Finished connecting aggregation switches and edge switches  " << "\n";

	//=========== Connect core switches to aggregate switches ===========//
	//
	NetDeviceContainer ca[num_group][num_core][num_pod];
	Ipv4InterfaceContainer ipCaContainer[num_group][num_core][num_pod];
	int fourth_octet = 1;

	for (i = 0; i<num_group; i++) {
		for (j = 0; j < num_core; j++) {
			fourth_octet = 1;
			for (h = 0; h < num_pod; h++) {
				ca[i][j][h] = p2p.Install(core[i].Get(j), agg[h].Get(i));

				int second_octet = k + i;
				int third_octet = j;
				//Assign subnet
				char *subnet;
				subnet = toString(10, second_octet, third_octet, 0);
				//Assign base
				char *base;
				base = toString(0, 0, 0, fourth_octet);
				address.SetBase(subnet, "255.255.255.0", base);
				ipCaContainer[i][j][h] = address.Assign(ca[i][j][h]);
				fourth_octet += 2;
			}
		}
	}
	std::cout << "Finished connecting core switches and aggregation switches  " << "\n";
	std::cout << "------------- " << "\n";


	//=========== Start the simulation ===========//
	//

	std::cout << "Start Simulation.. " << "\n";

	for (i = 0; i<total_host; i++) {
		app[i].Start(Seconds(clientStartTime));
		app[i].Stop(Seconds(clientStopTime));
	}
	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	// Calculate Throughput using Flowmonitor
	//
	//  FlowMonitorHelper flowmon;
	//  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

	// Run simulation.
	//
	unsigned long simulate_start = getTickCount();
	NS_LOG_INFO("Run Simulation.");
	Simulator::Stop(Seconds(serverStopTime + 1));
	Packet::EnablePrinting();

	Simulator::Run();

	//  monitor->CheckForLostPackets ();
	//	monitor->SerializeToXmlFile(filename, true, true);

	std::cout << "Simulation finished " << "\n";

	//=========== Show  Switch Received Packet Num ===========//
	//

	std::cout << "=========== Show  Switch Received Packet Num ===========" << std::endl;
	// Show Core Switch 
	for (i = 0; i < num_group; i++)
	{
		for (j = 0; j < num_core; j++)
		{
			std::cout << "Core Switch [group,core] [" << i << "," << j << "]:";
			ShowSwitchInfos(core[i].Get(j));
		}
	}

	// Show Agg Switch
	for (i = 0; i < num_pod; i++)
	{
		for (j = 0; j < num_agg; j++)
		{
			std::cout << "Agg Switch [pod,agg] [" << i << "," << j << "]:";
			ShowSwitchInfos(agg[i].Get(j));
		}
	}

	// Show Edge Switch
	for (i = 0; i < num_pod; i++)
	{
		for (j = 0; j < num_edge; j++)
		{
			std::cout << "Edge Switch [pod,edge] [" << i << "," << j << "]:";
			ShowSwitchInfos(edge[i].Get(j));
		}
	}

	std::cout << "==========================================================" << std::endl;

	//=========== Show Host Received Packet Num ===========//
	//
	std::cout << "============= Show Host Received Packet Num ==============" << std::endl;
	for (i = 0; i < num_pod; i++)
	{
		for (j = 0; j < num_edge; j++)
		{
			for (k = 0; k < num_host; k++)
			{
				std::cout << "Host [pod,edge,host] [" << i << "," << j << "," << k << "]:";
				ShowSwitchInfos(host[i][j].Get(k));
			}
		}
	}
	std::cout << "==========================================================" << std::endl;

	Simulator::Destroy();
	NS_LOG_INFO("Done.");
	unsigned long end = getTickCount();
	std::cout << "Simulate Running time: " << end - simulate_start << "ms" << std::endl;
	std::cout << "Running time: " << end - start << "ms" << std::endl;
	return 0;
}




