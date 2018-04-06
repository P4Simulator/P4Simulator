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

/*
*          Network topology discription
*          _________________________
*          |                        |
*          |        switch          |
*          |                        |
*          |________________________|
*            |      |      |      |
*           h0(dst) h1     h2     h3(src)
*      ip:10.1.1.1              10.1.1.4
*     mac:00:00:00:00:00:01     00:00:00:00:00:07
*application:UdpEchoServer      UdpEchoClient
* simulate start:h3 send ArpHeader packet whose dstIp is 10.1.1.1 to switch.
*
*/

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unordered_map>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/p4-helper.h"
#include "ns3/v4ping-helper.h"

#include "ns3/global.h"
#include <bm/SimpleSwitch.h>
#include <bm/bm_runtime/bm_runtime.h>
#include <bm/bm_sim/target_parser.h>
#include "ns3/switch-api.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("P4Example");


int main(int argc, char *argv[])
{
	unsigned long start = getTickCount();

	LogComponentEnable("P4Example", LOG_LEVEL_LOGIC);
	//LogComponentEnable("P4NetDevice", LOG_LEVEL_LOGIC);

	// Initialize global variable
	//
	P4GlobalVar::g_homePath = "/home/kphf1995cm/";
	P4GlobalVar::g_ns3RootName = "ns-allinone-3.26/";
	P4GlobalVar::g_ns3SrcName = "ns-3.26/";
	P4GlobalVar::g_nfDir = P4GlobalVar::g_homePath + P4GlobalVar::g_ns3RootName + P4GlobalVar::g_ns3SrcName + "src/ns4/test/";
	P4GlobalVar::g_nsType = NS3; // NS4 0   NS3 1
	P4GlobalVar::g_runtimeCliTime = 10;
	SwitchApi::InitApiMap();
	P4GlobalVar::InitNfStrUintMap();

	// Initialize parameters for UdpEcho Client/Server application
	//
	int port = 9;
	unsigned int packetSize = 20;            // send packet data size (byte)
	double interval = 1; // send packet interval time (ms)
	unsigned maxPackets = 1000; // send packet max number

	// Initialize parameters for Csma protocol
	//
	char dataRate[] = "100Gbps";   // 1Gbps
	double delay = 0.001;              // 0.001 (ms)

	// Initalize parameters for UdpEcho Client/Server Appilication 
	//
	int clientStartTime = 1; // UdpEchoClient Start Time (s)
	int clientStopTime = 10;
	int serverStartTime = 0; // UdpEchoServer Start Time (s)
	int serverStopTime = 11;


	// Add running changeable variable
	//
	CommandLine cmd;
	cmd.AddValue("ns", "Network simulator type [NS4 0 NS3 1]", P4GlobalVar::g_nsType);
	cmd.AddValue("inter", "UdpEchoClient send packet interval time/s [default 1ms]",interval);
	cmd.AddValue("delay"," Csma channel delay time/ms [default 0.001ms]",delay);
	cmd.Parse(argc, argv);

	maxPackets = (clientStopTime - clientStartTime) / interval*1000;

	if (P4GlobalVar::g_nsType == NS3)
		std::cout << "NS3 Simulate Mode:" << std::endl;
	else
		std::cout << "NS4 Simulate Mode:" << std::endl;
	std::cout << "Simulate Time: " << clientStopTime - clientStartTime <<" s"<< std::endl;
	std::cout << "Csma Channel DataRate: " << dataRate << std::endl;
	std::cout << "Csma Channel Delay: " << delay << " ms" << std::endl;
	std::cout << "Send Packet Interval: " << interval << " ms" << std::endl;
	std::cout << "Send Packet MaxNum: " << maxPackets << std::endl;
	std::cout<<"Send Packet Data Size: "<<packetSize<<" byte"<<std::endl;


	//std::cout << "----------------Create nodes----------------" << std::endl;
	NodeContainer terminals;
	terminals.Create(4);
	NodeContainer csmaSwitch;
	csmaSwitch.Create(1);

	//std::cout << "---------------Build Topology----------------" << std::endl;
	CsmaHelper csma;
	csma.SetChannelAttribute("DataRate", StringValue(dataRate));
	csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(delay)));

	NetDeviceContainer terminalDevices;
	NetDeviceContainer switchDevices;

	for (int i = 0; i < 4; i++)
	{
		NetDeviceContainer link = csma.Install(NodeContainer(terminals.Get(i), csmaSwitch));
		terminalDevices.Add(link.Get(0));
		switchDevices.Add(link.Get(1));
	}
	
	//std::cout << "---------------Bridge switch and network device----------------" << std::endl;
	if (P4GlobalVar::g_nsType == NS4) //ns4 mode
	{
		P4GlobalVar::g_populateFlowTableWay = LOCAL_CALL;//LOCAL_CALL RUNTIME_CLI
		P4GlobalVar::g_networkFunc = ROUTER;
		P4GlobalVar::SetP4MatchTypeJsonPath();
		P4GlobalVar::g_flowTablePath = P4GlobalVar::g_nfDir + "router/command.txt";
		P4GlobalVar::g_viewFlowTablePath = P4GlobalVar::g_nfDir + "router/view.txt";
		P4Helper bridge;
		bridge.Install(csmaSwitch.Get(0), switchDevices);

		// view switch flowtable info by controller before simulate
		//P4GlobalVar::g_p4Controller.ViewP4SwitchFlowTableInfo(0);
	}
	else //ns3 mode
	{
		BridgeHelper bridge;
		bridge.Install(csmaSwitch.Get(0), switchDevices);
	}

	//std::cout << "---------------Assign Ip Address----------------" << std::endl;
	// Add internet stack to the terminals
	InternetStackHelper internet;
	internet.Install(terminals);

	// Assign IP addresses.
	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer addresses = ipv4.Assign(terminalDevices);

	
	//std::cout << "---------------Create Applications----------------" << std::endl;
	Config::SetDefault("ns3::Ipv4RawSocketImpl::Protocol", StringValue("2"));

	// Install Server Application
	UdpEchoServerHelper echoServer(port);
	ApplicationContainer serverApps = echoServer.Install(terminals.Get(0));
	serverApps.Start(Seconds(serverStartTime));
	serverApps.Stop(Seconds(serverStopTime));

	Ipv4Address dstIp=addresses.GetAddress(0);
	
	UdpEchoClientHelper echoClient(dstIp, port);
	echoClient.SetAttribute("MaxPackets", UintegerValue(maxPackets));
	echoClient.SetAttribute("Interval", TimeValue(MilliSeconds(interval)));
	echoClient.SetAttribute("PacketSize", UintegerValue(packetSize));
	ApplicationContainer clientApps = echoClient.Install(terminals.Get(3));
	clientApps.Start(Seconds(clientStartTime));
	clientApps.Stop(Seconds(clientStopTime));

	csma.EnablePcapAll ("p4-example", false);
	Packet::EnablePrinting();

	std::cout << "---------------Run Simulation----------------" << std::endl;
	
	unsigned long simulate_start = getTickCount();
	Simulator::Stop(Seconds(serverStopTime + 1));

	Simulator::Run();

	if (P4GlobalVar::g_nsType == NS4) //ns4 mode
	{
		//view switch flowtable info after simulate
		//P4GlobalVar::g_p4Controller.ViewP4SwitchFlowTableInfo(0);
	}

	//********View switch receive packet Num***************************
	std::cout << "CsmaSwitch :" << csmaSwitch.Get(0)->m_packetNum << std::endl;
	unsigned int tN = terminals.GetN();
	for (unsigned int i = 0; i<tN; i++)
		std::cout << "Terminal " << i << " : " << terminals.Get(i)->m_packetNum << std::endl;
	//*****************************************************************


	Simulator::Destroy();
	
	unsigned long end = getTickCount();
	std::cout << "Simulate Running time: " << end - simulate_start << "ms" << std::endl;
	std::cout << "Running time: " << end - start << "ms" << std::endl;
	std::cout << "Run successfully!" << std::endl;
	return 0;
}



