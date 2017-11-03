#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "ns3/flow-monitor-module.h"
#include "ns3/bridge-helper.h"
#include "ns3/bridge-net-device.h"
//#include "ns3/slb-helper.h"
//#include "ns3/slb-net-device.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/csma-module.h"
#include "ns3/ipv4-nix-vector-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SLBExample");

static void SinkRx (Ptr<const Packet> p, const Address &ad) {
    std::cout << "Rx" << "Received from  "<< ad << std::endl;
}

int main() {
    LogComponentEnable("SLBExample",LOG_LEVEL_LOGIC);
//    LogComponentEnable("SLBNetDevice",LOG_LEVEL_LOGIC);
    LogComponentEnable("BridgeNetDevice",LOG_LEVEL_LOGIC);
    LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
    LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);

    NS_LOG_INFO ("Create nodes.");
    NodeContainer hosts;
    NodeContainer switchs;

    hosts.Create(2);
    switchs.Create(3);

    InternetStackHelper internet;
    Ipv4NixVectorHelper nixRouting;
    Ipv4StaticRoutingHelper staticRouting;
    Ipv4ListRoutingHelper list;
    list.Add (staticRouting, 0);
    list.Add (nixRouting, 10);
    internet.SetRoutingHelper(list);

    internet.Install (hosts);
    internet.Install (switchs);

    NS_LOG_INFO ("Build Topology.");
    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
    csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

    NetDeviceContainer s0, s1, s2;
    NetDeviceContainer h0, h1;

    NetDeviceContainer link0 = csma.Install(NodeContainer(switchs.Get(0), switchs.Get(1)));
    NetDeviceContainer link2 = csma.Install(NodeContainer(switchs.Get(0), switchs.Get(2)));
    NetDeviceContainer link3 = csma.Install (NodeContainer(switchs.Get(0), hosts.Get(0)));
    NetDeviceContainer link4 = csma.Install (NodeContainer(switchs.Get(1), hosts.Get(1)));

    s0.Add(link0.Get(0));
    s0.Add(link2.Get(0));
    s0.Add(link3.Get(0));

    s1.Add(link0.Get(1));
    s1.Add(link4.Get(0));
    s2.Add(link2.Get(1));

    h0.Add(link3.Get(1));
    h1.Add(link4.Get(1));

    BridgeHelper bridge;
    bridge.Install(switchs.Get(0), s0);
    bridge.Install(switchs.Get(1), s1);
  //  SLBHelper slbHelper;
    bridge.Install(switchs.Get(2), s2);

    NS_LOG_INFO ("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.0.1.0", "255.255.255.0");
    ipv4.Assign(s0);
    ipv4.SetBase ("10.0.1.0", "255.255.255.0", "0.0.0.20");
    Ipv4InterfaceContainer addr0 = ipv4.Assign(h0);

    ipv4.SetBase ("10.0.1.0", "255.255.255.0", "0.0.0.30");
    ipv4.Assign (s1);
    ipv4.SetBase ("10.0.1.0", "255.255.255.0", "0.0.0.60");
    Ipv4InterfaceContainer addr1 = ipv4.Assign (h1);

    ipv4.SetBase ("10.0.1.0", "255.255.255.0", "0.0.0.50");
    ipv4.Assign (s2);

    // std::cout << std::hex << tempadd.GetAddress(0) << '\n';

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>("tables.routes", std::ios::out);
    Ipv4GlobalRoutingHelper::PrintRoutingTableAllAt(Seconds(0), routingStream);

    // // ====================Simulation Begin====================
    NS_LOG_INFO ("Run Simulation.");

    NS_LOG_INFO ("Create Source");
    InetSocketAddress slb = InetSocketAddress(Ipv4Address("10.0.1.60"), 11);
    OnOffHelper oo = OnOffHelper("ns3::TcpSocketFactory", slb);
    int packetSize = 1024;      // 1024 bytes
    char dataRate_OnOff [] = "1Mbps";
    char maxBytes [] = "0";     // unlimited
    oo.SetAttribute("PacketSize", UintegerValue(packetSize));
    oo.SetAttribute("DataRate", StringValue(dataRate_OnOff));
    oo.SetAttribute("MaxBytes", StringValue(maxBytes));
    ApplicationContainer onOffApp = oo.Install(hosts.Get(0));
    onOffApp.Start(Seconds(2.0));
    onOffApp.Stop(Seconds(10.0));

    NS_LOG_INFO ("Create Sink.");
    InetSocketAddress dst = InetSocketAddress(Ipv4Address("10.0.1.60"));
    PacketSinkHelper sink = PacketSinkHelper("ns3::TcpSocketFactory", dst);
    ApplicationContainer sinkApp = sink.Install(hosts.Get(1));
    sinkApp.Start(Seconds(1.0));
    sinkApp.Stop(Seconds(11.0));

    Packet::EnablePrinting ();

    Config::ConnectWithoutContext ("/NodeList/3/ApplicationList/0/$ns3::PacketSink/Rx",
        MakeCallback (&SinkRx));

    Simulator::Run ();
    Simulator::Destroy ();
}
