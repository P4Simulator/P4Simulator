#include "p4-net-device.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/ethernet-header.h"
#include "ns3/arp-l3-protocol.h"

#include <bm/bm_sim/switch.h>
#include <bm/bm_sim/core/primitives.h>
#include <bm/bm_runtime/bm_runtime.h>
#include <bm/bm_sim/simple_pre.h>
#include <bm/SimpleSwitch.h>

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
#include <string>
#include <stdio.h>
#include <stdlib.h>
using namespace ns3;

#define MAXSIZE 512

std::string networkFunc; // define in p4-example.cc
//networkFunc=std::string("firewall");
int switchIndex; //define in p4-example.cc, to decide thrift_port
NS_OBJECT_ENSURE_REGISTERED(P4NetDevice);
NS_OBJECT_ENSURE_REGISTERED(P4Model);

NS_LOG_COMPONENT_DEFINE("P4NetDevice");

TypeId P4NetDevice::GetTypeId(void) {
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
std::string exec(const char* cmd) {
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

void P4NetDevice::AddBridgePort(Ptr<NetDevice> bridgePort) {
    //NS_LOG_FUNCTION_NOARGS ();
    // NS_ASSERT (bridgePort != this);
    if (!Mac48Address::IsMatchingType(bridgePort->GetAddress())) {
        NS_FATAL_ERROR("Device does not support eui 48 addresses: cannot be added to bridge.");
    }
    if (!bridgePort->SupportsSendFrom()) {
        NS_FATAL_ERROR("Device does not support SendFrom: cannot be added to bridge.");
    }
    if (m_address == Mac48Address()) {
        m_address = Mac48Address::ConvertFrom(bridgePort->GetAddress());
    }

    // NS_LOG_DEBUG ("RegisterProtocolHandler for " << bridgePort->GetInstanceTypeId ().GetName ());
    m_node->RegisterProtocolHandler(
            MakeCallback(&P4NetDevice::ReceiveFromDevice, this), 0, bridgePort,
            true);
    m_ports.push_back(bridgePort);
    m_channel->AddChannel(bridgePort->GetChannel());
}

TypeId P4Model::GetTypeId(void) {
 static TypeId tid = TypeId("ns3::P4Model").SetParent<Object>().SetGroupName(
         "Network")
         // .AddConstructor<P4Model> ()

         ;
 return tid;
}

void P4NetDevice::ReceiveFromDevice(Ptr<ns3::NetDevice> device,
        Ptr<const ns3::Packet> packet_in, uint16_t protocol,
        Address const &source, Address const &destination,
        PacketType packetType) {
    NS_LOG_FUNCTION(this);
    
    //  *************View received packet real content, can be removed*******************
    int pkgSize = packet_in->GetSize();
    NS_LOG_LOGIC("received packet_in size:"<<pkgSize);
    packet_in->EnablePrinting();
    packet_in->Print(std::cout);
    std::cout<<std::endl;
    //  ******************************************************************
    // output packet real content to debug
    /*uint8_t* bu = new uint8_t [buSize];
    packet_in->CopyData(bu, buSize);
    for (int i = 0; i < buSize; i ++) 
        std::cout << std::setfill('0') << std::setw(2) << std::hex << (int)bu[i] << ' ';
    std::cout << '\n';*/

    Mac48Address src48 = Mac48Address::ConvertFrom (source);
    Mac48Address dst48 = Mac48Address::ConvertFrom (destination);

    NS_LOG_LOGIC("src mac: "<<std::hex<<src48);
    NS_LOG_LOGIC("dst mac: "<<std::hex<<dst48);

    int port_num = GetPortNumber(device);
    struct ns3PacketAndPort *ns3packet = new (struct ns3PacketAndPort);
    ns3packet->port_num = port_num;
    //  *****************************TO DO********************************
    // there exists some problems in using packet_in->Copy().operator ->() 
    //ns3packet->packet = packet_in->Copy().operator ->();
    /**using the following methods realizing copy function temporarily, may find
     * some better ways latter.
     */  

    ns3packet->packet=(ns3::Packet*)packet_in.operator ->();
    //  *******************************************************************
    EthernetHeader eeh;
    eeh.SetDestination(dst48);
    eeh.SetSource(src48);
    eeh.SetLengthType(protocol);
    // ouput added ethernetHeader info to debug
    /*NS_LOG_LOGIC("EthernetHeader size: "<<eeh.GetHeaderSize());
    eeh.Print(std::cout);
    std::cout<<std::endl;*/

    ns3packet->packet->AddHeader(eeh);
    // output packet content after CopyDaTa and AddHeader to debug
    /*int bS = ns3packet->packet->GetSize();
    uint8_t* b = new uint8_t [bS];
    ns3packet->packet->CopyData(b, bS);
    for (int i = 34; i < 38&&i<bS; i ++) 
        std::cout << std::setfill('0') << std::setw(2) << std::hex << (int)b[i] << ' ';
    std::cout << '\n';*/
    
    struct ns3PacketAndPort* egress_packetandport = p4Model->receivePacket(ns3packet);

    if (egress_packetandport) {
        egress_packetandport->packet->RemoveHeader(eeh);
        int egress_port_num = egress_packetandport->port_num;
        // judge whether packet drop
        if(egress_port_num!=511){
          // ***********View egress port num, can be removed ***************************
          NS_LOG_LOGIC("egress_port_num: "<<egress_port_num);
          // ***************************************************************************
          Ptr<NetDevice> outNetDevice = GetBridgePort(egress_port_num);
          outNetDevice->Send(egress_packetandport->packet->Copy(), destination, protocol);
          }
        else
            std::cout<<"drop packet!\n";
    } else std::cout << "Null packet!\n";
}

int P4NetDevice::GetPortNumber(Ptr<NetDevice> port) {
    int ports_num = GetNBridgePorts();
    for (int i = 0; i < ports_num; i++) {
        if (GetBridgePort(i) == port)
            return i;
    }
    return -1;
}
char * int_to_str(int num)
{
    char *ss=new char[10];
    int pos = 0;

    if (num == 0)
        ss[0] = '0';
    while (num)
    {
        ss[pos++] = num % 10 + '0';
        num = num / 10;
    }
    char temp;
    for (int i = 0; i < pos / 2; i++)
    {
        temp = ss[i];
        ss[i] = ss[pos - 1 - i];
        ss[pos - 1 - i] = temp;
    }
    ss[pos] = '\0';
    return ss;
}

P4NetDevice::P4NetDevice() :
    m_node(0), m_ifIndex(0) {
    NS_LOG_FUNCTION_NOARGS ();
    m_channel = CreateObject<BridgeChannel> ();// Use BridgeChannel for prototype. Will develop P4 Channel in the future.
    p4Model = new P4Model;
    // nf interface
    char * a3;
    if(networkFunc.compare("firewall")==0) 
       a3= (char*) &"/home/kp/user/ns-allinone-3.26/ns-3.26/src/ns4/test/firewall/firewall.json"[0u];
    else{
        if(networkFunc.compare("simple")==0)
        {
            a3=(char*) &"/home/kp/user/ns-allinone-3.26/ns-3.26/src/ns4/test/simple/simple.json"[0u];
        }
        else
        {
            if(networkFunc.compare("l2_switch")==0)
            {
                a3=(char*) &"/home/kp/user/ns-allinone-3.26/ns-3.26/src/ns4/test/l2_switch/l2_switch.json"[0u];
            }
            else
            {
                if(networkFunc.compare("router")==0)
                {
                    //a3=(char*) &" --thrift-port 9091 /home/kp/user/ns-allinone-3.26/ns-3.26/src/ns4/test/router/router.json"[0u];
                    a3=(char*) &"/home/kp/user/ns-allinone-3.26/ns-3.26/src/ns4/test/router/router.json"[0u];
                }
            }
        }
    }
    // TO modify thrift_port

    /*char parms[200]="--thrift-port ";
    //char parms[200]="pcap";
    int thrift_port_num=9090+switchIndex;
    char* thrift_port_str;
    //itoa(thrift_port_num,thrift_port_str,10);
    thrift_port_str=int_to_str(thrift_port_num);
    //strcpy(parms,a3);
    strcat(parms,thrift_port_str);
    //NS_LOG_LOGIC("parms: "<<parms);
    strcat(parms," ");
    strcat(parms,a3);
    NS_LOG_LOGIC("parms: "<<parms);*/
    // Usage: SWITCH_NAME [options] <path to JSON config file>
    // --thrift-port arg        TCP port on which to run the Thrift runtime server
    char * args[2] = { NULL,a3};
    //char * args[2] = { NULL, a3 };
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
        pre(new bm::McSimplePre()) {

    add_component<bm::McSimplePre>(pre);

    argParser = new bm::TargetParserBasic();
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

/*int init_from_command_line_options(
      int argc, char *argv[],
      TargetParserIface *tp = nullptr,
      std::shared_ptr<TransportIface> my_transport = nullptr,
      std::unique_ptr<DevMgrIface> my_dev_mgr = nullptr);*/
int P4Model::init(int argc, char *argv[]) {
    NS_LOG_FUNCTION(this);
    //NS_LOG_LOGIC("argc: "<<argc);
    //for(int i=0;i<argc;i++)
        //std::cout<<argv[i]<<std::endl;
    //NS_LOG_LOGIC("switchIndex: "<<switchIndex);
    this->init_from_command_line_options(argc, argv, argParser);
    int thrift_port = this->get_runtime_port();

    //NS_LOG_LOGIC("thrift_port: "<<thrift_port);

    bm_runtime::start_server(this, thrift_port);
    //exec("python ");
    using ::sswitch_runtime::SimpleSwitchIf;
    using ::sswitch_runtime::SimpleSwitchProcessor;
    std::cout << "\nNight is coming. Sleep for 5 seconds.\n";
    std::this_thread::sleep_for(std::chrono::seconds(5));
    //bm_runtime::add_service<SimpleSwitchIf, SimpleSwitchProcessor>("simple_switch", sswitch_runtime::get_handler(this));
    return 0;
}

struct ns3PacketAndPort * P4Model::receivePacket(
        struct ns3PacketAndPort *ns3packet) {
    struct bm2PacketAndPort * bm2packet = ns3tobmv2(ns3packet);
    std::unique_ptr<bm::Packet> packet = std::move(bm2packet->packet);

    if (packet) {
        int port_num = bm2packet->port_num;
        int len = packet.get()->get_data_size();
        packet.get()->set_ingress_port(port_num);
        bm::PHV *phv = packet.get()->get_phv();
        phv->reset_metadata();
        phv->get_field("standard_metadata.ingress_port").set(port_num);
        phv->get_field("standard_metadata.packet_length").set(len);
        //Field &f_instance_type = phv->get_field("standard_metadata.instance_type");

        if (phv->has_field("intrinsic_metadata.ingress_global_timestamp")) {
            phv->get_field("intrinsic_metadata.ingress_global_timestamp").set(0);
        }

        // Ingress
        bm::Parser *parser = this->get_parser("parser");
        bm::Pipeline *ingress_mau = this->get_pipeline("ingress");
        phv = packet.get()->get_phv();

        parser->parse(packet.get());

        ingress_mau->apply(packet.get());

        packet->reset_exit();

        bm::Field &f_egress_spec = phv->get_field("standard_metadata.egress_spec");
        int egress_port = f_egress_spec.get_int();

        // Egress
        bm::Deparser *deparser = this->get_deparser("deparser");
        bm::Pipeline *egress_mau = this->get_pipeline("egress");
        f_egress_spec = phv->get_field("standard_metadata.egress_spec");
        f_egress_spec.set(0);
        egress_mau->apply(packet.get());
        deparser->deparse(packet.get());

        // Build return value
        struct bm2PacketAndPort* outPacket = new struct bm2PacketAndPort;
        outPacket->packet = std::move(packet);
        outPacket->port_num = egress_port;
        return bmv2tons3(outPacket);
    }
    return NULL;
}

struct ns3PacketAndPort * P4Model::bmv2tons3(
        struct bm2PacketAndPort *bm2packet) {
    // NS_LOG_FUNCTION(this);
    struct ns3PacketAndPort * ret = new struct ns3PacketAndPort;
    // Extract and set buffer
    void *buffer = bm2packet->packet.get()->data();
    size_t length = bm2packet->packet.get()->get_data_size();
    ret->packet = new ns3::Packet((uint8_t*)buffer, length);
    // Extract and set port number
    ret->port_num = bm2packet->port_num;
    // Set packet size
    return ret;
}

struct bm2PacketAndPort * P4Model::ns3tobmv2(
        struct ns3PacketAndPort *ns3packet) {
    int length = ns3packet->packet->GetSize();
    uint8_t* buffer = new uint8_t[length];

    struct bm2PacketAndPort* ret = new struct bm2PacketAndPort;

    int port_num = ns3packet->port_num;
    if (ns3packet->packet->CopyData(buffer, length)) {
        std::unique_ptr<bm::Packet> packet_ = new_packet_ptr(port_num, pktID++,
            length, bm::PacketBuffer(2048, (char*)buffer, length));
        ret->packet = std::move(packet_);
    }
    ret->port_num = port_num;
    return ret;
}
