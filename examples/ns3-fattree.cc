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
*
* Author: PengKuang <kphf1995cm@outlook.com>
*/

#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>

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
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include "ns3/global.h"
#include "ns3/p4-topology-reader-helper.h"
#include "ns3/helper.h"
#include "ns3/build-flowtable-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("P4Example");

static void SinkRx (Ptr<const Packet> p, const Address &ad) {
    std::cout << "Rx" << "Received from  "<< ad << std::endl;
}

static void PingRtt (std::string context, Time rtt) {
    std::cout << "Rtt" << context << " " << rtt << std::endl;
}

struct SwitchNodeC_t
{
	NetDeviceContainer switchDevices;
	std::vector<std::string> switchPortInfos;
};

struct HostNodeC_t
{
	NetDeviceContainer hostDevice;
	Ipv4InterfaceContainer hostIpv4;
	unsigned int linkSwitchIndex;
	unsigned int linkSwitchPort;
	std::string hostIpv4Str;
};

int main(int argc, char *argv[])
{

	unsigned long mainStart=getTickCount();

	// init global variable 	
	P4GlobalVar::g_homePath="/home/kphf1995cm/";
	P4GlobalVar::g_ns3RootName = "ns-allinone-3.26/";
	P4GlobalVar::g_ns3SrcName = "ns-3.26/"; 
	P4GlobalVar::g_nfDir = P4GlobalVar::g_homePath + P4GlobalVar::g_ns3RootName + P4GlobalVar::g_ns3SrcName + "src/ns4/test/";
	P4GlobalVar::g_topoDir = P4GlobalVar::g_homePath + P4GlobalVar::g_ns3RootName + P4GlobalVar::g_ns3SrcName + "src/ns4/topo/";
	P4GlobalVar::g_nsType = NS3;
	P4GlobalVar::g_runtimeCliTime=10;
        SwitchApi::InitApiMap();
	P4GlobalVar::InitNfStrUintMap();

	int podNum = 2;
	int toBuild=1;  // whether build flow table entired by program
	int application=0;   //application type (0 onOff Sink)
	
	// start debug module
	LogComponentEnable("P4Example", LOG_LEVEL_LOGIC);
	LogComponentEnable("P4NetDevice", LOG_LEVEL_LOGIC);
	LogComponentEnable("BridgeNetDevice",LOG_LEVEL_LOGIC);
	LogComponentEnable("CsmaTopologyReader", LOG_LEVEL_LOGIC);
	LogComponentEnable("BuildFlowtableHelper",LOG_LEVEL_LOGIC);
	LogComponentEnable("P4SwitchInterface",LOG_LEVEL_LOGIC);
	
	// define topo format,path
	std::string topoFormat("CsmaTopo");
	std::string topoPath = P4GlobalVar::g_topoDir + "csmaTopo.txt";
	NS_LOG_LOGIC(topoPath);
	std::string topoInput(topoPath);
        
	// define command line parse way
	CommandLine cmd;
	cmd.AddValue("format", "Format to use for data input [Orbis|Inet|Rocketfuel|CsmaTopo].",
		topoFormat);
	cmd.AddValue("model", "Select p4 model[0] or traditional bridge model[1]", P4GlobalVar::g_nsType);
	cmd.AddValue("podnum", "Numbers of built tree topo levels", podNum);
	cmd.AddValue("build","Build flow table entries by program[1] or not[0]",toBuild);
	cmd.AddValue("application","Application type OnoffSink[0] UdpClientServer[1]",application);
	cmd.Parse(argc, argv);
	
	// build topo
	/*
	You can build network topo by program or handwork, we use handwork to build topo to test topology reader program.
	*/
	FattreeTopoHelper treeTopo(podNum,topoPath);
	//BinaryTreeTopoHelper treeTopo(podNum,topoPath);
	treeTopo.Write();
        
	// read topo
	P4TopologyReaderHelper p4TopoHelp;
	p4TopoHelp.SetFileName(topoInput);
	p4TopoHelp.SetFileType(topoFormat);
	Ptr<P4TopologyReader> topoReader = p4TopoHelp.GetTopologyReader();
	if (topoReader != 0)
	{
		topoReader->Read();
	}
	if (topoReader->LinksSize() == 0)
	{
		NS_LOG_ERROR("Problems reading the topology file. Failing.");
		return -1;
	}

	// get switch and host node
	NodeContainer hosts = topoReader->GetHostNodeContainer();
	NodeContainer csmaSwitch = topoReader->GetSwitchNodeContainer();
	const unsigned int hostNum = hosts.GetN();
	const unsigned int switchNum = csmaSwitch.GetN();

	// get switch network function
	std::vector<std::string> switchNetFunc=topoReader->GetSwitchNetFunc();

	NS_LOG_LOGIC("switchNum:" << switchNum<< "hostNum:" << hostNum);

	// set default network link parameter
	CsmaHelper csma;
	csma.SetChannelAttribute("DataRate", StringValue("1000Mbps"));
	csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(0.01)));

	// init network link info
	P4TopologyReader::ConstLinksIterator_t iter;
	SwitchNodeC_t switchNodes[switchNum];
	HostNodeC_t hostNodes[hostNum];
	unsigned int fromIndex, toIndex;
	std::string dataRate,delay;
	for (iter = topoReader->LinksBegin(); iter != topoReader->LinksEnd(); iter++)
	{
		if(iter->GetAttributeFailSafe("DataRate",dataRate))
			csma.SetChannelAttribute("DataRate", StringValue(dataRate));
		if(iter->GetAttributeFailSafe("Delay",delay))
                        csma.SetChannelAttribute("Delay", StringValue(delay));

		NetDeviceContainer link = csma.Install(NodeContainer(iter->GetFromNode(), iter->GetToNode()));
		fromIndex = iter->GetFromIndex();
		toIndex = iter->GetToIndex();
		if (iter->GetFromType()=='s' && iter->GetToType()=='s')
		{
			unsigned int fromSwitchPortNumber = switchNodes[fromIndex].switchDevices.GetN();
			unsigned int toSwitchPortNumber = switchNodes[toIndex].switchDevices.GetN();
			switchNodes[fromIndex].switchDevices.Add(link.Get(0));
			switchNodes[fromIndex].switchPortInfos.push_back("s"+UintToString(toIndex)+"_"+UintToString(toSwitchPortNumber));

			switchNodes[toIndex].switchDevices.Add(link.Get(1));
			switchNodes[toIndex].switchPortInfos.push_back("s" + UintToString(fromIndex) + "_" + UintToString(fromSwitchPortNumber));
		}
		else
		{
			if (iter->GetFromType() == 's' && iter->GetToType() == 'h')
			{
				unsigned int fromSwitchPortNumber = switchNodes[fromIndex].switchDevices.GetN();
				switchNodes[fromIndex].switchDevices.Add(link.Get(0));
				switchNodes[fromIndex].switchPortInfos.push_back("h" + UintToString(toIndex-switchNum));
				
				hostNodes[toIndex - switchNum].hostDevice.Add(link.Get(1));
				hostNodes[toIndex - switchNum].linkSwitchIndex = fromIndex;
				hostNodes[toIndex - switchNum].linkSwitchPort = fromSwitchPortNumber;
			}
			else
			{
				if (iter->GetFromType() == 'h' && iter->GetToType() == 's')
				{
					unsigned int toSwitchPortNumber = switchNodes[toIndex].switchDevices.GetN();
					switchNodes[toIndex].switchDevices.Add(link.Get(1));
					switchNodes[toIndex].switchPortInfos.push_back("h" + UintToString(fromIndex - switchNum));

					hostNodes[fromIndex - switchNum].hostDevice.Add(link.Get(0));
					hostNodes[fromIndex - switchNum].linkSwitchIndex = toIndex;
					hostNodes[fromIndex - switchNum].linkSwitchPort = toSwitchPortNumber;
				}
				else
				{
					NS_LOG_LOGIC("link error!");
					abort();
				}
			}
		}
	}

	// view host link info
	for (unsigned int i = 0; i < hostNum; i++)
		std::cout << "h" << i << ": " << hostNodes[i].linkSwitchIndex << " " << hostNodes[i].linkSwitchPort<<std::endl;

	// view switch port info
	for (unsigned int i = 0; i < switchNum; i++)
	{
		std::cout << "s" << i << ": ";
		for (size_t k = 0; k < switchNodes[i].switchPortInfos.size(); k++)
			std::cout << switchNodes[i].switchPortInfos[k] << " ";
		std::cout << std::endl;
	}

	// add internet stack to the hosts
	InternetStackHelper internet;
	internet.Install(hosts);

	//assign ip address
	NS_LOG_LOGIC("assign ip address");
	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.1.0.0", "255.255.0.0");
	for (unsigned int i = 0; i < hostNum; i++)
	{
		hostNodes[i].hostIpv4 = ipv4.Assign(hostNodes[i].hostDevice);
		//ipv4.NewNetwork();
		hostNodes[i].hostIpv4Str = Uint32ipToHex(hostNodes[i].hostIpv4.GetAddress(0).Get());
		std::cout<<i<<" "<<hostNodes[i].hostIpv4Str<<std::endl;
	}
	
	//build needed parameter to build flow table entries 
	std::vector<unsigned int> linkSwitchIndex(hostNum);
	std::vector<unsigned int> linkSwitchPort(hostNum);
	std::vector<std::string> hostIpv4(hostNum);
	std::vector<std::vector<std::string>> switchPortInfo(switchNum);
	for(unsigned int i=0;i<hostNum;i++)
	{
		linkSwitchIndex[i]=hostNodes[i].linkSwitchIndex;
		linkSwitchPort[i]=hostNodes[i].linkSwitchPort;
		hostIpv4[i]=hostNodes[i].hostIpv4Str;
	}
	for(unsigned int i=0;i<switchNum;i++)
	{
		switchPortInfo[i]=switchNodes[i].switchPortInfos;
	}

	//build flow table entries by program
	if(toBuild==1&&P4GlobalVar::g_nsType==NS4)
	{
		NS_LOG_LOGIC("BuildFlowtableHelper");
		//BuildFlowtableHelper flowtableHelper("fattree",podNum);
		BuildFlowtableHelper flowtableHelper;
		flowtableHelper.Build(linkSwitchIndex,linkSwitchPort,hostIpv4,switchPortInfo);
		flowtableHelper.Write(P4GlobalVar::g_flowTableDir);
		flowtableHelper.Show();
	}

	//bridge siwtch and switch devices
	if (P4GlobalVar::g_nsType == NS4)
	{
		P4GlobalVar::g_populateFlowTableWay = LOCAL_CALL;
		std::string flowTableName;
		P4Helper bridge;
		for (unsigned int i = 0; i < switchNum; i++)
		{
			flowTableName = UintToString(i);
			//P4GlobalVar::g_networkFunc = SIMPLE_ROUTER;
			P4GlobalVar::g_networkFunc = P4GlobalVar::g_nfStrUintMap[switchNetFunc[i]];
			P4GlobalVar::SetP4MatchTypeJsonPath();
			P4GlobalVar::g_flowTablePath = P4GlobalVar::g_flowTableDir + flowTableName;
			bridge.Install(csmaSwitch.Get(i),switchNodes[i].switchDevices);// do what?
		}
	}
	else
	{
		BridgeHelper bridge;
		for (unsigned int i = 0; i < switchNum; i++)
		{
			bridge.Install(csmaSwitch.Get(i), switchNodes[i].switchDevices);
		}
	}

	//build application
	Config::SetDefault("ns3::Ipv4RawSocketImpl::Protocol",StringValue("2"));
	if(application==0) //Onoff Sink
	{
		NS_LOG_LOGIC("OnoffSink");
		ApplicationContainer apps;
		unsigned int halfHostNum=hostNum/2;
		for(unsigned int i=0;i<halfHostNum;i++)
		{
			unsigned int serverI=hostNum-i-1;
			Ipv4Address serverAddr=hostNodes[serverI].hostIpv4.GetAddress(0);
			InetSocketAddress dst=InetSocketAddress(serverAddr);
			
			OnOffHelper onOff=OnOffHelper("ns3::TcpSocketFactory",dst);
			onOff.SetAttribute("PacketSize",UintegerValue(1024));
			onOff.SetAttribute("DataRate",StringValue("1Mbps"));
			onOff.SetAttribute("MaxBytes",StringValue("0"));

			apps=onOff.Install(hosts.Get(i));
			apps.Start(Seconds(1.0));
			apps.Stop(Seconds(100.0));

			PacketSinkHelper sink=PacketSinkHelper("ns3::TcpSocketFactory",dst);
			apps=sink.Install(hosts.Get(serverI));
			apps.Start(Seconds(0.0));
			apps.Stop(Seconds(101.0));
		}
	}
	if(application==1) // Udp Echo Client Server Application
	{
		NS_LOG_LOGIC("Udp Echo Client Server");
                unsigned int halfHostNum=hostNum/2;
                for(unsigned int i=0;i<halfHostNum;i++)
                {
                        unsigned int serverI=hostNum-i-1;
			UdpServerHelper echoServer(9);
			ApplicationContainer serverApps=echoServer.Install(hosts.Get(serverI));
			serverApps.Start(Seconds(1.0));
			serverApps.Stop(Seconds(10.0));			
                        Ipv4Address serverAddr=hostNodes[serverI].hostIpv4.GetAddress(0);

			UdpEchoClientHelper echoClient(serverAddr,9);
			echoClient.SetAttribute ("MaxPackets", UintegerValue (20));
  			echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  			echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  			ApplicationContainer clientApps = echoClient.Install (hosts.Get (i));
			clientApps.Start (Seconds (2.0));
  			clientApps.Stop (Seconds (10.0));
                }

	}

	csma.EnablePcapAll ("p4-example", false);
	Config::ConnectWithoutContext("/NodeList/3/ApplicationList/0/$ns3::PacketSink/Rx",
		MakeCallback(&SinkRx));
	Config::Connect("/NodeList/*/ApplicationList/*/$ns3::V4Ping/Rtt",
		MakeCallback(&PingRtt));
  	Packet::EnablePrinting ();

	unsigned long simulateStart=getTickCount();        
	Simulator::Run ();
  	Simulator::Destroy ();
	//NS_LOG_INFO("Done.");
	unsigned long end=getTickCount();

	std::cout << "Host Num: " << hostNum << " Switch Num: " << switchNum << std::endl;
	std::cout << "Program Running time: " << end - mainStart<<"ms" << std::endl;
	std::cout << "Simulate Running time: " << end - simulateStart<<"ms" << std::endl;
	//return 0;
}

