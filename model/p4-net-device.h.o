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

#include <fstream>
#include <memory>
#include <vector>
#include <chrono>

#include "ns3/p4-controller.h"
#include "ns3/p4-model.h"

namespace ns3 {
	
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
	class P4Model;

	class P4NetDevice : public NetDevice {
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
		bool SendPacket(Ptr<Packet> packet, const Address& dest, Ptr<NetDevice>outDevice);
		void SendNs3Packet(struct Ns3PacketAndPort *ns3Packet, uint16_t protocol, Address const &destination);

		/**
		* \brief Forwards a broadcast or a multicast packet
		* \param incomingPort the packet incoming port
		* \param packet the packet
		* \param protocol the packet protocol (e.g., Ethertype)
		* \param src the packet source
		* \param dst the packet destination
		*/
		void ForwardBroadcast(Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
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
		virtual void DoDispose(void);
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



