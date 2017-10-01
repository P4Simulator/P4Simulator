/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "p4-helper.h"
#include "ns3/log.h"
#include "ns3/p4-net-device.h"
// #include "ns3/p4-module.h"
#include "ns3/node.h"
#include "ns3/names.h"
#include "ns3/net-device-container.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("P4Helper");

P4Helper::P4Helper () {
    NS_LOG_FUNCTION_NOARGS ();
    m_deviceFactory.SetTypeId ("ns3::P4NetDevice");
}

void P4Helper::SetDeviceAttribute (std::string n1, const AttributeValue &v1) {
    NS_LOG_FUNCTION_NOARGS ();
    m_deviceFactory.Set (n1, v1);
}

NetDeviceContainer
P4Helper::Install (Ptr<Node> node, NetDeviceContainer c) {
    NS_LOG_FUNCTION_NOARGS ();
    NS_LOG_LOGIC ("**** Install bridge device on node " << node->GetId ());

    NetDeviceContainer devs;
    Ptr<P4NetDevice> dev = m_deviceFactory.Create<P4NetDevice> ();
    devs.Add (dev);
    node->AddDevice (dev);

    for (NetDeviceContainer::Iterator i = c.Begin (); i != c.End (); ++i) {
        NS_LOG_LOGIC ("**** Add BridgePort "<< *i);
        dev->AddBridgePort (*i);
    }
    return devs;
}

NetDeviceContainer
P4Helper::Install (std::string nodeName, NetDeviceContainer c) {
    NS_LOG_FUNCTION_NOARGS ();
    Ptr<Node> node = Names::Find<Node> (nodeName);
    return Install (node, c);
}

}
