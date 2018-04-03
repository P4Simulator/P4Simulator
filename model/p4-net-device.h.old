/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) YEAR COPYRIGHTHOLDER
 *
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
 * Author: 
 */

#ifndef P4_NET_DEVICE_H
#define P4_NET_DEVICE_H
#include <ns3/bridge-net-device.h>
#include <ns3/net-device.h>
#include <ns3/ptr.h>
#include <ns3/mac48-address.h>
#include "ns3/log.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/bridge-channel.h"
#include "ns3/node.h"
#include "ns3/enum.h"
#include "ns3/string.h"
#include "ns3/integer.h"
#include "ns3/uinteger.h"
#include <bm/bm_sim/queue.h>
#include <bm/bm_sim/queueing.h>
#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/switch.h>
#include <bm/bm_sim/event_logger.h>
#include <bm/bm_sim/simple_pre_lag.h>
#include <bm/bm_sim/parser.h>
#include <bm/bm_sim/tables.h>
#include <bm/bm_sim/logger.h>
#include <fstream>

#include <memory>
#include <vector>
#include <chrono>
#include "ns3/p4-controller.h"
namespace ns3 {
//

/**
 *
 * \brief A P4 Pipeline Implementation to be wrapped in P4 Device
 *
 * The P4Model is using pipeline implementation provided by
 * `Behavioral Model` (https://github.com/p4lang/behavioral-model).
 * In particular, some internal processing functions and the `switch`
 * class are used. However, the way P4Model processes packets is
 * adapted to the requirements of ns-3.
 *
 * P4Model is initialized along with P4 Device, and expose a public
 * function called receivePacket() to the P4 Device. Whenever P4
 * Device has a packet needing handling, it call receivePacket and
 * wait for this function to return. receivePacket() puts the packet
 * through P4 pipeline.
 *
 * \attention P4Model transform ns::packet to bm::packet, which results
 * loss of metadata. We are currently working on reserving the metadata.
 *
 */

class P4Model: public bm::Switch {
public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId(void);

    /**
     * \brief a function from bm which will be called every time a
     * packet is received.
     *
     * Since we are not letting the bm switch really work in a
     * simulation enviornment. Instead we just borrow its processing pipeline,
     * which means this receive_() will never be called, so we just return 0.
     */
    int receive_(int port_num, const char *buffer, int len) {
        return 0;
    }

    /**
     * \brief a function from bm called to initialize the P4 device.
     *
     * Never called either for the same reason with receive_()
     */
    void start_and_return_() {
    }

    /**
     * \brief Process a packet using P4 pipeline. Called every time
     * there is a packet needing processing from P4 Device.
     *
     * \param ns3packet Original ns-3 packet and input port from P4 Device
     * \return Processed ns-3 packet to be put back to P4 Device for
     * transmission.
     */
    struct Ns3PacketAndPort * ReceivePacket(struct Ns3PacketAndPort *ns3Packet);

    /**
     * \brief Initialize the P4 Model
     *
     * We instantiate one P4 Model using a json file compiled from
     * P4 file. Also start the thrift to communicate with the
     * controller.
     *
     * \TODO We will implement a controller model in the future so
     * a thrift server is not needed to populate table entriea.
     *
     */
    int init(int argc, char *argv[]);

    /**
     * \brief Define target-specific properties, for example
     * `standard_metadata` and `intrinsic_metadata`
     */
    P4Model();

    ~P4Model()
    {
      if(m_argParser!=NULL)
        delete m_argParser;
    }

    /**
     * \brief configure switch with json file
     */
    int InitFromCommandLineOptionsLocal(int argc, char *argv[],bm::TargetParserBasic *tp = nullptr);

private:

    /**
     * \brief Transform a ns::packet and a bm::packet
     *
     * To use the P4 pipeline provided by Behavioral Model, input
     * packet must be conform the bm style. Also we preserve the
     * ingress port information here.
     *
     * Called when receive a packet from P4 Device.
     *
     * \param ns3packet A `ns::Packet` instance
     * \return A `bm::Packet` instance transformed from a ns::Packet instance.
     */
    struct Bm2PacketAndPort * Ns3ToBmv2(struct Ns3PacketAndPort * ns3Packet);

    /**
     * \brief Transform a bm::packet and a ns::packet
     *
     * Called when putting a packet back to the P4 Device.
     */
    struct Ns3PacketAndPort * Bmv2ToNs3(struct Bm2PacketAndPort *);

    /**
     * \brief Packet ID
     */
    int m_pktID = 0;

    using clock = std::chrono::high_resolution_clock;

    /**
     * \brief Structure of parsers
     */
    bm::TargetParserBasic * m_argParser;

    /**
     * A simple, 2-level, packet replication engine,
     * configurable by the control plane.
     */
    std::shared_ptr<bm::McSimplePre> m_pre;

};

/**
 * \brief Structure of a ns packet and port. An inelegant way of representing
 *  both packets along with its metadata.
 *
 * \TODO Find a better way.
 */
struct Ns3PacketAndPort {
    int portNum;
    Packet * packet;
};

/**
 * \brief Structure of a bm packet and port. An inelegant way of representing
 *  both packets along with its metadata.
 *
 * \TODO Find a better way.
 */
struct Bm2PacketAndPort {
    int portNum;
    std::unique_ptr<bm::Packet> packet;
};

/**
 * \brief Programmable Data Plane Device
 *
 * The key and most exciting part of NS4.
 *
 * P4NetDevice is a subclass of NetDevice in the ns-3 domain and serves as
 * the network layer of a P4 target. It is compatible with other net devices
 * in ns-3.
 *
 * \attention P4 Net Device now use `BridgeChannel` which only supports
 * IEEE 802 protocols.
 *
 * \TODO Create a new channel class supporting arbitrary underlying channel.
 *
 */
class P4NetDevice: public NetDevice {
public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId(void);

    P4NetDevice();
    virtual ~P4NetDevice();

    /**
     * \brief Gets the number of bridged 'ports', i.e., the NetDevices currently bridged.
     *
     * \return the number of bridged ports.
     */
    uint32_t GetNBridgePorts(void) const;

    /**
     * \brief Gets the n-th bridged port.
     * \param n the port index
     * \return the n-th bridged NetDevice
     */
    Ptr<NetDevice> GetBridgePort(uint32_t n) const;

    /**
     * \brief Add a port connected to the P4 target.
     * @param
     */
    void AddBridgePort(Ptr<NetDevice> bridgePort);

    /**
     * \brief Send a packet to one of the ports.
     */
    bool SendPacket(Ptr<Packet> packet, Ptr<NetDevice>outDevice);
    bool SendPacket(Ptr<Packet> packet, const Address& dest , Ptr<NetDevice>outDevice);

    /**
     * \brief Forwards a broadcast or a multicast packet
     * \param incomingPort the packet incoming port
     * \param packet the packet
     * \param protocol the packet protocol (e.g., Ethertype)
     * \param src the packet source
     * \param dst the packet destination
     */
    void ForwardBroadcast (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
        uint16_t protocol, Mac48Address src, Mac48Address dst);

    // inherited from NetDevice base class.
    virtual void SetIfIndex(const uint32_t index);
    virtual uint32_t GetIfIndex(void) const;
    virtual Ptr<Channel> GetChannel(void) const;
    virtual void SetAddress(Address address);
    virtual Address GetAddress(void) const;
    virtual bool SetMtu(const uint16_t mtu);
    virtual uint16_t GetMtu(void) const;
    virtual bool IsLinkUp(void) const;
    virtual void AddLinkChangeCallback(Callback<void> callback);
    virtual bool IsBroadcast(void) const;
    virtual Address GetBroadcast(void) const;
    virtual bool IsMulticast(void) const;
    virtual Address GetMulticast(Ipv4Address multicastGroup) const;
    virtual bool IsPointToPoint(void) const;
    virtual bool IsBridge(void) const;
    virtual bool Send(Ptr<Packet> packet, const Address& dest,
            uint16_t protocolNumber);//Deprecated, use SendPacket instead.
    virtual bool SendFrom(Ptr<Packet> packet, const Address& source,
            const Address& dest, uint16_t protocolNumber);//Deprecated, use SendPacket instead.
    virtual Ptr<Node> GetNode(void) const;
    virtual void SetNode(Ptr<Node> node);
    virtual bool NeedsArp(void) const;
    virtual void SetReceiveCallback(NetDevice::ReceiveCallback cb);
    virtual void SetPromiscReceiveCallback(
            NetDevice::PromiscReceiveCallback cb);
    virtual bool SupportsSendFrom() const;
    virtual Address GetMulticast(Ipv6Address addr) const;

protected:
      virtual void DoDispose (void);
    /**
     * \brief Callback function when a new packet is received from one of
     * underlying channels. It sends the packet to P4 Model for modification
     * and sends it out.
     *
     */
    void ReceiveFromDevice(Ptr<ns3::NetDevice> device,
            Ptr<const ns3::Packet> packet, uint16_t protocol,
            Address const &source, Address const &destination,
            PacketType packetType);

private:
    P4Model* p4Model;
    /**
     * \brief Copy constructor
     *
     * Defined and unimplemented to avoid misuse
     */
    P4NetDevice(const BridgeNetDevice &);

    /**
     * \brief Copy constructor
     *
     * Defined and unimplemented to avoid misuse
     * \returns
     */
    BridgeNetDevice &operator =(const BridgeNetDevice &);

    NetDevice::ReceiveCallback m_rxCallback; //!< receive callback
    NetDevice::PromiscReceiveCallback m_promiscRxCallback; //!< promiscuous receive callback

    Mac48Address m_address; //!< MAC address of the NetDevice
    Ptr<Node> m_node; //!< node owning this NetDevice
    std::vector< Ptr<NetDevice> > m_ports; //!< bridged ports
    Ptr<BridgeChannel> m_channel; //!< virtual bridged channel
    uint32_t m_ifIndex; //!< Interface index
    uint16_t m_mtu; //!< MTU of the bridged NetDevice

    /**
     * \brief get the port number of a net device connected to P4 net device.
     */
    int GetPortNumber(Ptr<NetDevice>);
}; //namespace ns3

}
#endif/* NS4_NS4_MODEL_H_H */


