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
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include "ns3/global.h"
#include "ns3/p4-topology-reader-helper-new.h"
#include "ns3/helper.h"
using namespace ns3;

unsigned int P4GlobalVar::g_networkFunc=ROUTER;
std::string  P4GlobalVar::g_flowTablePath="";
std::string  P4GlobalVar::g_p4MatchTypePath="";
unsigned int P4GlobalVar::g_populateFlowTableWay=LOCAL_CALL;

std::string P4GlobalVar::g_homePath="/home/kphf1995cm/";

std::string P4GlobalVar::g_ns3RootName = "ns-allinone-3.26/";
std::string P4GlobalVar::g_ns3SrcName = "ns-3.26/";
std::string P4GlobalVar::g_nfDir = P4GlobalVar::g_homePath + P4GlobalVar::g_ns3RootName + P4GlobalVar::g_ns3SrcName + "src/ns4/test/";
std::string P4GlobalVar::g_topoDir = P4GlobalVar::g_homePath + P4GlobalVar::g_ns3RootName + P4GlobalVar::g_ns3SrcName + "src/ns4/topo/";

unsigned int P4GlobalVar::g_nsType = NS4;

NS_LOG_COMPONENT_DEFINE("P4Example");

// set switch network function, flowtable path and flowtable match type path
// firewall router silkroad

void SetSwitchConfigInfo(std::string ftPath, std::string mtPath)
{
	P4GlobalVar::g_flowTablePath = ftPath;
	P4GlobalVar::g_p4MatchTypePath = mtPath;
}

void InitSwitchConfig()
{
	switch (P4GlobalVar::g_networkFunc)
	{
	case FIREWALL:
	{
		SetSwitchConfigInfo(P4GlobalVar::g_nfDir + "firewall/command.txt", P4GlobalVar::g_nfDir + "firewall/mtype.txt");
		break;
	}
	case ROUTER:
	{
		SetSwitchConfigInfo(P4GlobalVar::g_nfDir + "router/command.txt", P4GlobalVar::g_nfDir + "router/mtype.txt");
		break;
	}
	case SILKROAD:
	{
		SetSwitchConfigInfo(P4GlobalVar::g_nfDir + "silkroad/command.txt", P4GlobalVar::g_nfDir + "silkroad/mtype.txt");
		break;
	}
	default:
	{
		break;
	}
	}
}

struct SwitchNode_t
{
	NetDeviceContainer switchDevices;
	std::vector<std::string> switchPortInfos;
};

struct HostNode_t
{
	NetDeviceContainer hostDevice;
	Ipv4InterfaceContainer hostIpv4;
	unsigned int linkSwitchIndex;
	unsigned int linkSwitchPort;
	std::string hostIpv4Str;
};

int main(int argc, char *argv[])
{
	int podNum = 2;
	// start debug module
	LogComponentEnable("P4Example", LOG_LEVEL_LOGIC);
	NS_LOG_LOGIC("P4-Topo-Test");
	LogComponentEnable("P4NetDevice", LOG_LEVEL_LOGIC);
	LogComponentEnable("CsmaTopologyReader", LOG_LEVEL_LOGIC);
	// define topo format,path
	std::string topoFormat("CsmaTopo");
	std::string topoPath = P4GlobalVar::g_topoDir + "csmaTopo1.txt";
	NS_LOG_LOGIC(topoPath);
	std::string topoInput(topoPath);
        
	// define command line parse way
	CommandLine cmd;
	cmd.AddValue("format", "Format to use for data input [Orbis|Inet|Rocketfuel|CsmaTopo].",
		topoFormat);
	cmd.AddValue("model", "Select p4 model(0) or traditional bridge model(1)", P4GlobalVar::g_nsType);
	cmd.AddValue("podnum", "Numbers of built tree topo levels", podNum);
	cmd.Parse(argc, argv);
	
	// build topo
	/*
	You can build network topo by program or handwork, we use handwork to build topo to test topology reader program.
	*/
        
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
	NS_LOG_LOGIC("switchNum:" << switchNum<< "hostNum:" << hostNum);

	// set default network link parameter
	CsmaHelper csma;
	csma.SetChannelAttribute("DataRate", StringValue("1000Mbps"));
	csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(0.01)));

	// init network link info
	P4TopologyReader::ConstLinksIterator_t iter;
	SwitchNode_t switchNodes[switchNum];
	HostNode_t hostNodes[hostNum];
	unsigned int fromIndex, toIndex;
	for (iter = topoReader->LinksBegin(); iter != topoReader->LinksEnd(); iter++)
	{
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
        
	Simulator::Run ();
  	Simulator::Destroy ();
	//return 0;
}























