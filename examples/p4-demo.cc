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
#include <string>

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
#include "global.h"

using namespace ns3;

unsigned int g_networkFunc;
std::string g_flowTablePath;
std::string g_p4MatchTypePath; 
unsigned int g_populateFlowTableWay;

std::string g_homePath="/home/kp/";
std::string g_ns3RootName="ns-allinone-3.26/";
std::string g_ns3SrcName="ns-3.26/";
std::string g_nfDir=g_homePath+g_ns3RootName+g_ns3SrcName+"src/ns4/test/";

unsigned int g_nsType=NS4;

NS_LOG_COMPONENT_DEFINE ("P4Example");


// set switch network function, flowtable path and flowtable match type path
// firewall router silkroad
void SetSwitchConfigInfo(std::string ftPath,std::string mtPath)
{
  g_flowTablePath=ftPath;
  g_p4MatchTypePath=mtPath;
}

int main (int argc, char *argv[]) 
{
  LogComponentEnable ("P4Example", LOG_LEVEL_LOGIC);
  LogComponentEnable ("P4NetDevice", LOG_LEVEL_LOGIC);
  CommandLine cmd;
  cmd.Parse (argc, argv);

  NS_LOG_INFO ("Create nodes.");
  NodeContainer terminals;
  terminals.Create (4);
  NodeContainer csmaSwitch;
  csmaSwitch.Create (1);

  NS_LOG_INFO ("Build Topology");
    
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
    
  NetDeviceContainer terminalDevices;
  NetDeviceContainer switchDevices;
    
  for (int i = 0; i < 4; i++)
    {
      NetDeviceContainer link = csma.Install (NodeContainer (terminals.Get (i), csmaSwitch));
      terminalDevices.Add (link.Get (0));
      switchDevices.Add (link.Get (1));
    }
  Ptr<Node> switchNode = csmaSwitch.Get (0);
  if (g_nsType==NS4) //ns4 mode
    {
      g_populateFlowTableWay=LOCAL_CALL;//LOCAL_CALL RUNTIME_CLI
      g_networkFunc=FIREWALL;
      InitSwitchConfig();

      P4Helper bridge;
      bridge.Install (switchNode, switchDevices);
    }
  else //ns3 mode
    {
      BridgeHelper bridge;
      bridge.Install (switchNode, switchDevices);
    }
  // Add internet stack to the terminals
  InternetStackHelper internet;
  internet.Install (terminals);

  // Assign IP addresses.
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer addresses = ipv4.Assign (terminalDevices);

  NS_LOG_INFO ("Create Applications.");
  NS_LOG_INFO ("Create Source");
  Config::SetDefault ("ns3::Ipv4RawSocketImpl::Protocol", StringValue ("2"));
    
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (terminals.Get (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));
  Ipv4Address dstIp;
  if(network_func.compare("silkroad")==0)
    dstIp=Ipv4Address("10.1.1.10");
  else
    dstIp=addresses.GetAddress (0);
  UdpEchoClientHelper echoClient (dstIp, 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (20));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  ApplicationContainer clientApps = echoClient.Install (terminals.Get (3));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  //NS_LOG_INFO ("Configure Tracing.");
  csma.EnablePcapAll ("p4-example", false);
  Packet::EnablePrinting ();

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}


void InitSwitchConfig()
{
  switch(g_networkFunc)
    {
    case FIREWALL:
      {
        SetSwitchConfigInfo(g_nfDir+"firewall/command.txt",g_nfDir+"firewall/mtype.txt");
        break;
      }
    case ROUTER:
      {
        SetSwitchConfigInfo(g_nfDir+"router/command.txt",g_nfDir+"router/mtype.txt");
        break;
      }
    case SILK_ROAD:
      {
        SetSwitchConfigInfo(g_nfDir+"silkroad/command.txt",g_nfDir+"silkroad/mtype.txt");
        break;
      }
    default:
      {
        break;
      }
    }
}
