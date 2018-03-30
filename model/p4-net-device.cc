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

#include "ns3/p4-net-device.h"
#include "ns3/helper.h"
#include "ns3/global.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/ethernet-header.h"
#include "ns3/arp-l3-protocol.h"
#include <bm/bm_sim/switch.h>
#include <bm/bm_sim/core/primitives.h>
#include <bm/bm_runtime/bm_runtime.h>
#include <bm/bm_sim/simple_pre.h>
#include <bm/SimpleSwitch.h>
#include <bm/bm_sim/options_parse.h>
#include <memory.h>
#include <memory>
#include <iomanip>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <array>
#include <chrono>
#include <thread>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <map>

using namespace ns3;

#define MAXSIZE 512
//MatchKeyValue_t flowtableMatchType;

NS_OBJECT_ENSURE_REGISTERED(P4NetDevice);
NS_OBJECT_ENSURE_REGISTERED(P4Model);

NS_LOG_COMPONENT_DEFINE("P4NetDevice");

TypeId P4NetDevice::GetTypeId(void)
{
	static TypeId tid =
		TypeId("ns3::P4NetDevice").SetParent<NetDevice>().SetGroupName("P4").AddConstructor<
		P4NetDevice>().AddAttribute("Mtu",
			"The MAC-level Maximum Transmission Unit",
			UintegerValue(1500),
			MakeUintegerAccessor(&P4NetDevice::SetMtu,
				&P4NetDevice::GetMtu),
			MakeUintegerChecker<uint16_t>());
	return tid;
}

// Call external commands. Maybe used in controller model.
std::string exec(const char* cmd)
{
	std::array<char, 128> buffer;
	std::string result;
	std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
	if (!pipe)
		throw std::runtime_error("popen() failed!");
	while (!feof(pipe.get())) {
		if (fgets(buffer.data(), 128, pipe.get()) != NULL)
			result += buffer.data();
	}
	return result;
}

void P4NetDevice::AddBridgePort(Ptr<NetDevice> bridgePort)
{
	if (!Mac48Address::IsMatchingType(bridgePort->GetAddress())) {
		NS_FATAL_ERROR("Device does not support eui 48 addresses: cannot be added to bridge.");
	}
	if (!bridgePort->SupportsSendFrom()) {
		NS_FATAL_ERROR("Device does not support SendFrom: cannot be added to bridge.");
	}
	if (m_address == Mac48Address()) {
		m_address = Mac48Address::ConvertFrom(bridgePort->GetAddress());
	}

	m_node->RegisterProtocolHandler(
		MakeCallback(&P4NetDevice::ReceiveFromDevice, this), 0, bridgePort,
		true);
	m_ports.push_back(bridgePort);
	m_channel->AddChannel(bridgePort->GetChannel());
}

TypeId P4Model::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::P4Model").SetParent<Object>().SetGroupName(
		"Network")
		// .AddConstructor<P4Model> ()

		;
	return tid;
}

void P4NetDevice::ReceiveFromDevice(Ptr<ns3::NetDevice> device,
	Ptr<const ns3::Packet> packetIn, uint16_t protocol,
	Address const &source, Address const &destination,
	PacketType packetType)
{
	NS_LOG_FUNCTION(this);
	//view received packet real content
	int pkgSize = packetIn->GetSize();
	NS_LOG_LOGIC("received packet_in size:" << pkgSize);
	packetIn->EnablePrinting();
	packetIn->Print(std::cout);
	std::cout << std::endl;

	Mac48Address src48 = Mac48Address::ConvertFrom(source);
	Mac48Address dst48 = Mac48Address::ConvertFrom(destination);

	int portNum = GetPortNumber(device);
	struct Ns3PacketAndPort *ns3Packet = new (struct Ns3PacketAndPort);
	ns3Packet->portNum = portNum;
	ns3Packet->packet = (ns3::Packet*)packetIn.operator ->();
	EthernetHeader eeh;
	eeh.SetDestination(dst48);
	eeh.SetSource(src48);
	eeh.SetLengthType(protocol);

	ns3Packet->packet->AddHeader(eeh);

	struct Ns3PacketAndPort* egressPacketAndPort = p4Model->ReceivePacket(ns3Packet);

	if (egressPacketAndPort) {
		egressPacketAndPort->packet->RemoveHeader(eeh);
		int egressPortNum = egressPacketAndPort->portNum;
		// judge whether packet drop
		if (egressPortNum != 511) {
			NS_LOG_LOGIC("EgressPortNum: " << egressPortNum);
			Ptr<NetDevice> outNetDevice = GetBridgePort(egressPortNum);
			outNetDevice->Send(egressPacketAndPort->packet->Copy(), destination, protocol);
		}
		else
			std::cout << "Drop Packet!!!(511)\n";
	}
	else
		std::cout << "Null Packet!\n";
}

int P4NetDevice::GetPortNumber(Ptr<NetDevice> port) {
	int portsNum = GetNBridgePorts();
	for (int i = 0; i < portsNum; i++) {
		if (GetBridgePort(i) == port)
			return i;
	}
	return -1;
}


P4NetDevice::P4NetDevice() :
	m_node(0), m_ifIndex(0) {
	NS_LOG_FUNCTION_NOARGS();
	m_channel = CreateObject<BridgeChannel>();// Use BridgeChannel for prototype. Will develop P4 Channel in the future.
	P4SwitchInterface* p4Switch=P4GlobalVar::g_p4Controller.AddP4Switch();

	p4Model = new P4Model;
	
	p4Switch->SetP4Model(p4Model);
	p4Switch->SetJsonPath(P4GlobalVar::g_p4JsonPath);
	p4Switch->SetP4InfoPath(P4GlobalVar::g_p4MatchTypePath);
	p4Switch->SetFlowTablePath(P4GlobalVar::g_flowTablePath);
	p4Switch->SetViewFlowTablePath(P4GlobalVar::g_viewFlowTablePath);
	p4Switch->SetNetworkFunc(P4GlobalVar::g_networkFunc);

	char * a3;
	a3 = (char*)P4GlobalVar::g_p4JsonPath.data();
	char * args[2] = { NULL,a3 };
	p4Model->init(2, args);
	
	//Init P4Model Flow Table
	if(P4GlobalVar::g_populateFlowTableWay == LOCAL_CALL)
		p4Switch->Init();
	
	p4Switch=NULL;
	NS_LOG_LOGIC("A P4 Netdevice was initialized.");
}

uint32_t
P4NetDevice::GetNBridgePorts(void) const {
	// NS_LOG_FUNCTION_NOARGS ();
	return m_ports.size();
}

Ptr<NetDevice>
P4NetDevice::GetBridgePort(uint32_t n) const {
	// NS_LOG_FUNCTION_NOARGS ();
	if (n >= m_ports.size()) return NULL;
	return m_ports[n];
}

P4NetDevice::~P4NetDevice() {
	if(p4Model!=NULL)
		delete p4Model;
	NS_LOG_FUNCTION_NOARGS();
}

void
P4NetDevice::DoDispose() {
	NS_LOG_FUNCTION_NOARGS();
	for (std::vector< Ptr<NetDevice> >::iterator iter = m_ports.begin(); iter != m_ports.end(); iter++) {
		*iter = 0;
	}
	m_ports.clear();
	m_channel = 0;
	m_node = 0;
	NetDevice::DoDispose();
}


void
P4NetDevice::SetIfIndex(const uint32_t index) {
	NS_LOG_FUNCTION_NOARGS();
	m_ifIndex = index;
}

uint32_t
P4NetDevice::GetIfIndex(void) const {
	NS_LOG_FUNCTION_NOARGS();
	return m_ifIndex;
}

Ptr<Channel>
P4NetDevice::GetChannel(void) const {
	NS_LOG_FUNCTION_NOARGS();
	return m_channel;
}

void
P4NetDevice::SetAddress(Address address) {
	NS_LOG_FUNCTION_NOARGS();
	m_address = Mac48Address::ConvertFrom(address);
}

Address
P4NetDevice::GetAddress(void) const {
	NS_LOG_FUNCTION_NOARGS();
	return m_address;
}

bool
P4NetDevice::SetMtu(const uint16_t mtu) {
	NS_LOG_FUNCTION_NOARGS();
	m_mtu = mtu;
	return true;
}

uint16_t
P4NetDevice::GetMtu(void) const {
	NS_LOG_FUNCTION_NOARGS();
	return m_mtu;
}


bool
P4NetDevice::IsLinkUp(void) const {
	NS_LOG_FUNCTION_NOARGS();
	return true;
}


void
P4NetDevice::AddLinkChangeCallback(Callback<void> callback)
{}


bool
P4NetDevice::IsBroadcast(void) const {
	NS_LOG_FUNCTION_NOARGS();
	return true;
}


Address
P4NetDevice::GetBroadcast(void) const {
	NS_LOG_FUNCTION_NOARGS();
	return Mac48Address("ff:ff:ff:ff:ff:ff");
}

bool
P4NetDevice::IsMulticast(void) const {
	NS_LOG_FUNCTION_NOARGS();
	return true;
}

Address
P4NetDevice::GetMulticast(Ipv4Address multicastGroup) const {
	NS_LOG_FUNCTION(this << multicastGroup);
	Mac48Address multicast = Mac48Address::GetMulticast(multicastGroup);
	return multicast;
}


bool
P4NetDevice::IsPointToPoint(void) const {
	NS_LOG_FUNCTION_NOARGS();
	return false;
}

bool
P4NetDevice::IsBridge(void) const {
	NS_LOG_FUNCTION_NOARGS();
	return true;
}


bool
P4NetDevice::Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) {
	return false;
}

bool
P4NetDevice::SendFrom(Ptr<Packet> packet, const Address& src, const Address& dest, uint16_t protocolNumber) {
	return false;
}

bool
P4NetDevice::SendPacket(Ptr<Packet> packet, const Address& dest, Ptr<NetDevice>outDevice) {
	NS_LOG_FUNCTION_NOARGS();
	// outDevice->Send();
	return false;
}

bool P4NetDevice::SendPacket(Ptr<Packet> packet, Ptr<NetDevice>outDevice) {
	NS_LOG_FUNCTION_NOARGS();
	// outDevice->Send();
	return false;
}


Ptr<Node>
P4NetDevice::GetNode(void) const {
	NS_LOG_FUNCTION_NOARGS();
	return m_node;
}


void
P4NetDevice::SetNode(Ptr<Node> node) {
	NS_LOG_FUNCTION_NOARGS();
	m_node = node;
}


bool
P4NetDevice::NeedsArp(void) const {
	NS_LOG_FUNCTION_NOARGS();
	return true;
}


void
P4NetDevice::SetReceiveCallback(NetDevice::ReceiveCallback cb) {
	NS_LOG_FUNCTION_NOARGS();
	m_rxCallback = cb;
}

void
P4NetDevice::SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb) {
	NS_LOG_FUNCTION_NOARGS();
	m_promiscRxCallback = cb;
}

bool
P4NetDevice::SupportsSendFrom() const {
	NS_LOG_FUNCTION_NOARGS();
	return true;
}

Address P4NetDevice::GetMulticast(Ipv6Address addr) const {
	NS_LOG_FUNCTION(this << addr);
	return Mac48Address::GetMulticast(addr);
}


using bm::McSimplePre;
extern int import_primitives();

P4Model::P4Model() :
	m_pre(new bm::McSimplePre()) {

	add_component<bm::McSimplePre>(m_pre);

	m_argParser = new bm::TargetParserBasic();
	add_required_field("standard_metadata", "ingress_port");
	add_required_field("standard_metadata", "packet_length");
	add_required_field("standard_metadata", "instance_type");
	add_required_field("standard_metadata", "egress_spec");
	add_required_field("standard_metadata", "clone_spec");
	add_required_field("standard_metadata", "egress_port");

	force_arith_field("standard_metadata", "ingress_port");
	force_arith_field("standard_metadata", "packet_length");
	force_arith_field("standard_metadata", "instance_type");
	force_arith_field("standard_metadata", "egress_spec");
	force_arith_field("standard_metadata", "clone_spec");

	force_arith_field("queueing_metadata", "enq_timestamp");
	force_arith_field("queueing_metadata", "enq_qdepth");
	force_arith_field("queueing_metadata", "deq_timedelta");
	force_arith_field("queueing_metadata", "deq_qdepth");

	force_arith_field("intrinsic_metadata", "ingress_global_timestamp");
	force_arith_field("intrinsic_metadata", "lf_field_list");
	force_arith_field("intrinsic_metadata", "mcast_grp");
	force_arith_field("intrinsic_metadata", "resubmit_flag");
	force_arith_field("intrinsic_metadata", "egress_rid");
	force_arith_field("intrinsic_metadata", "recirculate_flag");

	import_primitives();
}


int P4Model::init(int argc, char *argv[]) {

	NS_LOG_FUNCTION(this);
	using ::sswitch_runtime::SimpleSwitchIf;
	using ::sswitch_runtime::SimpleSwitchProcessor;

	// use local call to populate flowtable
	if (P4GlobalVar::g_populateFlowTableWay == LOCAL_CALL)
	{
		this->InitFromCommandLineOptionsLocal(argc, argv, m_argParser);
	}
	else
	{
		// start thrift server , use runtime_CLI populate flowtable
		if (P4GlobalVar::g_populateFlowTableWay == RUNTIME_CLI)
		{
			this->init_from_command_line_options(argc, argv, m_argParser);
			int thriftPort = this->get_runtime_port();
			bm_runtime::start_server(this, thriftPort);
			NS_LOG_LOGIC("Wait " << P4GlobalVar::g_runtimeCliTime << " seconds for RuntimeCLI operations ");
			std::this_thread::sleep_for(std::chrono::seconds(P4GlobalVar::g_runtimeCliTime));
			//bm_runtime::add_service<SimpleSwitchIf, SimpleSwitchProcessor>("simple_switch", sswitch_runtime::get_handler(this));
		}
	}
	return 0;
}

int P4Model::InitFromCommandLineOptionsLocal(int argc, char *argv[], bm::TargetParserBasic *tp)
{


	NS_LOG_FUNCTION(this);
	bm::OptionsParser parser;
	parser.parse(argc, argv, tp);
	//NS_LOG_LOGIC("parse pass");
	std::shared_ptr<bm::TransportIface> transport = nullptr;
	int status = 0;
	if (transport == nullptr)
	{
#ifdef BMNANOMSG_ON
		//notifications_addr = parser.notifications_addr;
		transport = std::shared_ptr<bm::TransportIface>(
			TransportIface::make_nanomsg(parser.notifications_addr));
#else
		//notifications_addr = "";
		transport = std::shared_ptr<bm::TransportIface>(bm::TransportIface::make_dummy());
#endif
	}
	if (parser.no_p4)
		status = init_objects_empty(parser.device_id, transport);
	else
		// load p4 json to switch
		status = init_objects(parser.config_file_path, parser.device_id, transport);
	return status;
}


struct Ns3PacketAndPort * P4Model::ReceivePacket(
	struct Ns3PacketAndPort *ns3Packet) {
	NS_LOG_FUNCTION(this);
	struct Bm2PacketAndPort * bm2Packet = Ns3ToBmv2(ns3Packet);
	std::unique_ptr<bm::Packet> packet = std::move(bm2Packet->packet);

	if (packet) {
		int portNum = bm2Packet->portNum;
		int len = packet.get()->get_data_size();
		packet.get()->set_ingress_port(portNum);
		bm::PHV *phv = packet.get()->get_phv();
		phv->reset_metadata();
		phv->get_field("standard_metadata.ingress_port").set(portNum);
		phv->get_field("standard_metadata.packet_length").set(len);

		if (phv->has_field("intrinsic_metadata.ingress_global_timestamp")) {
			phv->get_field("intrinsic_metadata.ingress_global_timestamp").set(0);
		}

		// Ingress
		bm::Parser *parser = this->get_parser("parser");
		bm::Pipeline *ingressMau = this->get_pipeline("ingress");
		phv = packet.get()->get_phv();

		parser->parse(packet.get());

		ingressMau->apply(packet.get());

		packet->reset_exit();

		bm::Field &fEgressSpec = phv->get_field("standard_metadata.egress_spec");
		int egressPort = fEgressSpec.get_int();

		// Egress
		bm::Deparser *deparser = this->get_deparser("deparser");
		bm::Pipeline *egressMau = this->get_pipeline("egress");
		fEgressSpec = phv->get_field("standard_metadata.egress_spec");
		fEgressSpec.set(0);
		egressMau->apply(packet.get());
		deparser->deparse(packet.get());

		// Build return value
		struct Bm2PacketAndPort* outPacket = new struct Bm2PacketAndPort;
		outPacket->packet = std::move(packet);
		outPacket->portNum = egressPort;
		return Bmv2ToNs3(outPacket);
	}
	return NULL;
}

struct Ns3PacketAndPort * P4Model::Bmv2ToNs3(
	struct Bm2PacketAndPort *bm2Packet) {
	struct Ns3PacketAndPort * ret = new struct Ns3PacketAndPort;
	void *buffer = bm2Packet->packet.get()->data();
	size_t length = bm2Packet->packet.get()->get_data_size();
	ret->packet = new ns3::Packet((uint8_t*)buffer, length);
	ret->portNum = bm2Packet->portNum;
	return ret;
}

struct Bm2PacketAndPort * P4Model::Ns3ToBmv2(
	struct Ns3PacketAndPort *ns3Packet) {
	int length = ns3Packet->packet->GetSize();
	uint8_t* buffer = new uint8_t[length];
	struct Bm2PacketAndPort* ret = new struct Bm2PacketAndPort;

	int portNum = ns3Packet->portNum;
	if (ns3Packet->packet->CopyData(buffer, length)) {
		std::unique_ptr<bm::Packet> packet_ = new_packet_ptr(portNum, m_pktID++,
			length, bm::PacketBuffer(2048, (char*)buffer, length));
		ret->packet = std::move(packet_);
	}
	ret->portNum = portNum;
	return ret;
}

