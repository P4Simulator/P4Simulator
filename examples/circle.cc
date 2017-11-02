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
//
// - CBR/UDP flows from n0 to n1 and from n3 to n0
// - DropTail queues
//

#include <iostream>
#include <fstream>
#include <string>

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
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>


using namespace ns3;
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


NS_LOG_COMPONENT_DEFINE ("P4Example");


int main (int argc, char *argv[]) {
    
    unsigned long start = getTickCount();

    // Users may find it convenient to turn on explicit debugging
    // for selected modules; the below lines suggest how to do this

    LogComponentEnable ("P4Example", LOG_LEVEL_LOGIC);
    LogComponentEnable("BridgeNetDevice",LOG_LEVEL_LOGIC);
    
    
    // Allow the user to override any of the defaults and the above Bind() at
    // run-time, via command-line arguments
    CommandLine cmd;
    cmd.Parse (argc, argv);

    // Explicitly create the nodes required by the topology (shown above).
    NS_LOG_INFO ("Create nodes.");
    NodeContainer terminals;
    terminals.Create (2);

    NodeContainer csmaSwitch;
    csmaSwitch.Create (3);

    NS_LOG_INFO ("Build Topology");
    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
    csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

    // Create the csma links, from each terminal to the switch

    NetDeviceContainer terminalDevice;
    NetDeviceContainer switchDevice0,switchDevice1,switchDevice2;

    NetDeviceContainer link0 = csma.Install (NodeContainer (csmaSwitch.Get (0), csmaSwitch.Get(1)));
    NetDeviceContainer link1 = csma.Install (NodeContainer (csmaSwitch.Get (0), csmaSwitch.Get(2)));
    NetDeviceContainer link2 = csma.Install (NodeContainer (csmaSwitch.Get (1), csmaSwitch.Get(2)));
    NetDeviceContainer link3 = csma.Install (NodeContainer (csmaSwitch.Get (1), terminals.Get(0)));
    NetDeviceContainer link4 = csma.Install (NodeContainer (csmaSwitch.Get (2), terminals.Get(1)));
    
    terminalDevice.Add(link3.Get(1));
    terminalDevice.Add(link4.Get(1));
    switchDevice0.Add(link0.Get(0));
    switchDevice0.Add(link1.Get(0));
    switchDevice1.Add(link0.Get(1));
    switchDevice1.Add(link2.Get(0));
    switchDevice1.Add(link3.Get(0));
    switchDevice2.Add(link4.Get(0));
    switchDevice2.Add(link1.Get(1));
    switchDevice2.Add(link2.Get(1));


    // Create the p4 netdevice, which will do the packet switching
     BridgeHelper bridge;
     bridge.Install (csmaSwitch.Get(0), switchDevice0);
     bridge.Install (csmaSwitch.Get(1), switchDevice1);
     bridge.Install (csmaSwitch.Get(2), switchDevice2);


    // Add internet stack to the terminals
    InternetStackHelper internet;
    Ipv4NixVectorHelper nixRouting;
    Ipv4StaticRoutingHelper staticRouting;
    Ipv4ListRoutingHelper list;
    list.Add(staticRouting, 0);
    list.Add(nixRouting, 10);
    internet.SetRoutingHelper(list);
    internet.Install (terminals);
    internet.Install (csmaSwitch);
    // We've got the "hardware" in place.  Now we need to add IP addresses.
    //
    NS_LOG_INFO ("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer addresses = ipv4.Assign (terminalDevice);
    ipv4.NewNetwork ();
    ipv4.Assign(switchDevice0);
    
        ipv4.NewNetwork ();
    ipv4.Assign(switchDevice1);
    
        ipv4.NewNetwork ();
    ipv4.Assign(switchDevice2);
    //Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    //
    // Create an OnOff application to send UDP datagrams from node zero to node 1.
    //
    NS_LOG_INFO ("Create Applications.");

    NS_LOG_INFO ("Create Source");
    Config::SetDefault ("ns3::Ipv4RawSocketImpl::Protocol", StringValue ("2"));

    UdpEchoServerHelper echoServer (9);

    ApplicationContainer serverApps = echoServer.Install (terminals.Get (0));
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (10.0));

    UdpEchoClientHelper echoClient (addresses.GetAddress (0), 9);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps = echoClient.Install (terminals.Get (1));
    clientApps.Start (Seconds (2.0));
    clientApps.Stop (Seconds (10.0));
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
     Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("dynamic-global-routing.routes", std::ios::out);    Ipv4GlobalRoutingHelper::PrintRoutingTableAllAt (Seconds (0), routingStream);
    NS_LOG_INFO ("Configure Tracing.");

    // first, pcap tracing in non-promiscuous mode
    csma.EnablePcapAll ("p4-example", false);
    Packet::EnablePrinting ();

    NS_LOG_INFO ("Run Simulation.");
    Simulator::Run ();
    Simulator::Destroy ();
    NS_LOG_INFO ("Done.");
    unsigned long end = getTickCount();
    NS_LOG_INFO("Running time: "<<end-start);

}
