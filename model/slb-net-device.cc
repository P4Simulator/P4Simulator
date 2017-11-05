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
 * Author: Gustavo Carneiro  <gjc@inescporto.pt>
 */
#include "slb-net-device.h"
#include "ns3/node.h"
#include "ns3/channel.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstring>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SLBNetDevice");

NS_OBJECT_ENSURE_REGISTERED (SLBNetDevice);

TypeId
SLBNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SLBNetDevice")
    .SetParent<NetDevice> ()
    .SetGroupName("SLB")
    .AddConstructor<SLBNetDevice> ()
    .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
                   UintegerValue (1500),
                   MakeUintegerAccessor (&SLBNetDevice::SetMtu,
                                         &SLBNetDevice::GetMtu),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("EnableLearning",
                   "Enable the learning mode of the Learning Bridge",
                   BooleanValue (true),
                   MakeBooleanAccessor (&SLBNetDevice::m_enableLearning),
                   MakeBooleanChecker ())
    .AddAttribute ("ExpirationTime",
                   "Time it takes for learned MAC state entry to expire.",
                   TimeValue (Seconds (300)),
                   MakeTimeAccessor (&SLBNetDevice::m_expirationTime),
                   MakeTimeChecker ())
  ;
  return tid;
}


SLBNetDevice::SLBNetDevice () : BridgeNetDevice()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_channel = CreateObject<BridgeChannel> ();
}

SLBNetDevice::~SLBNetDevice()
{
  NS_LOG_FUNCTION_NOARGS ();
}

// void
// SLBNetDevice::DoDispose ()
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   for (std::vector< Ptr<NetDevice> >::iterator iter = m_ports.begin (); iter != m_ports.end (); iter++)
//     {
//       *iter = 0;
//     }
//   m_ports.clear ();
//   m_channel = 0;
//   m_node = 0;
//   NetDevice::DoDispose ();
// }

void
SLBNetDevice::ReceiveFromDevice (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet, uint16_t protocol,
                                    Address const &src, Address const &dst, PacketType packetType) {
    NS_LOG_FUNCTION_NOARGS ();
    NS_LOG_DEBUG ("UID is " << packet->GetUid ());
    packet->EnablePrinting();
    packet->Print(std::cout);
    std::cout<<std::endl;
    Mac48Address src48 = Mac48Address::ConvertFrom (src);
    Mac48Address dst48 = Mac48Address::ConvertFrom (dst);

    // std::cout << "============SLB Receive===========\n";
    // // std::cout << "Receive - "  << GetAddress() << '\n';
    // packet->Print(std::cout);
    // std::cout.flush();
    // std::cout << "\n--------------------------\n";

    int bufferSize = packet->GetSize();
    uint8_t* buffer = new uint8_t [bufferSize];
    packet->CopyData(buffer, bufferSize);
    for (int i = 0; i < bufferSize; i ++) std::cout << std::setfill('0') << std::setw(2) << std::hex << (int)buffer[i] << std::dec << ' ';
    std::cout << '\n';
    std::cout<<buffer[14]<<buffer[15]<<buffer[16]<<buffer[17]<<std::setw(2);
    std::cout<<buffer[24]<<buffer[25]<<buffer[26]<<buffer[27]<<std::setw(2);


    int srcIP = (int)buffer[14]*256*256*256 + (int)buffer[15]*256*256 + (int)buffer[16]*256 + (int)buffer[17];
    int dstIP = (int)buffer[24]*256*256*256 + (int)buffer[25]*256*256 + (int)buffer[26]*256 + (int)buffer[27];
    std::cout<<srcIP<<" "<<dstIP<<std::endl;
    buffer[27]=96;
    bool isTcp = false;
    PacketMetadata::ItemIterator i = packet->BeginItem();
    while (i.HasNext ()) {
        PacketMetadata::Item item = i.Next ();
        switch (item.type) {
            case PacketMetadata::Item::HEADER:
                isTcp = (item.tid.GetName() == "ns3::TcpHeader");
                break;
            default:
            break;
        }
        if (isTcp) break;
    }

    if (protocol == 0x0800 && isTcp) {
        // std::cout << "============Receive IPv4===========\n";
        // std::cout << "src addr - " << src48 << '\n';
        // std::cout << "dst addr - " << dst48 << '\n';
        // std::cout << "this addr - " << GetAddress() << '\n';
        // int inPort = (int)buffer[20]*256 + (int)buffer[21];
        // int outPort = (int)buffer[22]*256 + (int)buffer[23];
         int srcIP = (int)buffer[12]*256*256*256 + (int)buffer[13]*256*256 + (int)buffer[14]*256 + (int)buffer[15];
         int dstIP = (int)buffer[16]*256*256*256 + (int)buffer[17]*256*256 + (int)buffer[18]*256 + (int)buffer[19];
        // 10.0.1.x
	std::cout<<srcIP<<" "<<dstIP<<std::endl;
        buffer[19] = 60;
        Ptr<Packet> pkt = new ns3::Packet((uint8_t*)buffer, bufferSize);
        uint8_t* dstBuffer = new uint8_t[6];
        dstBuffer[0] = 0;dstBuffer[1] = 0;dstBuffer[2] = 0;dstBuffer[3] = 0;dstBuffer[4] = 0;dstBuffer[5] = 8;
        Address dest(2, dstBuffer, 6);
        std::cout << "============Send Back===========\n";
        incomingPort->SendFrom(pkt->Copy(), src, dest, protocol);
        return;
    }

    if (!m_promiscRxCallback.IsNull ()) {
        m_promiscRxCallback (this, packet, protocol, src, dst, packetType);
    }

    switch (packetType)
    {
    case PACKET_HOST:
        if (dst48 == m_address) {
            m_rxCallback (this, packet, protocol, src);
        }
        break;

    case PACKET_BROADCAST:
    case PACKET_MULTICAST:
        m_rxCallback (this, packet, protocol, src);
        ForwardBroadcast (incomingPort, packet, protocol, src48, dst48);
        break;

    case PACKET_OTHERHOST:
        if (dst48 == m_address) {
            m_rxCallback (this, packet, protocol, src);
        } else {
          ForwardUnicast (incomingPort, packet, protocol, src48, dst48);
        }
        break;
    }
}

// void
// SLBNetDevice::ForwardUnicast (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
//                                  uint16_t protocol, Mac48Address src, Mac48Address dst)
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   NS_LOG_DEBUG ("LearningBridgeForward (incomingPort=" << incomingPort->GetInstanceTypeId ().GetName ()
//                                                        << ", packet=" << packet << ", protocol="<<protocol
//                                                        << ", src=" << src << ", dst=" << dst << ")");

//   Learn (src, incomingPort);
//   Ptr<NetDevice> outPort = GetLearnedState (dst);
//   if (outPort != NULL && outPort != incomingPort)
//     {
//       NS_LOG_LOGIC ("Learning bridge state says to use port `" << outPort->GetInstanceTypeId ().GetName () << "'");
//       outPort->SendFrom (packet->Copy (), src, dst, protocol);
//     }
//   else
//     {
//       NS_LOG_LOGIC ("No learned state: send through all ports");
//       for (std::vector< Ptr<NetDevice> >::iterator iter = m_ports.begin ();
//            iter != m_ports.end (); iter++)
//         {
//           Ptr<NetDevice> port = *iter;
//           if (port != incomingPort)
//             {
//               NS_LOG_LOGIC ("LearningBridgeForward (" << src << " => " << dst << "): "
//                                                       << incomingPort->GetInstanceTypeId ().GetName ()
//                                                       << " --> " << port->GetInstanceTypeId ().GetName ()
//                                                       << " (UID " << packet->GetUid () << ").");
//               port->SendFrom (packet->Copy (), src, dst, protocol);
//             }
//         }
//     }
// }

// void
// SLBNetDevice::ForwardBroadcast (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
//                                    uint16_t protocol, Mac48Address src, Mac48Address dst)
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   NS_LOG_DEBUG ("LearningBridgeForward (incomingPort=" << incomingPort->GetInstanceTypeId ().GetName ()
//                                                        << ", packet=" << packet << ", protocol="<<protocol
//                                                        << ", src=" << src << ", dst=" << dst << ")");
//   Learn (src, incomingPort);

//   for (std::vector< Ptr<NetDevice> >::iterator iter = m_ports.begin ();
//        iter != m_ports.end (); iter++)
//     {
//       Ptr<NetDevice> port = *iter;
//       if (port != incomingPort)
//         {
//           NS_LOG_LOGIC ("LearningBridgeForward (" << src << " => " << dst << "): "
//                                                   << incomingPort->GetInstanceTypeId ().GetName ()
//                                                   << " --> " << port->GetInstanceTypeId ().GetName ()
//                                                   << " (UID " << packet->GetUid () << ").");
//           port->SendFrom (packet->Copy (), src, dst, protocol);
//         }
//     }
// }

// void SLBNetDevice::Learn (Mac48Address source, Ptr<NetDevice> port)
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   if (m_enableLearning)
//     {
//       LearnedState &state = m_learnState[source];
//       state.associatedPort = port;
//       state.expirationTime = Simulator::Now () + m_expirationTime;
//     }
// }

// Ptr<NetDevice> SLBNetDevice::GetLearnedState (Mac48Address source)
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   if (m_enableLearning)
//     {
//       Time now = Simulator::Now ();
//       std::map<Mac48Address, LearnedState>::iterator iter =
//         m_learnState.find (source);
//       if (iter != m_learnState.end ())
//         {
//           LearnedState &state = iter->second;
//           if (state.expirationTime > now)
//             {
//               return state.associatedPort;
//             }
//           else
//             {
//               m_learnState.erase (iter);
//             }
//         }
//     }
//   return NULL;
// }

// uint32_t
// SLBNetDevice::GetNBridgePorts (void) const
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   return m_ports.size ();
// }


// Ptr<NetDevice>
// SLBNetDevice::GetBridgePort (uint32_t n) const
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   return m_ports[n];
// }

void
SLBNetDevice::AddBridgePort (Ptr<NetDevice> bridgePort)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (bridgePort != this);
  if (!Mac48Address::IsMatchingType (bridgePort->GetAddress ()))
    {
      NS_FATAL_ERROR ("Device does not support eui 48 addresses: cannot be added to bridge.");
    }
  if (!bridgePort->SupportsSendFrom ())
    {
      NS_FATAL_ERROR ("Device does not support SendFrom: cannot be added to bridge.");
    }
  if (m_address == Mac48Address ())
    {
      m_address = Mac48Address::ConvertFrom (bridgePort->GetAddress ());
    }

  NS_LOG_DEBUG ("RegisterProtocolHandler for " << bridgePort->GetInstanceTypeId ().GetName ());
  m_node->RegisterProtocolHandler (MakeCallback (&SLBNetDevice::ReceiveFromDevice, this),
                                   0, bridgePort, true);
  m_ports.push_back (bridgePort);
  m_channel->AddChannel (bridgePort->GetChannel ());
}

// void
// SLBNetDevice::SetIfIndex (const uint32_t index)
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   m_ifIndex = index;
// }

// uint32_t
// SLBNetDevice::GetIfIndex (void) const
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   return m_ifIndex;
// }

// Ptr<Channel>
// SLBNetDevice::GetChannel (void) const
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   return m_channel;
// }

// void
// SLBNetDevice::SetAddress (Address address)
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   m_address = Mac48Address::ConvertFrom (address);
// }

// Address
// SLBNetDevice::GetAddress (void) const
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   return m_address;
// }

// bool
// SLBNetDevice::SetMtu (const uint16_t mtu)
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   m_mtu = mtu;
//   return true;
// }

// uint16_t
// SLBNetDevice::GetMtu (void) const
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   return m_mtu;
// }


// bool
// SLBNetDevice::IsLinkUp (void) const
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   return true;
// }


// void
// SLBNetDevice::AddLinkChangeCallback (Callback<void> callback)
// {}


// bool
// SLBNetDevice::IsBroadcast (void) const
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   return true;
// }


// Address
// SLBNetDevice::GetBroadcast (void) const
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   return Mac48Address ("ff:ff:ff:ff:ff:ff");
// }

// bool
// SLBNetDevice::IsMulticast (void) const
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   return true;
// }

// Address
// SLBNetDevice::GetMulticast (Ipv4Address multicastGroup) const
// {
//   NS_LOG_FUNCTION (this << multicastGroup);
//   Mac48Address multicast = Mac48Address::GetMulticast (multicastGroup);
//   return multicast;
// }


// bool
// SLBNetDevice::IsPointToPoint (void) const
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   return false;
// }

// bool
// SLBNetDevice::IsBridge (void) const
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   return true;
// }


// bool
// SLBNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   return SendFrom (packet, m_address, dest, protocolNumber);
// }

// bool
// SLBNetDevice::SendFrom (Ptr<Packet> packet, const Address& src, const Address& dest, uint16_t protocolNumber)
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   Mac48Address dst = Mac48Address::ConvertFrom (dest);

//   // try to use the learned state if data is unicast
//   if (!dst.IsGroup ())
//     {
//       Ptr<NetDevice> outPort = GetLearnedState (dst);
//       if (outPort != NULL)
//         {
//           outPort->SendFrom (packet, src, dest, protocolNumber);
//           return true;
//         }
//     }

//   // data was not unicast or no state has been learned for that mac
//   // address => flood through all ports.
//   Ptr<Packet> pktCopy;
//   for (std::vector< Ptr<NetDevice> >::iterator iter = m_ports.begin ();
//        iter != m_ports.end (); iter++)
//     {
//       pktCopy = packet->Copy ();
//       Ptr<NetDevice> port = *iter;
//       port->SendFrom (pktCopy, src, dest, protocolNumber);
//     }

//   return true;
// }


// Ptr<Node>
// SLBNetDevice::GetNode (void) const
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   return m_node;
// }


// void
// SLBNetDevice::SetNode (Ptr<Node> node)
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   m_node = node;
// }


// bool
// SLBNetDevice::NeedsArp (void) const
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   return true;
// }


// void
// SLBNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   m_rxCallback = cb;
// }

// void
// SLBNetDevice::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb)
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   m_promiscRxCallback = cb;
// }

// bool
// SLBNetDevice::SupportsSendFrom () const
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   return true;
// }

// Address SLBNetDevice::GetMulticast (Ipv6Address addr) const
// {
//   NS_LOG_FUNCTION (this << addr);
//   return Mac48Address::GetMulticast (addr);
// }

} // namespace ns3
