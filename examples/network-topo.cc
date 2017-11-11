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
 */


#include <iostream>
#include <fstream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/p4-helper.h"
#include "ns3/v4ping-helper.h"
#include "ns3/p4-topology-reader-helper.h"
#include "ns3/tree-topo-helper.h"
#include "ns3/fattree-topo-helper.h"
#include "ns3/build-flowtable-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <vector>

using namespace ns3;

// a fattree network topology.
NS_LOG_COMPONENT_DEFINE ("P4Example");

std::string network_func;
std::string flowtable_path;
std::string flowtable_matchtype_path;
std::string populate_flowtable_type;

// count simulate time
unsigned long getTickCount(void)
{
        unsigned long currentTime=0;
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


static void SinkRx (Ptr<const Packet> p, const Address &ad) {
    std::cout << "Rx" << "Received from  "<< ad << std::endl;
}

static void PingRtt (std::string context, Time rtt) {
    std::cout << "Rtt" << context << " " << rtt << std::endl;
}

std::string int_to_str(unsigned int num)
{
    std::string res;
    if (num == 0)
        res = "0";
    while (num)
    {
        res.insert(res.begin(),num%10+'0');
        num /= 10;
    }
    return res;
}

std::string uint32ip_to_hex(unsigned int ip)
{
    if (ip != 0)
    {
        std::string res("0x00000000");
        int k = 9;
        int tmp;
        while (ip)
        {
            tmp = ip % 16;
            if (tmp < 10)
                res[k] = tmp + '0';
            else
                res[k] = tmp - 10 + 'a';
            k--;
            ip /= 16;
        }
        return res;
    }
    else
        return std::string("0x00000000");
}

// initialize switch configuration information
void init_switch_config_info(std::string nf,std::string ft_path,std::string ta_mt_path)
{
    network_func=nf;
    flowtable_path=ft_path;
    flowtable_matchtype_path=ta_mt_path;
}


int main (int argc, char *argv[]) {

    unsigned long start = getTickCount();
    int p4 = 1;
    int podnum=2;

    LogComponentEnable ("P4Example", LOG_LEVEL_LOGIC);
    LogComponentEnable ("P4NetDevice", LOG_LEVEL_LOGIC);
  
    // define read topo format  
    std::string format ("CsmaTopo");
    std::string topo_path="/home/kphf1995cm/ns-allinone-3.26/ns-3.26/src/ns4/examples/net_topo/csma_topo.txt";
    std::string input (topo_path);

    CommandLine cmd;
    cmd.AddValue ("format", "Format to use for data input [Orbis|Inet|Rocketfuel|CsmaNetTopo].",
                format);
    cmd.AddValue("p4","Select P4Model or traditional bridge model",p4);
    cmd.AddValue("podnum","Numbers of built tree topo levels",podnum);
    cmd.Parse (argc, argv);
    
    //build fattree topo
    std::cout<<podnum<<std::endl;
    FattreeTopoHelper treeTopo(podnum,topo_path);
    treeTopo.Write();


    // pick a topology reader based in the requested format.
    P4TopologyReaderHelper topoHelp;
    topoHelp.SetFileName (input);
    topoHelp.SetFileType (format);
    Ptr<P4TopologyReader> inFile = topoHelp.GetTopologyReader ();
    if (inFile != 0)
      {
        inFile->Read ();
      }

   if (inFile->LinksSize () == 0)
      {
        NS_LOG_ERROR ("Problems reading the topology file. Failing.");
        return -1;
      }


    NodeContainer terminals=inFile->GetTerminalNodeContainer();
    NodeContainer csmaSwitch=inFile->GetSwitchNodeContainer();
    unsigned int terminalNum=terminals.GetN();
    unsigned int switchNum=csmaSwitch.GetN();
    NS_LOG_LOGIC("switchNum:"<<switchNum);


    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", StringValue("1000Mbps"));
    csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0.01)));


    std::vector<NetDeviceContainer> terminalDevices(switchNum);
    std::vector<Ipv4InterfaceContainer> terminalIpv4s(switchNum);
    std::vector<NetDeviceContainer> switchDevices(switchNum);
    std::vector<std::vector<std::string>> switchPortInfo(switchNum);
    std::vector<unsigned int> terminalNodeLinkedSwitchIndex(terminalNum);
    std::vector<unsigned int> terminalNodeLinkedSwitchPort(terminalNum);
    std::vector<unsigned int> terminalNodeLinkedSwitchNumber(terminalNum);
    std::vector<std::string> terminalNodeIp(terminalNum);
  
 
    P4TopologyReader::ConstLinksIterator iter;
    int i = 0;
    int curTerminalIndex=0;
    for ( iter = inFile->LinksBegin (); iter != inFile->LinksEnd (); iter++, i++ )
    {
         NetDeviceContainer link = csma.Install(NodeContainer (iter->GetFromNode (), iter->GetToNode ()));

         if(iter->GetFromType().compare("terminal")==0&&iter->GetToType().compare("switch")==0)
         {
            terminalDevices[iter->GetFromLinkedSwitchIndex()].Add(link.Get(0));
            terminalNodeLinkedSwitchIndex[curTerminalIndex]=iter->GetToLinkedSwitchIndex();// linked switch index
            terminalNodeLinkedSwitchPort[curTerminalIndex]=switchDevices[iter->GetToLinkedSwitchIndex()].GetN();//linked switch port
            terminalNodeLinkedSwitchNumber[curTerminalIndex]=terminalDevices[iter->GetFromLinkedSwitchIndex()].GetN();

            switchDevices[iter->GetToLinkedSwitchIndex()].Add(link.Get(1));
            switchPortInfo[iter->GetToLinkedSwitchIndex()].push_back("t"+int_to_str(curTerminalIndex));
            curTerminalIndex++;
         }
         else
         {
            if(iter->GetFromType().compare("switch")==0&&iter->GetToType().compare("terminal")==0)
            {
                 terminalDevices[iter->GetToLinkedSwitchIndex()].Add(link.Get(1));
                 terminalNodeLinkedSwitchIndex[curTerminalIndex]=iter->GetFromLinkedSwitchIndex();
                 terminalNodeLinkedSwitchPort[curTerminalIndex]=switchDevices[iter->GetFromLinkedSwitchIndex()].GetN();
                 terminalNodeLinkedSwitchNumber[curTerminalIndex]=terminalDevices[iter->GetToLinkedSwitchIndex()].GetN();

                 switchDevices[iter->GetFromLinkedSwitchIndex()].Add(link.Get(0));
                 switchPortInfo[iter->GetFromLinkedSwitchIndex()].push_back("t"+int_to_str(curTerminalIndex));
                 curTerminalIndex++;
            }
            else
            {
                if(iter->GetFromType().compare("switch")==0&&iter->GetToType().compare("switch")==0)
                {
                    unsigned int fromLinkedPortNum=switchDevices[iter->GetFromLinkedSwitchIndex()].GetN();
                    unsigned int toLinkedPortNum=switchDevices[iter->GetToLinkedSwitchIndex()].GetN();
                    switchDevices[iter->GetFromLinkedSwitchIndex()].Add(link.Get(0));
                    switchPortInfo[iter->GetFromLinkedSwitchIndex()].push_back("s"+int_to_str(iter->GetToLinkedSwitchIndex())+"_"+int_to_str(toLinkedPortNum));

                    switchDevices[iter->GetToLinkedSwitchIndex()].Add(link.Get(1));
                    switchPortInfo[iter->GetToLinkedSwitchIndex()].push_back("s"+int_to_str(iter->GetFromLinkedSwitchIndex())+"_"+int_to_str(fromLinkedPortNum));
                }
                else
                {
                    NS_LOG_LOGIC("link error!");
                    abort();
                }
            }
         }
    }
    // view every terminal link info
    for(size_t k=0;k<terminalNum;k++)
        std::cout<<"t"<<k<<": "<<terminalNodeLinkedSwitchIndex[k]<<" "<<terminalNodeLinkedSwitchPort[k]<<" "<<terminalNodeLinkedSwitchNumber[k]<<std::endl;
    
    // view every switch port info
    for(size_t k=0;k<switchNum;k++)
    {
        std::cout<<"s"<<k<<": ";
        for(std::vector<std::string>::iterator iter=switchPortInfo[k].begin();iter!=switchPortInfo[k].end();iter++)
        {
            std::cout<<*iter<<" ";
        }
        std::cout<<std::endl;
    }


    // Add internet stack to the terminals
    InternetStackHelper internet;
    internet.Install (terminals);

    NS_LOG_INFO ("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.1.0.0", "255.255.0.0");
    for(unsigned int k=0;k<switchNum;k++)
    {
        if(terminalDevices[k].GetN()>0)
        {
            terminalIpv4s[k]=ipv4.Assign(terminalDevices[k]);
        }

    //    ipv4.NewNetwork ();
    }
    
    // view every terminal ip
    for(size_t k=0;k<terminalNum;k++)
    {
        std::cout<<"t"<<k<<": "<<terminalIpv4s[terminalNodeLinkedSwitchIndex[k]].GetAddress (terminalNodeLinkedSwitchNumber[k]-1)<<std::endl;
        terminalNodeIp[k]=uint32ip_to_hex(terminalIpv4s[terminalNodeLinkedSwitchIndex[k]].GetAddress (terminalNodeLinkedSwitchNumber[k]-1).Get());
    }

    // build flowtable entries
    bool buildAlready=false;
    if(buildAlready==false&&p4)
    {
        BuildFlowtableHelper flowtableHelper("fattree",podnum);
        flowtableHelper.build(terminalNodeLinkedSwitchIndex,terminalNodeLinkedSwitchPort,terminalNodeIp,switchPortInfo);
        flowtableHelper.write("/home/kphf1995cm/ns-allinone-3.26/ns-3.26/src/ns4/test/simple_router/table_entries");
       // flowtableHelper.show();
    }

    if (p4) {
	populate_flowtable_type="local_call";
        std::string flowtable_parentdir("/home/kphf1995cm/ns-allinone-3.26/ns-3.26/src/ns4/test/simple_router/table_entries/");
        std::string flowtable_name;
        for(unsigned int k=0;k<switchNum;k++)
        {
            flowtable_name=int_to_str(k);
            network_func="simple_router";
            flowtable_matchtype_path="/home/kphf1995cm/ns-allinone-3.26/ns-3.26/src/ns4/test/simple_router/match_type.txt";
            P4Helper bridge;
            NS_LOG_INFO("P4 bridge established");
            flowtable_path=flowtable_parentdir+flowtable_name; 
            bridge.Install (csmaSwitch.Get(k), switchDevices[k]);
        }
    } else {
       BridgeHelper bridge;
       for(unsigned int k=0;k<switchNum;k++)
       {
         bridge.Install (csmaSwitch.Get(k), switchDevices[k]);
       }
    }

    NS_LOG_INFO ("Create Applications.");
    NS_LOG_INFO ("Create Source");
    Config::SetDefault ("ns3::Ipv4RawSocketImpl::Protocol", StringValue ("2"));
    
    std::string applicationType="onoff_sink";
    if(applicationType.compare("onoff_sink")==0)
    {
        ApplicationContainer apps;
	unsigned int halfTmlNum=terminalNum/2;
        for(unsigned int j=0;j<halfTmlNum;j++)
        {
	   unsigned int server_j=terminalNum-j-1;
      	   Ipv4Address server_addr=terminalIpv4s[terminalNodeLinkedSwitchIndex[server_j]].GetAddress (terminalNodeLinkedSwitchNumber[server_j]-1);
	   InetSocketAddress dst = InetSocketAddress (server_addr);
           OnOffHelper onoff = OnOffHelper ("ns3::TcpSocketFactory", dst);
	   onoff.SetAttribute("PacketSize", UintegerValue(1024));
           onoff.SetAttribute("DataRate", StringValue("1Mbps"));
           onoff.SetAttribute("MaxBytes", StringValue("0"));

           apps = onoff.Install (terminals.Get (j));
           apps.Start (Seconds (1.0));
           apps.Stop (Seconds (100.0));

       	   PacketSinkHelper sink = PacketSinkHelper ("ns3::TcpSocketFactory", dst);
           apps = sink.Install (terminals.Get(server_j));
           apps.Start (Seconds (0.0));
           apps.Stop (Seconds (101.0));
        }
    }

    std::cout<<"terminal num: "<<terminalNum<<" switch num: "<<switchNum<<std::endl;

    Config::ConnectWithoutContext ("/NodeList/3/ApplicationList/0/$ns3::PacketSink/Rx",
        MakeCallback (&SinkRx));
    Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::V4Ping/Rtt",
        MakeCallback (&PingRtt));
    Packet::EnablePrinting ();
    NS_LOG_INFO ("Run Simulation.");
    Simulator::Run ();
    Simulator::Destroy ();
    NS_LOG_INFO ("Done.");

    unsigned long end = getTickCount();
    std::cout<<"Running time: "<<end-start<<std::endl;
    std::cout<<"terminal num: "<<terminalNum<<" switch num: "<<switchNum<<std::endl;
}
