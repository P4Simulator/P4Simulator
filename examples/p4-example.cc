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
//        n0     n1
//        |      |
//       ----------
//       | Switch |
//       ----------
//        |      |
//        n2     n3
//
//
// - CBR/UDP flows from n0 to n1 and from n3 to n0
// - DropTail queues

#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/p4-helper.h"
#include "ns3/v4ping-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("P4Example");

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
    terminals.Create (4);

    NodeContainer csmaSwitch;
    csmaSwitch.Create (1);

    NS_LOG_INFO ("Build Topology");
    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
    csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

    // Create the csma links, from each terminal to the switch

    NetDeviceContainer terminalDevices;
    NetDeviceContainer switchDevices;

    for (int i = 0; i < 4; i++) {
      NetDeviceContainer link = csma.Install (NodeContainer (terminals.Get (i), csmaSwitch));
      terminalDevices.Add (link.Get (0));
      switchDevices.Add (link.Get (1));
    }

    // Create the p4 netdevice, which will do the packet switching
    Ptr<Node> switchNode = csmaSwitch.Get (0);

    if (p4) {
        P4Helper bridge;
        NS_LOG_INFO("P4 bridge established");
        bridge.Install (switchNode, switchDevices);
    } else {
       BridgeHelper bridge;
       bridge.Install (switchNode, switchDevices);
    }

    // Add internet stack to the terminals
    InternetStackHelper internet;
    internet.Install (terminals);

    // We've got the "hardware" in place.  Now we need to add IP addresses.
    //
    NS_LOG_INFO ("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer addresses = ipv4.Assign (terminalDevices);

    //
    // Create an OnOff application to send UDP datagrams from node zero to node 1.
    //
    NS_LOG_INFO ("Create Applications.");

    NS_LOG_INFO ("Create Source");
    Config::SetDefault ("ns3::Ipv4RawSocketImpl::Protocol", StringValue ("2"));

    InetSocketAddress dst = InetSocketAddress (addresses.GetAddress (3));
    OnOffHelper onoff = OnOffHelper ("ns3::TcpSocketFactory", dst);
    onoff.SetConstantRate (DataRate (15000));
    onoff.SetAttribute ("PacketSize", UintegerValue (1200));

    ApplicationContainer apps = onoff.Install (terminals.Get (0));
    apps.Start (Seconds (1.0));
    apps.Stop (Seconds (10.0));

    NS_LOG_INFO ("Create Sink.");
    PacketSinkHelper sink = PacketSinkHelper ("ns3::TcpSocketFactory", dst);
    apps = sink.Install (terminals.Get (3));
    apps.Start (Seconds (0.0));
    apps.Stop (Seconds (11.0));

    NS_LOG_INFO ("Create pinger");
    V4PingHelper ping = V4PingHelper (addresses.GetAddress (2));
    NodeContainer pingers;
    pingers.Add (terminals.Get (0));
    pingers.Add (terminals.Get (1));
    pingers.Add (terminals.Get (2));
    apps = ping.Install (pingers);
    apps.Start (Seconds (2.0));
    apps.Stop (Seconds (5.0));
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
