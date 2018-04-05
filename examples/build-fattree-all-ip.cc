#include <iostream>
#include <vector>
#include <time.h>
#include <string>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/p4-helper.h"
#include "ns3/v4ping-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/binary-tree-topo-helper.h"
#include "ns3/fattree-topo-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>

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


static void SinkRx(Ptr<const Packet> p, const Address &ad) {
	std::cout << "Rx" << "Received from  " << ad << std::endl;
}

//static void PingRtt(std::string context, Time rtt) {
//	std::cout << "Rtt" << context << " " << rtt << std::endl;
//}

// switch number:0~switchNum-1
// host number: 0~hostNum-1

struct SwitchUnit_t
{
	SwitchUnit_t()
	{}
	// switch link port network device
	NetDeviceContainer switchDevices;
	// switch link port node info
	std::vector<bool> isSwitch;
	std::vector<unsigned int> index;
	std::vector<unsigned int> portNb;
};

// Attention: host can not link with other host
struct  HostUnit_t
{
	HostUnit_t()
		:index(0), portNb(0)
	{}
	//host network device and ipv4 info
	NetDeviceContainer hostDevice;
	Ipv4InterfaceContainer hostIpv4;
	//host link switch info
	unsigned int index;
	unsigned int portNb;
};

void ShowSwitchHostInfo(const std::vector<SwitchUnit_t> &switchInfos, const std::vector<HostUnit_t> &hostInfos)
{
	// Show switch info
	std::cout << "Show switch info from 0 to " << switchInfos.size() - 1 << std::endl;
	for (size_t i = 0; i < switchInfos.size(); i++)
	{
		std::cout << "switch " << i << ":" << std::endl;
		for (size_t j = 0; j<switchInfos[i].index.size(); j++)
		{
			if (switchInfos[i].isSwitch[j])
				std::cout << "link switch index portNb:";
			else
				std::cout << "link host index portNb:";
			std::cout << switchInfos[i].index[j] << " " << switchInfos[i].portNb[j] << std::endl;
		}
	}

	// Show host info
	std::cout << "Show host info from 0 to " << hostInfos.size() - 1 << std::endl;
	for (size_t i = 0; i < hostInfos.size(); i++)
	{
		//std::cout << "ipv4 address:";
		//hostInfos[i].hostIpv4.GetAddress(0).Print(std::cout);
		std::cout << " index portNb: " << hostInfos[i].index << " " << hostInfos[i].portNb << std::endl;
	}
}

char *GetStartIpAddr(unsigned int s)
{
	std::string ip;
	if (s == 0)
		ip = "0";
	else
	{
		while (s)
		{
			ip.insert(ip.begin(), s % 10 + '0');
			s /= 10;
		}
	}
	std::string str("0.0.0.");
	str.append(ip);
	char *res = new char[str.size() + 1];
	for (size_t i = 0; i < str.size(); i++)
		res[i] = str[i];
	res[str.size()] = '\0';
	return res;
}

char* GetHostIpAddrBase(unsigned int pod, unsigned int s)
{
	std::string ipBase("10.");
	if (pod == 0)
		ipBase.append("0.");
	else
	{
		std::string podStr;
		while (pod)
		{
			podStr.insert(podStr.begin(), pod % 10 + '0');
			pod /= 10;
		}
		ipBase.append(podStr);
		ipBase.insert(ipBase.end(), '.');
	}
	if (s == 0)
		ipBase.append("0.0");
	else
	{
		std::string sStr;
		while (s)
		{
			sStr.insert(sStr.begin(), s % 10 + '0');
			s /= 10;
		}
		ipBase.append(sStr);
		ipBase.append(".0");
	}
	char *res = new char[ipBase.size() + 1];
	for (size_t i = 0; i < ipBase.size(); i++)
		res[i] = ipBase[i];
	res[ipBase.size()] = '\0';
	return res;
}


int main(int argc, char *argv[])
{
	unsigned long mainStart = getTickCount();

	unsigned int podNum = 4;
	std::string linkDataRate("1000Mbps");
	std::string linkDelay("0.01ms");
	int packetSize = 1024;
	std::string onOffDataRate("1Mbps");
	std::string maxBytes("0");


	LogComponentEnable("P4Example", LOG_LEVEL_LOGIC);
	//LogComponentEnable("BridgeNetDevice", LOG_LEVEL_LOGIC);

	CommandLine cmd;
	cmd.AddValue("podnum", "Numbers of built tree topo levels", podNum);
	cmd.Parse(argc, argv);

	unsigned int coreSwitchNum = podNum*podNum / 4;
	unsigned int hostNum = coreSwitchNum*podNum;
	unsigned int switchNum = coreSwitchNum * 5;

	unsigned int aggrSwitchStartIndex = coreSwitchNum;
	unsigned int edgeSwitchStartIndex = 3 * coreSwitchNum;

	unsigned int halfPodNum = podNum / 2;

	NodeContainer hosts;
	hosts.Create(hostNum);
	NodeContainer switches;
	switches.Create(switchNum);

	std::vector<HostUnit_t> hostInfos(hostNum);
	std::vector<SwitchUnit_t> switchInfos(switchNum);

	NetDeviceContainer aggrSwNetDev[podNum][halfPodNum];// number pod number aggr switch
	NetDeviceContainer edgeSwNetDev[podNum][halfPodNum];// number pod number edge switch (including: link host, edge switch, link aggr switch)
	Ipv4InterfaceContainer edgeSwNetDevIp[podNum][halfPodNum];

	CsmaHelper csma;
	csma.SetChannelAttribute("DataRate", StringValue(linkDataRate));
	csma.SetChannelAttribute("Delay", StringValue(linkDelay));

	// Initialize Internet Stack and Routing Protocols
	//	
	InternetStackHelper internet;
	Ipv4NixVectorHelper nixRouting;
	Ipv4StaticRoutingHelper staticRouting;
	Ipv4ListRoutingHelper list;
	list.Add(staticRouting, 0);
	list.Add(nixRouting, 10);
	internet.SetRoutingHelper(list);
	internet.Install(hosts);
	internet.Install(switches);

	//=========== 1 Connect aggregate switches to edge switches, edge switches to hosts  ===========//
	unsigned int curAggrSwitchIndex;
	unsigned int curEdgeSwitchIndex;
	unsigned int aggrLinkEdgeSwitchIndex;
	unsigned int edgeLinkHostIndex;

	Ipv4AddressHelper ipv4;
	//ipv4.SetBase("10.0.0.0", "255.255.255.0", "0.0.0.2");
	//=========== 1.1 for edge switch and host link  ===========//
	for (unsigned int i = 0; i < podNum; i++)// number pod
	{
		for (unsigned int j = 0; j < halfPodNum; j++) // number switch in pod
		{
			// set host ip base
			//ipv4.SetBase(GetHostIpAddrBase(i, j),"255.255.255.0");

			curAggrSwitchIndex = aggrSwitchStartIndex + i*halfPodNum + j;
			curEdgeSwitchIndex = edgeSwitchStartIndex + i*halfPodNum + j;
			for (unsigned int p = 0; p < halfPodNum; p++)
			{
				//aggrLinkEdgeSwitchIndex = edgeSwitchStartIndex + i*halfPodNum + p;
				edgeLinkHostIndex = i*coreSwitchNum + j*halfPodNum + p;

				NetDeviceContainer linkEdgeHost = csma.Install(NodeContainer(switches.Get(curEdgeSwitchIndex), hosts.Get(edgeLinkHostIndex)));

				switchInfos[curEdgeSwitchIndex].switchDevices.Add(linkEdgeHost.Get(0));
				hostInfos[edgeLinkHostIndex].hostDevice.Add(linkEdgeHost.Get(1));
				// assign host ip
				//hostInfos[edgeLinkHostIndex].hostIpv4 = ipv4.Assign(hostInfos[edgeLinkHostIndex].hostDevice);

				switchInfos[curEdgeSwitchIndex].isSwitch.push_back(false);
				switchInfos[curEdgeSwitchIndex].index.push_back(edgeLinkHostIndex);
				switchInfos[curEdgeSwitchIndex].portNb.push_back(0);

				hostInfos[edgeLinkHostIndex].index = curEdgeSwitchIndex;
				hostInfos[edgeLinkHostIndex].portNb = switchInfos[curEdgeSwitchIndex].switchDevices.GetN() - 1;

			}
		}
	}

	//=========== 1.2 for aggr switch and edge switch link  ===========//
	for (unsigned int i = 0; i < podNum; i++)// number pod
	{
		for (unsigned int j = 0; j < halfPodNum; j++) // number switch in pod
		{
			curAggrSwitchIndex = aggrSwitchStartIndex + i*halfPodNum + j;
			curEdgeSwitchIndex = edgeSwitchStartIndex + i*halfPodNum + j;

			for (unsigned int p = 0; p < halfPodNum; p++)
			{
				aggrLinkEdgeSwitchIndex = edgeSwitchStartIndex + i*halfPodNum + p;

				NetDeviceContainer linkAggrEdge = csma.Install(NodeContainer(switches.Get(curAggrSwitchIndex), switches.Get(aggrLinkEdgeSwitchIndex)));

				switchInfos[curAggrSwitchIndex].switchDevices.Add(linkAggrEdge.Get(0));
				switchInfos[aggrLinkEdgeSwitchIndex].switchDevices.Add(linkAggrEdge.Get(1));

				switchInfos[curAggrSwitchIndex].isSwitch.push_back(true);
				switchInfos[curAggrSwitchIndex].index.push_back(aggrLinkEdgeSwitchIndex);
				switchInfos[curAggrSwitchIndex].portNb.push_back(switchInfos[aggrLinkEdgeSwitchIndex].switchDevices.GetN() - 1);

				switchInfos[aggrLinkEdgeSwitchIndex].isSwitch.push_back(true);
				switchInfos[aggrLinkEdgeSwitchIndex].index.push_back(curAggrSwitchIndex);
				switchInfos[aggrLinkEdgeSwitchIndex].portNb.push_back(switchInfos[curAggrSwitchIndex].switchDevices.GetN() - 1);
			}
		}
	}

	//===========2 Connect core switches to aggregate switches ===========//
	unsigned int startPodI = 0;
	unsigned int k;
	unsigned int coreLinkAggrSwitchIndex;
	for (unsigned int i = 0; i < coreSwitchNum; i++)//traverse core switch
	{
		k = startPodI;
		for (unsigned int j = 0; j < podNum; j++)//traverse pod
		{
			coreLinkAggrSwitchIndex = aggrSwitchStartIndex + j*halfPodNum + k;
			NetDeviceContainer linkCoreAggr = csma.Install(NodeContainer(switches.Get(i), switches.Get(coreLinkAggrSwitchIndex)));

			switchInfos[i].switchDevices.Add(linkCoreAggr.Get(0));
			switchInfos[coreLinkAggrSwitchIndex].switchDevices.Add(linkCoreAggr.Get(1));

			switchInfos[i].isSwitch.push_back(true);
			switchInfos[i].index.push_back(coreLinkAggrSwitchIndex);
			switchInfos[i].portNb.push_back(switchInfos[coreLinkAggrSwitchIndex].switchDevices.GetN() - 1);

			switchInfos[coreLinkAggrSwitchIndex].isSwitch.push_back(true);
			switchInfos[coreLinkAggrSwitchIndex].index.push_back(i);
			switchInfos[coreLinkAggrSwitchIndex].portNb.push_back(switchInfos[i].switchDevices.GetN() - 1);
			//update k
			if (k != halfPodNum - 1)
				k++;
			else
				k = 0;
		}
		// update startPodI
		if (startPodI != halfPodNum - 1)
			startPodI++;
		else
			startPodI = 0;
	}


	//===========3 Bridge switch and switch devices  ===========//
	BridgeHelper bridge;
	for (unsigned int i = 0; i < switchNum; i++)
	{
		bridge.Install(switches.Get(i), switchInfos[i].switchDevices);
	}

	//=========== Assign ip address ===========//

	// Assign edge switch
	for (unsigned int i = 0; i < podNum; i++)
	{
		for (unsigned int j = 0; j < halfPodNum; j++)
		{
			for (unsigned int h = 0; h < halfPodNum; h++)
			{
				unsigned int hostIndex = i*coreSwitchNum + j*halfPodNum + h;
				// Add host network device first
				edgeSwNetDev[i][j].Add(hostInfos[hostIndex].hostDevice);
			}
			unsigned int edgeIndex = edgeSwitchStartIndex + i*halfPodNum + j;

			// Add edge switch network device
			for (unsigned int t = 0; t < switchInfos[edgeIndex].switchDevices.GetN(); t++)
			{
				edgeSwNetDev[i][j].Add(switchInfos[edgeIndex].switchDevices.Get(t));
			}
			// Add aggr switch bottom network device
			std::cout << "edge switch " << edgeIndex << " link aggr switch index:" << std::endl;
			for (unsigned int t = halfPodNum; t < podNum; t++)
			{
				//std::cout<<"t:"<<t<<std::endl;
				//std::cout<<"switchInfos[edgeIndex].index.size:"<<switchInfos[edgeIndex].index.size()<<std::endl;
				unsigned int aggrIndex = switchInfos[edgeIndex].index[t];
				std::cout << aggrIndex << " ";
				unsigned int aggrPort = switchInfos[edgeIndex].portNb[t];
				edgeSwNetDev[i][j].Add(switchInfos[aggrIndex].switchDevices.Get(aggrPort));
			}
			std::cout << std::endl;
			ipv4.SetBase(GetHostIpAddrBase(i, j), "255.255.255.0");
			edgeSwNetDevIp[i][j]=ipv4.Assign(edgeSwNetDev[i][j]);

		}
	}

	for (unsigned int i = 0; i < podNum; i++)
	{
		for (unsigned int j = 0; j < halfPodNum; j++)
		{
			unsigned int aggrIndex = aggrSwitchStartIndex + i*halfPodNum + j;
			// Add aggr switch top network device
			for (unsigned int t = halfPodNum; t < switchInfos[aggrIndex].switchDevices.GetN(); t++)
			{
				aggrSwNetDev[i][j].Add(switchInfos[aggrIndex].switchDevices.Get(t));
			}
			// Add core switch bottom network device
			std::cout << "aggr switch " << aggrIndex << " link core switch index:";
			for (unsigned int t = halfPodNum; t < podNum; t++)
			{
				unsigned int coreIndex = switchInfos[aggrIndex].index[t];
				std::cout << coreIndex << " ";
				unsigned int corePort = switchInfos[aggrIndex].portNb[t];
				aggrSwNetDev[i][j].Add(switchInfos[coreIndex].switchDevices.Get(corePort));
			}
			std::cout << std::endl;
			ipv4.SetBase(GetHostIpAddrBase(i+podNum, j), "255.255.255.0");
			ipv4.Assign(aggrSwNetDev[i][j]);
		}
	}


	//=========== Show switch and host info  ===========//
	ShowSwitchHostInfo(switchInfos, hostInfos);

	//=========== Build OnOff applications  ===========//

	ApplicationContainer apps;
	unsigned int halfHostNum = hostNum / 2;
	for (unsigned int i = 0; i<halfHostNum; i++)
	{
		unsigned int serverI = hostNum - i - 1;

		//Ipv4Address serverAddr = hostInfos[serverI].hostIpv4.GetAddress(0);
		unsigned int s_p, s_s, s_h;
		s_p = serverI / coreSwitchNum;
		s_s = (serverI-s_p*coreSwitchNum)/halfPodNum;
		s_h = serverI%halfPodNum;
		std::cout<<"s_p s_s s_h:"<<s_p<<" "<<s_s<<" "<<s_h<<std::endl;
		std::cout<<"edgeSwNetDevIp[s_p][s_s]:"<<edgeSwNetDevIp[s_p][s_s].GetN()<<std::endl;
		Ipv4Address serverAddr = edgeSwNetDevIp[s_p][s_s].GetAddress(s_h);
		InetSocketAddress dst = InetSocketAddress(serverAddr);

		OnOffHelper onOff = OnOffHelper("ns3::TcpSocketFactory", dst);
		onOff.SetAttribute("PacketSize", UintegerValue(packetSize));
		onOff.SetAttribute("DataRate", StringValue(onOffDataRate));
		onOff.SetAttribute("MaxBytes", StringValue(maxBytes));

		apps = onOff.Install(hosts.Get(i));
		apps.Start(Seconds(1.0));
		apps.Stop(Seconds(100.0));

		PacketSinkHelper sink = PacketSinkHelper("ns3::TcpSocketFactory", dst);
		apps = sink.Install(hosts.Get(serverI));
		apps.Start(Seconds(0.0));
		apps.Stop(Seconds(101.0));
	}

	//csma.EnablePcapAll("p4-example", false);
	Config::ConnectWithoutContext("/NodeList/3/ApplicationList/0/$ns3::PacketSink/Rx",
		MakeCallback(&SinkRx));
	//Config::Connect("/NodeList/*/ApplicationList/*/$ns3::V4Ping/Rtt",
		//MakeCallback(&PingRtt));
	//Packet::EnablePrinting();
	
	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	unsigned long simulateStart = getTickCount();
	Simulator::Run();
	Simulator::Destroy();
	//NS_LOG_INFO("Done.");
	unsigned long end = getTickCount();

	std::cout << "Host Num: " << hostNum << " Switch Num: " << switchNum << std::endl;
	std::cout << "Program Running time: " << end - mainStart << "ms" << std::endl;
	std::cout << "Simulate Running time: " << end - simulateStart << "ms" << std::endl;
}



