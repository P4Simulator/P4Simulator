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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("P4Example");

std::string networkFunc;
int switchIndex=0;

static void SinkRx (Ptr<const Packet> p, const Address &ad) {
    std::cout << "Rx" << "Received from  "<< ad << std::endl;
}

static void PingRtt (std::string context, Time rtt) {
    std::cout << "Rtt" << context << " " << rtt << std::endl;
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
    
    // ******************TO DO *******************************************
    // may be can consider networkFunc as a parm form command line load
    // now implmented network function includes: simple firewall l2_switch router
    //networkFunc="simple";
    //networkFunc="firewall";
    //networkFunc="l2_switch";
    networkFunc="router";
    // *******************************************************************

    // LogComponentEnable ("Buffer", LOG_LEVEL_LOGIC);
    // LogComponentEnable ("Packet", LOG_LEVEL_LOGIC);
    // LogComponentEnable ("CsmaNetDevice", LOG_LEVEL_FUNCTION);

    // Allow the user to override any of the defaults and the above Bind() at
    // run-time, via command-line arguments
    //
    CommandLine cmd;
    cmd.Parse (argc, argv);

    //
    // Explicitly create the nodes required by the topology (shown above).
    //
    NS_LOG_INFO ("Create nodes.");
    NodeContainer terminals;
    terminals.Create (3);

    NodeContainer csmaSwitch;
    csmaSwitch.Create (2);

    NS_LOG_INFO ("Build Topology");
    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
    csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

    // Create the csma links, from each terminal to the switch

    NetDeviceContainer terminalDevice1,terminalDevice2;
    NetDeviceContainer switchDevice1,switchDevice2;
    // terminalDevice1(n0,n1),terminalDevice2(n2);
    // switchDevice1(s0),switchDevice2(s1);

    for (int i = 0; i < 2; i++) {
      NetDeviceContainer link = csma.Install (NodeContainer (terminals.Get (i), csmaSwitch.Get(0)));
      terminalDevice1.Add (link.Get (0));
      switchDevice1.Add (link.Get (1));
    }

    NetDeviceContainer link1=csma.Install(NodeContainer(csmaSwitch.Get(0),csmaSwitch.Get(1)));
    switchDevice1.Add(link1.Get(0));
    switchDevice2.Add(link1.Get(1));

    NetDeviceContainer link2=csma.Install(NodeContainer(terminals.Get(2),csmaSwitch.Get(1)));
    terminalDevice2.Add(link2.Get(0));
    switchDevice2.Add(link2.Get(1));



    // Create the p4 netdevice, which will do the packet switching
    Ptr<Node> switchNode0 = csmaSwitch.Get (0);
    Ptr<Node> switchNode1 = csmaSwitch.Get (1);
    //p4=0;
    if (p4) {
        P4Helper bridge;
        NS_LOG_INFO("P4 bridge established");
        bridge.Install (switchNode0, switchDevice1);
        switchIndex++;// to decide thrift_port
        bridge.Install (switchNode1,switchDevice2);
    } else {
       BridgeHelper bridge;
       bridge.Install (switchNode0, switchDevice1);
       bridge.Install(switchNode1,switchDevice2);
    }



    // Add internet stack to the terminals
    InternetStackHelper internet;
    internet.Install (terminals);

    // We've got the "hardware" in place.  Now we need to add IP addresses.
    //
    NS_LOG_INFO ("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer addresses1 = ipv4.Assign (terminalDevice1);
    //ipv4.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer addresses2 = ipv4.Assign (terminalDevice2);


    //
    // Create an OnOff application to send UDP datagrams from node zero to node 1.
    //
    NS_LOG_INFO ("Create Applications.");

    NS_LOG_INFO ("Create Source");
    Config::SetDefault ("ns3::Ipv4RawSocketImpl::Protocol", StringValue ("2"));
    
    // no -> n2
    InetSocketAddress dst = InetSocketAddress (addresses2.GetAddress (0));
    NS_LOG_LOGIC("n2 addr: "<<dst);
    OnOffHelper onoff = OnOffHelper ("ns3::TcpSocketFactory", dst);
    onoff.SetConstantRate (DataRate (15000));
    onoff.SetAttribute ("PacketSize", UintegerValue (1200));

    ApplicationContainer apps = onoff.Install (terminals.Get (0));
    apps.Start (Seconds (1.0));
    apps.Stop (Seconds (10.0));
    
    // n2 responce n0
    NS_LOG_INFO ("Create Sink.");
    PacketSinkHelper sink = PacketSinkHelper ("ns3::TcpSocketFactory", dst);
    apps = sink.Install (terminals.Get (2));
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
