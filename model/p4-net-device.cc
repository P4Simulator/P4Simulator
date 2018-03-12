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

/*
// siwtch info
unsigned int g_networkFunc;
std::string g_p4MatchTypePath;
std::string g_flowtablePath;

unsigned int g_populateFlowtableWay;

// path info
std::string g_homePath;
std::string g_ns3RootName;
std::string g_ns3SrcName;
unsigned int g_nsType;
std::string g_nfDir;
*/

#define MAXSIZE 512
MatchKeyValue_t flowtableMatchType;

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
    NS_LOG_LOGIC("received packet_in size:"<<pkgSize);
    packetIn->EnablePrinting();
    packetIn->Print(std::cout);
    std::cout<<std::endl;

    Mac48Address src48 = Mac48Address::ConvertFrom (source);
    Mac48Address dst48 = Mac48Address::ConvertFrom (destination);

    int portNum = GetPortNumber(device);
    struct Ns3PacketAndPort *ns3Packet = new (struct Ns3PacketAndPort);
    ns3Packet->portNum = portNum;
    ns3Packet->packet=(ns3::Packet*)packetIn.operator ->();
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
        if(egressPortNum!=511){
          NS_LOG_LOGIC("egressPortNum: "<<egressPortNum);
          Ptr<NetDevice> outNetDevice = GetBridgePort(egressPortNum);
          outNetDevice->Send(egressPacketAndPort->packet->Copy(), destination, protocol);
          }
        else
            std::cout<<"drop packet!\n";
    } 
    else 
        std::cout << "Null packet!\n";
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
    NS_LOG_FUNCTION_NOARGS ();
    m_channel = CreateObject<BridgeChannel> ();// Use BridgeChannel for prototype. Will develop P4 Channel in the future.
    p4Model = new P4Model;
    char * a3;
    /*std::string jsonPath;
    if(P4GlobalVar::g_networkFunc==FIREWALL) 
       {
	     jsonPath=P4GlobalVar::g_nfDir+"firewall/firewall.json";
         a3=(char*)jsonPath.data();

	   }
    if(P4GlobalVar::g_networkFunc==SILKROAD)
       {
	    jsonPath=P4GlobalVar::g_nfDir+"silkroad/silkroad.json";
        a3=(char*)jsonPath.data();
	}
    if(P4GlobalVar::g_networkFunc==ROUTER)
       {
	    jsonPath=P4GlobalVar::g_nfDir+"router/router.json";
        a3=(char*)jsonPath.data();
	}
    if(P4GlobalVar::g_networkFunc==SIMPLE_ROUTER)
       {
        jsonPath=P4GlobalVar::g_nfDir+"simple_router/simple_router.json";
		a3=(char*)jsonPath.data();
	}*/
    a3=(char*)P4GlobalVar::g_p4JsonPath.data();
    char * args[2] = { NULL,a3};
    p4Model->init(2, args);
    NS_LOG_LOGIC("A P4 Netdevice was initialized.");
}

uint32_t
P4NetDevice::GetNBridgePorts (void) const {
    // NS_LOG_FUNCTION_NOARGS ();
    return m_ports.size ();
}

Ptr<NetDevice>
P4NetDevice::GetBridgePort (uint32_t n) const {
    // NS_LOG_FUNCTION_NOARGS ();
    if (n >= m_ports.size()) return NULL;
    return m_ports[n];
}

P4NetDevice::~P4NetDevice() {
    NS_LOG_FUNCTION_NOARGS ();
}

void
P4NetDevice::DoDispose () {
    NS_LOG_FUNCTION_NOARGS ();
    for (std::vector< Ptr<NetDevice> >::iterator iter = m_ports.begin (); iter != m_ports.end (); iter++) {
      *iter = 0;
    }
    m_ports.clear ();
    m_channel = 0;
    m_node = 0;
    NetDevice::DoDispose ();
}


void
P4NetDevice::SetIfIndex (const uint32_t index) {
      NS_LOG_FUNCTION_NOARGS ();
      m_ifIndex = index;
}

uint32_t
P4NetDevice::GetIfIndex (void) const {
      NS_LOG_FUNCTION_NOARGS ();
      return m_ifIndex;
}

Ptr<Channel>
P4NetDevice::GetChannel (void) const {
    NS_LOG_FUNCTION_NOARGS ();
    return m_channel;
}

void
P4NetDevice::SetAddress (Address address) {
    NS_LOG_FUNCTION_NOARGS ();
    m_address = Mac48Address::ConvertFrom (address);
}

Address
P4NetDevice::GetAddress (void) const {
    NS_LOG_FUNCTION_NOARGS ();
    return m_address;
}

bool
P4NetDevice::SetMtu (const uint16_t mtu) {
    NS_LOG_FUNCTION_NOARGS ();
    m_mtu = mtu;
    return true;
}

uint16_t
P4NetDevice::GetMtu (void) const {
    NS_LOG_FUNCTION_NOARGS ();
    return m_mtu;
}


bool
P4NetDevice::IsLinkUp (void) const {
    NS_LOG_FUNCTION_NOARGS ();
    return true;
}


void
P4NetDevice::AddLinkChangeCallback (Callback<void> callback)
{}


bool
P4NetDevice::IsBroadcast (void) const {
    NS_LOG_FUNCTION_NOARGS ();
    return true;
}


Address
P4NetDevice::GetBroadcast (void) const {
    NS_LOG_FUNCTION_NOARGS ();
    return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
P4NetDevice::IsMulticast (void) const {
    NS_LOG_FUNCTION_NOARGS ();
    return true;
}

Address
P4NetDevice::GetMulticast (Ipv4Address multicastGroup) const {
    NS_LOG_FUNCTION (this << multicastGroup);
    Mac48Address multicast = Mac48Address::GetMulticast (multicastGroup);
    return multicast;
}


bool
P4NetDevice::IsPointToPoint (void) const {
    NS_LOG_FUNCTION_NOARGS ();
    return false;
}

bool
P4NetDevice::IsBridge (void) const {
    NS_LOG_FUNCTION_NOARGS ();
    return true;
}


bool
P4NetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) {
    return false;
}

bool
P4NetDevice::SendFrom (Ptr<Packet> packet, const Address& src, const Address& dest, uint16_t protocolNumber) {
    return false;
}

bool
P4NetDevice::SendPacket (Ptr<Packet> packet, const Address& dest, Ptr<NetDevice>outDevice) {
    NS_LOG_FUNCTION_NOARGS ();
    // outDevice->Send();
    return false;
}

bool P4NetDevice::SendPacket(Ptr<Packet> packet, Ptr<NetDevice>outDevice){
    NS_LOG_FUNCTION_NOARGS ();
    // outDevice->Send();
    return false;
}


Ptr<Node>
P4NetDevice::GetNode (void) const {
    NS_LOG_FUNCTION_NOARGS ();
    return m_node;
}


void
P4NetDevice::SetNode (Ptr<Node> node) {
    NS_LOG_FUNCTION_NOARGS ();
    m_node = node;
}


bool
P4NetDevice::NeedsArp (void) const {
    NS_LOG_FUNCTION_NOARGS ();
    return true;
}


void
P4NetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb) {
    NS_LOG_FUNCTION_NOARGS ();
    m_rxCallback = cb;
}

void
P4NetDevice::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb) {
    NS_LOG_FUNCTION_NOARGS ();
    m_promiscRxCallback = cb;
}

bool
P4NetDevice::SupportsSendFrom () const {
    NS_LOG_FUNCTION_NOARGS ();
    return true;
}

Address P4NetDevice::GetMulticast (Ipv6Address addr) const {
    NS_LOG_FUNCTION (this << addr);
    return Mac48Address::GetMulticast (addr);
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
    if(P4GlobalVar::g_populateFlowTableWay==LOCAL_CALL)
    {
        this->InitFromCommandLineOptionsLocal(argc, argv, m_argParser);
        ReadTableActionMatchType(P4GlobalVar::g_p4MatchTypePath,flowtableMatchType);
        PopulateFlowTable(P4GlobalVar::g_flowTablePath);
        for(MatchKeyValue_t::iterator iter=flowtableMatchType.begin();iter!=flowtableMatchType.end();iter++)
	      ViewFlowtableEntryNum(iter->first); 
    }
    else
    {
        // start thrift server , use runtime_CLI populate flowtable
        if(P4GlobalVar::g_populateFlowTableWay==RUNTIME_CLI)
        {
            this->init_from_command_line_options(argc, argv, m_argParser);
            int thriftPort = this->get_runtime_port();
            bm_runtime::start_server(this, thriftPort);
            std::cout << "\nNight is coming. Sleep for 5 seconds.\n";
            std::this_thread::sleep_for(std::chrono::seconds(5));
            //bm_runtime::add_service<SimpleSwitchIf, SimpleSwitchProcessor>("simple_switch", sswitch_runtime::get_handler(this));
        }
    }
    return 0;
}

void P4Model::ViewFlowtableEntryNum(std::string flowtableName)
{
    std::vector<bm::MatchTable::Entry> entries=mt_get_entries(0,flowtableName);
    std::cout<<flowtableName<<" entry num:"<<entries.size()<<std::endl;
}

void P4Model::ReadTableActionMatchType(std::string filePath,MatchKeyValue_t& tableActionMatchType)
{

    std::ifstream topgen;
    topgen.open(filePath);

    if (!topgen.is_open())
    {
        std::cout <<filePath<< " can not open!"<< std::endl;
        abort();
    }

    std::string tableAction;
    std::string matchType;

    std::istringstream lineBuffer;
    std::string line;

    while (getline(topgen, line))
    {
        lineBuffer.clear();
        lineBuffer.str(line);

        lineBuffer >> tableAction >> matchType;
        if (matchType.compare("exact")==0)
        {
            tableActionMatchType[tableAction]=bm::MatchKeyParam::Type::EXACT;
        }
        else
        {
            if (matchType.compare("lpm")==0)
            {
                 tableActionMatchType[tableAction]=bm::MatchKeyParam::Type::LPM;
            }
            else
            {
                if (matchType.compare("ternary")==0)
                {
                     tableActionMatchType[tableAction]=bm::MatchKeyParam::Type::TERNARY;
                }
                else
                {
                    if (matchType.compare("valid")==0)
                    {
                         tableActionMatchType[tableAction]=bm::MatchKeyParam::Type::VALID;
                    }
                    else
                    {
                        if (matchType.compare("range")==0)
                        {
                             tableActionMatchType[tableAction]=bm::MatchKeyParam::Type::RANGE;
                        }
                        else
                        {
                            std::cout << "match type error." << std::endl;
                        }
                    }
                }
            }
        }
    //    std::cout << table_action << " " << match_type << std::endl;
    }
}

int P4Model::InitFromCommandLineOptionsLocal(int argc, char *argv[],bm::TargetParserBasic *tp)
{

    
    NS_LOG_FUNCTION(this);
    bm::OptionsParser parser;
    parser.parse(argc,argv,tp);
    //NS_LOG_LOGIC("parse pass");
    std::shared_ptr<bm::TransportIface> transport=nullptr;
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


void P4Model::PopulateFlowTable(const std::string commandPath)
{

    std::fstream fp;
    fp.open(commandPath);
    if (!fp)
    {
        std::cout <<"in P4Model::PopulateFlowTable, "<< commandPath << " can't open." << std::endl;
    }
    else
    {
        const int maxSize=500;
        char row[maxSize];
        while (fp.getline(row, maxSize))
        {
            ParseFlowtableCommand(std::string(row));
        }
    }
}
void P4Model::ParseFlowtableCommand(const std::string commandRow)
{

    std::vector<std::string> parms;
    int lastP = 0, curP = 0;
    for (size_t i = 0; i < commandRow.size(); i++,curP++)
    {
        if (commandRow[i] == ' ')
        {
            parms.push_back(commandRow.substr(lastP, curP - lastP));
            lastP = i + 1;
        }
    }
    if (lastP < curP)
    {
        parms.push_back(commandRow.substr(lastP, curP - lastP));
    }
    if (parms.size() > 0)
    {
        if (parms[0].compare("table_set_default") == 0)
        {
          
            bm::ActionData actionData;
            if (parms.size() > 3)
            {
                for (size_t i = 3; i < parms.size(); i++)
                    actionData.push_back_action_data(bm::Data(parms[i]));
            }
            mt_set_default_action(0, parms[1], parms[2], actionData);
        }
        else
        {
            if (parms[0].compare("table_add") == 0)
            {
               
                std::vector<bm::MatchKeyParam> matchKey;
                bm::ActionData actionData;
                bm::entry_handle_t *handle=new bm::entry_handle_t(1);// table entry num
                bm::MatchKeyParam::Type matchType=flowtableMatchType[parms[1]];
                
                unsigned int keyNum=0;
                unsigned int actionDataNum=0;
                size_t i;
                for(i=3;i<parms.size();i++)
                {
                    if(parms[i].compare("=>")!=0)
                    {
                        keyNum++;
                        if(matchType==bm::MatchKeyParam::Type::EXACT)
                        {
                            matchKey.push_back(bm::MatchKeyParam(matchType,HexstrToBytes(parms[i])));
                        }
                        else
                        {
                            if(matchType==bm::MatchKeyParam::Type::LPM)
                            {
                                int pos=parms[i].find("/");
                                std::string prefix=parms[i].substr(0,pos);
                                std::string length=parms[i].substr(pos+1);
                                unsigned int prefixLength=StrToInt(length);
                                matchKey.push_back(bm::MatchKeyParam(matchType,HexstrToBytes(parms[i],prefixLength),int(prefixLength)));
                            }
                            else
                            {
                                if(matchType==bm::MatchKeyParam::Type::TERNARY)
                                {
                                    int pos=parms[i].find("&&&");
                                    std::string key=HexstrToBytes(parms[i].substr(0,pos));
                                    std::string mask=HexstrToBytes(parms[i].substr(pos+3));
                                    if(key.size()!=mask.size())
                                    {
                                        std::cout<<"key and mask length unmatch!"<<std::endl;
                                        abort();
                                    }
                                    else
                                    {
                                        matchKey.push_back(bm::MatchKeyParam(matchType,key,mask));
                                    }
                                }
                            }
                        }
                    }
                    else
                        break;
                }
                i++;
                int priority;
                //judge key_num equal table need key num
                if(matchType!=bm::MatchKeyParam::Type::TERNARY&&matchType!=bm::MatchKeyParam::Type::RANGE)
                {
                    for(;i<parms.size();i++)
                    {
                        actionDataNum++;
                        actionData.push_back_action_data(bm::Data(parms[i]));
                    }
                    priority=0;
                    // judge action_data_num equal action need num
                     mt_add_entry(0, parms[1], matchKey, parms[2], actionData, handle,priority);
                }
                else
                {
                    for(;i<parms.size()-1;i++)
                    {
                        actionDataNum++;
                        actionData.push_back_action_data(bm::Data(parms[i]));
                    }
                    // judge action_data_num equal action need num
                    priority=StrToInt(parms[parms.size()-1]);
                    mt_add_entry(0, parms[1], matchKey, parms[2], actionData, handle, priority);
                }
            }
        }
    }
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

