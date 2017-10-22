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

// Network topology
//
//          n0     
//           |      
//       ----------
//       | S0     |
//       ----------
//        |      |
//        n1   ----------
//             | S1     |   
//             ----------
//                 |
//                 n2    
//
// - CBR/UDP flows from n0 to n1 and from n3 to n0
// - DropTail queues
//

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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("P4Example");

std::string networkFunc;
int switchIndex=0;
std::string flowtable_path;

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

int main (int argc, char *argv[]) {
    int p4 = 1;

    //
    // Users may find it convenient to turn on explicit debugging
    // for selected modules; the below lines suggest how to do this
    //

    LogComponentEnable ("P4Example", LOG_LEVEL_LOGIC);
    LogComponentEnable ("P4Helper", LOG_LEVEL_LOGIC);
    LogComponentEnable ("P4NetDevice", LOG_LEVEL_LOGIC);
    LogComponentEnable("BridgeNetDevice",LOG_LEVEL_LOGIC);
    LogComponentEnable("P4TopologyReader",LOG_LEVEL_LOGIC);
    LogComponentEnable("CsmaTopologyReader",LOG_LEVEL_LOGIC);
    
    // ******************TO DO *******************************************
    // may be can consider networkFunc as a parm form command line load
    // now implmented network function includes: simple firewall l2_switch router
    //networkFunc="simple"; 
    //networkFunc="firewall";
    //networkFunc="l2_switch";
    networkFunc="router";
    // *******************************************************************

    // Allow the user to override any of the defaults and the above Bind() at
    // run-time, via command-line arguments
    //
    std::string format ("CsmaTopo");
    std::string input ("/home/kp/user/ns-allinone-3.26/ns-3.26/src/ns4/examples/net_topo/csma_topo.txt");

    CommandLine cmd;
    cmd.AddValue ("format", "Format to use for data input [Orbis|Inet|Rocketfuel|CsmaNetTopo].",
                format);
    cmd.AddValue ("input", "Name of the input file.",
                input);
    cmd.Parse (argc, argv);

     // Pick a topology reader based in the requested format.
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

    //
    // Explicitly create the nodes required by the topology (shown above).
    //
    NS_LOG_INFO ("Create nodes.");
    NodeContainer terminals=inFile->GetTerminalNodeContainer();
    NodeContainer csmaSwitch=inFile->GetSwitchNodeContainer();
    unsigned int terminalNum=terminals.GetN();
    unsigned int switchNum=csmaSwitch.GetN();
    //int linkNum=inFile->LinksSize();
    NS_LOG_LOGIC("switchNum:"<<switchNum);


    NS_LOG_INFO ("Build Topology");
    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
    csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

    // Create the csma links, from each terminal to the switch

    NetDeviceContainer* terminalDevices=new NetDeviceContainer[switchNum];// every switch linked terminal, at the end of terminal
    Ipv4InterfaceContainer* terminalIpv4s=new Ipv4InterfaceContainer[switchNum];
    NetDeviceContainer* switchDevices=new NetDeviceContainer[switchNum];// every switch linked device, at the end of switch
    std::vector<std::string> *switchPortInfo=new std::vector<std::string>[switchNum];// every switch linked port info (0s,0t)
    unsigned int* terminalNodeLinkedSwitchIndex=new unsigned int[terminalNum];
    unsigned int* terminalNodeLinkedSwitchPort=new unsigned int[terminalNum];
    unsigned int* terminalNodeLinkedSwitchNumber=new unsigned int[terminalNum];

   
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
                    switchDevices[iter->GetFromLinkedSwitchIndex()].Add(link.Get(0));
                    switchPortInfo[iter->GetFromLinkedSwitchIndex()].push_back("s"+int_to_str(iter->GetToLinkedSwitchIndex()));

                    switchDevices[iter->GetToLinkedSwitchIndex()].Add(link.Get(1));
                    switchPortInfo[iter->GetToLinkedSwitchIndex()].push_back("s"+int_to_str(iter->GetFromLinkedSwitchIndex()));
                }
                else
                {
                    NS_LOG_LOGIC("link error!");
                    abort();
                }
            }
         }
    }
    for(size_t k=0;k<terminalNum;k++)
        std::cout<<terminalNodeLinkedSwitchIndex[k]<<" "<<terminalNodeLinkedSwitchPort[k]<<" "<<terminalNodeLinkedSwitchNumber[k]<<std::endl;
    for(size_t k=0;k<switchNum;k++)
    {
        for(std::vector<std::string>::iterator iter=switchPortInfo[k].begin();iter!=switchPortInfo[k].end();iter++)
        {
            std::cout<<*iter<<" ";
        }
        std::cout<<std::endl;
    }

    // Create the p4 netdevice, which will do the packet switching
    p4=0;
    if (p4) {
        std::string flowtable_parentdir("/home/kp/user/ns-allinone-3.26/ns-3.26/src/ns4/test/flowtable_dir/");
        std::string childdir("router_flowtable/");
        for(unsigned int k=0;k<switchNum;k++)
        {
            std::string flowtable_name("command1.txt");// can read from a file
            networkFunc="router";
            P4Helper bridge;
            NS_LOG_INFO("P4 bridge established");
            flowtable_path=flowtable_parentdir+childdir+flowtable_name; 
            bridge.Install (csmaSwitch.Get(k), switchDevices[k]);
        }
    } else {
       BridgeHelper bridge;
       for(unsigned int k=0;k<switchNum;k++)
       {
         bridge.Install (csmaSwitch.Get(k), switchDevices[k]);
       }
    }



    // Add internet stack to the terminals
    InternetStackHelper internet;
    internet.Install (terminals);

    // We've got the "hardware" in place.  Now we need to add IP addresses.
    //
    NS_LOG_INFO ("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    for(unsigned int k=0;k<switchNum;k++)
    {
        if(terminalDevices[k].GetN()>0)
        {
            terminalIpv4s[k]=ipv4.Assign(terminalDevices[k]);
        }
        //output ipv4 addr
        std::cout<<"s"<<k<<": ";
        for(size_t j=0;j<terminalIpv4s[k].GetN();j++)
            std::cout<<terminalIpv4s[k].GetAddress(j)<<" ";
        std::cout<<std::endl;

        ipv4.NewNetwork ();
    }
    // random select a node as server
    Ptr<UniformRandomVariable> unifRandom = CreateObject<UniformRandomVariable> ();
    unifRandom->SetAttribute ("Min", DoubleValue (0));
    unifRandom->SetAttribute ("Max", DoubleValue (terminalNum- 1));
    unsigned int randomTerminalNumber = unifRandom->GetInteger (0, terminalNum - 1);
    NS_LOG_LOGIC("randomTerminalNumber:"<<randomTerminalNumber);

    //
    // Create an OnOff application to send UDP datagrams from node zero to node 1.
    //
    NS_LOG_INFO ("Create Applications.");

    NS_LOG_INFO ("Create Source");
    Config::SetDefault ("ns3::Ipv4RawSocketImpl::Protocol", StringValue ("2"));
    
    // no -> n2
    Ipv4Address serverAddr=terminalIpv4s[terminalNodeLinkedSwitchIndex[randomTerminalNumber]].GetAddress (terminalNodeLinkedSwitchNumber[randomTerminalNumber]-1);
    //Ipv4Address serverAddr=terminalIpv4s[3].GetAddress(0);
    NS_LOG_LOGIC("server addr: "<<serverAddr);
    InetSocketAddress dst = InetSocketAddress (serverAddr);

    OnOffHelper onoff = OnOffHelper ("ns3::TcpSocketFactory", dst);
    onoff.SetConstantRate (DataRate (15000));
    onoff.SetAttribute ("PacketSize", UintegerValue (1200));

    ApplicationContainer apps;
    for(unsigned int j=0;j<terminalNum;j++)
    {
        if(j!=randomTerminalNumber)
        {
            apps = onoff.Install (terminals.Get (j));
            apps.Start (Seconds (1.0));
            apps.Stop (Seconds (10.0));
        }
    }
    // n2 responce n0
    NS_LOG_INFO ("Create Sink.");
    PacketSinkHelper sink = PacketSinkHelper ("ns3::TcpSocketFactory", dst);
    apps = sink.Install (terminals.Get(randomTerminalNumber));
    apps.Start (Seconds (0.0));
    apps.Stop (Seconds (11.0));

    /*if(networkFunc.compare("simple")==0){

      NS_LOG_INFO ("Create pinger");
      V4PingHelper ping = V4PingHelper (addresses.GetAddress (2));
      NodeContainer pingers;
      pingers.Add (terminals.Get (0));
      pingers.Add (terminals.Get (1));
      pingers.Add (terminals.Get (2));
      apps = ping.Install (pingers);
      apps.Start (Seconds (2.0));
      apps.Stop (Seconds (5.0));

    }*/

    NS_LOG_INFO ("Configure Tracing.");

    // first, pcap tracing in non-promiscuous mode
    csma.EnablePcapAll ("p4-example", false);
    // then, print what the packet sink receives.
    Config::ConnectWithoutContext ("/NodeList/3/ApplicationList/0/$ns3::PacketSink/Rx",
        MakeCallback (&SinkRx));
    // finally, print the ping rtts.
    Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::V4Ping/Rtt",
        MakeCallback (&PingRtt));
    Packet::EnablePrinting ();

    NS_LOG_INFO ("Run Simulation.");
    Simulator::Run ();
    Simulator::Destroy ();
    NS_LOG_INFO ("Done.");
}
