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
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
using namespace ns3;

#define MAXSIZE 512

std::string networkFunc; // define in p4-example.cc,to select nf
int switchIndex; //define in p4-example.cc, to decide thrift_port
std::string flowtable_path;//define in p4-example.cc

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
    /*int buSize = packet_in->GetSize();
    uint8_t* bu = new uint8_t [buSize];
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
    //NS_LOG_LOGIC("after copy");
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
    //for (int i = 34; i < 38&&i<bS; i ++) 
    for (int i = 0; i < bS; i ++)
        std::cout << std::setfill('0') << std::setw(2) << std::hex << (int)b[i] << ' ';
    std::cout << '\n';
    NS_LOG_LOGIC("add Headers");*/
    
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
    using ::sswitch_runtime::SimpleSwitchIf;
    using ::sswitch_runtime::SimpleSwitchProcessor;
    NS_LOG_LOGIC("switchIndex: "<<switchIndex);
    std::string populate_flowtable_type;// runtime_CLI  local_call
    populate_flowtable_type="local_call";
    //populate_flowtable_type="runtime_CLI";
    // ************************************************************
    // use local call to populate flowtable
    if(populate_flowtable_type.compare("local_call")==0)
    {
        this->my_init_from_command_line_options(argc, argv, argParser);
        std::string flowtable_parentdir("/home/kp/user/ns-allinone-3.26/ns-3.26/src/ns4/test/flowtable_dir/");
        std::string childdir("router_flowtable");
        std::string flowtable_name("/command1.txt");
        //populate_flow_table(flowtable_path);
        populate_flow_table(flowtable_parentdir+childdir+flowtable_name);
        //**********************************************
        std::vector<bm::MatchTable::Entry> entries=mt_get_entries(0,std::string("arp_nhop"));
        std::cout<<"arp_nhop entry num:"<<entries.size()<<std::endl;
        for(size_t i=0;i<entries.size();i++);
        //**********************************************
         //**********************************************
        std::vector<bm::MatchTable::Entry> entries1=mt_get_entries(0,std::string("forward_table"));
        std::cout<<"forward_table entry num:"<<entries1.size()<<std::endl;
        for(size_t i=0;i<entries1.size();i++);
        //**********************************************
         //**********************************************
        std::vector<bm::MatchTable::Entry> entries2=mt_get_entries(0,std::string("send_frame"));
        std::cout<<"send_frame entry num:"<<entries2.size()<<std::endl;
        for(size_t i=0;i<entries2.size();i++);
        //**********************************************
    }
    // **************************************************************
    else
    {
        // ******************************TO DO*************************************
        // start thrift server , use runtime_CLI populate flowtable
        // have problem in modify default thrift port
        if(populate_flowtable_type.compare("runtime_CLI")==0)
        {
            this->init_from_command_line_options(argc, argv, argParser);
            int thrift_port = this->get_runtime_port();
            //NS_LOG_LOGIC("thrift_port: "<<thrift_port);
            bm_runtime::start_server(this, thrift_port);
            //exec("python ");
            std::cout << "\nNight is coming. Sleep for 5 seconds.\n";
            std::this_thread::sleep_for(std::chrono::seconds(5));
            //bm_runtime::add_service<SimpleSwitchIf, SimpleSwitchProcessor>("simple_switch", sswitch_runtime::get_handler(this));
        }
        //*********************************************************************
    }
    return 0;
}

int P4Model::my_init_from_command_line_options(int argc, char *argv[],bm::TargetParserBasic *tp)
{
    NS_LOG_FUNCTION(this);
    bm::OptionsParser parser;
    parser.parse(argc,argv,tp);
    std::shared_ptr<bm::TransportIface> transport=nullptr;
    int status = 0;
    if (transport == nullptr) {
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
    status = init_objects(parser.config_file_path, parser.device_id, transport);
    return status;
}

/*void P4Model::populate_flow_table(std::string command_path)
{
    std::fstream fp;
    fp.open(command_path);
    if(!fp)
    {
        std::cout<<command_path<<" can't open."<<std::endl;
    }
    else
    {
        bm::ActionData action_data;
        mt_set_default_action(0,std::string("arp_nhop"),std::string("_drop"),action_data);
    }
}*/
void P4Model::populate_flow_table(std::string command_path)
{
    //get switch all table name
    //get key field
    //according to table name get all action name
    //according to action name get parm num and type
    //without test parms error

    std::fstream fp;
    fp.open(command_path);
    if (!fp)
    {
        std::cout <<"in P4Model::populate_flow_table, "<< command_path << " can't open." << std::endl;
    }
    else
    {
        const int max_size=500;
        char row[max_size];
        while (fp.getline(row, max_size))
        {
            //std::cout << row << std::endl;
            parse_flowtable_command(std::string(row));
        }
    }
}
void P4Model::parse_flowtable_command(std::string command_row)
{
    std::vector<std::string> parms;
    int last_p = 0, cur_p = 0;
    //int action_end = command_row.find("=>");
    //if(action_end<command_row.size)
    for (size_t i = 0; i < command_row.size(); i++,cur_p++)
    {
        if (command_row[i] == ' ')
        {
            parms.push_back(command_row.substr(last_p, cur_p - last_p));
            last_p = i + 1;
        }
    }
    if (last_p < cur_p)
    {
        parms.push_back(command_row.substr(last_p, cur_p - last_p));
    }
    for (size_t i = 0; i < parms.size(); i++)
        std::cout << parms[i] << " ";
    std::cout<<std::endl;
    if (parms.size() > 0)
    {
        if (parms[0].compare("table_set_default") == 0)
        {
            //table_set_default ipv4_nhop _drop
            //        table_name action_name action_data
            bm::ActionData action_data;
            if (parms.size() > 3)
            {
                for (size_t i = 3; i < parms.size(); i++)
                    action_data.push_back_action_data(str_to_int(parms[i]));
                 //void push_back_action_data(const Data &data)
                 ///usr/local/include/bm/bm_sim/actions.h 
                //build action_data
            }
            mt_set_default_action(0, parms[1], parms[2], action_data);
        }
        else
        {
            if (parms[0].compare("table_add") == 0)
            {
                //table_add ipv4_nhop set_nhop 0x0a010104 => 0x0a0a0a0a
                //table_name action_name match_key=>action_data
                // need to match type
                NS_LOG_LOGIC("come to table_add");
                std::vector<bm::MatchKeyParam> match_key;
                bm::ActionData action_data;
                bm::entry_handle_t *handle=new bm::entry_handle_t(1);// table entry num
                /*
                 *enum class Type {
                 *              RANGE,
                 *              VALID,
                 *              EXACT,
                 *              LPM,
                 *              TERNARY
                 *             };
                 */
                //*************************TO DO**********************************
                // should accord to p4 json file to decide every table key match type
                // now use EXACT temporarily 
                bm::MatchKeyParam::Type match_type=bm::MatchKeyParam::Type::EXACT;
                //****************************************************************
                unsigned int key_num=0;
                unsigned int action_data_num=0;
                size_t i;
                for(i=3;i<parms.size();i++)
                {
                    if(parms[i].compare("=>")!=0)
                    {
                        key_num++;
                        match_key.push_back(bm::MatchKeyParam(match_type,parms[i]));
                        NS_LOG_LOGIC("match_key:"<<" "<<parms[i]);
                    }
                    else
                        break;
                }
                i++;
                int priority;
                //judge key_num equal table need key num
                if(match_type!=bm::MatchKeyParam::Type::TERNARY&&match_type!=bm::MatchKeyParam::Type::RANGE)
                {
                    for(;i<parms.size();i++)
                    {
                        action_data_num++;
                        action_data.push_back_action_data(str_to_int(parms[i]));
                        NS_LOG_LOGIC("action_data:"<<parms[i]);
                    }
                    priority=0;
                    // judge action_data_num equal action need num
                     mt_add_entry(0, parms[1], match_key, parms[2], action_data, handle,priority);

                     //******************************************************
                      std::cout<<"table_name:"<<parms[1]<<std::endl;
                      std::cout<<"match_key num:"<<match_key.size()<<std::endl;
                      std::cout<<"action_name:"<<parms[2]<<std::endl;
                      std::cout<<"action_data num:"<<action_data.size()<<std::endl;
                      std::cout<<"entry_handle:"<<*handle<<std::endl;
                     //******************************************************
                }
                else
                {
                    for(;i<parms.size()-1;i++)
                    {
                        action_data_num++;
                        action_data.push_back_action_data(str_to_int(parms[i]));
                    }
                    // judge action_data_num equal action need num
                    priority=str_to_int(parms[parms.size()-1]);
                    mt_add_entry(0, parms[1], match_key, parms[2], action_data, handle, priority);
                }
            }
        }
    }
}
unsigned int P4Model::str_to_int(const std::string str)
{
    unsigned int res = 0;
    if (str.find("0x") < str.size())//16
    {
        for (size_t i = 2; i < str.size(); i++)
        {
            if (str[i] >= '0'&&str[i] <= '9')
            {
                res = res * 16 + str[i] - '0';
            }
            else
            {
                if (str[i] >= 'a'&&str[i] <= 'f')
                {
                    res = res * 16 + str[i] - 'a'+10;
                }
                else
                {
                    std::cout << "in P4Model::str_to_int, action data error!" << std::endl;
                }
            }
        }
    }
    else
    {
        if (str.find("0b") < str.size())//2
        {
            for (size_t i = 2; i < str.size(); i++)
            {
                if (str[i] >= '0'&&str[i] <= '1')
                {
                    res = res * 2 + str[i] - '0';
                }
                else
                {
                    std::cout << "in P4Model::str_to_int, action data error!" << std::endl;
                }
            }
        }
        else //10
        {
            for (size_t i = 0; i < str.size(); i++)
            {
                if (str[i] >= '0'&&str[i] <= '9')
                {
                    res = res * 10 + str[i] - '0';
                }
                else
                {
                    std::cout << "in P4Model::str_to_int, action data error!" << std::endl;
                }
            }
        }
    }
    return res;
}

struct ns3PacketAndPort * P4Model::receivePacket(
        struct ns3PacketAndPort *ns3packet) {
    NS_LOG_FUNCTION(this);
    struct bm2PacketAndPort * bm2packet = ns3tobmv2(ns3packet);
    NS_LOG_LOGIC("ns3tobmv2");
    std::unique_ptr<bm::Packet> packet = std::move(bm2packet->packet);

    if (packet) {
        //NS_LOG_LOGIC("packet");
        int port_num = bm2packet->port_num;
        int len = packet.get()->get_data_size();
        packet.get()->set_ingress_port(port_num);
        bm::PHV *phv = packet.get()->get_phv();
        phv->reset_metadata();
        //NS_LOG_LOGIC("reset_metadata");
        phv->get_field("standard_metadata.ingress_port").set(port_num);
        phv->get_field("standard_metadata.packet_length").set(len);
        //Field &f_instance_type = phv->get_field("standard_metadata.instance_type");

        if (phv->has_field("intrinsic_metadata.ingress_global_timestamp")) {
            phv->get_field("intrinsic_metadata.ingress_global_timestamp").set(0);
        }
        //NS_LOG_LOGIC("come to ingress");

        // Ingress
        bm::Parser *parser = this->get_parser("parser");
        bm::Pipeline *ingress_mau = this->get_pipeline("ingress");
        phv = packet.get()->get_phv();
        //NS_LOG_LOGIC("come to parse");

        parser->parse(packet.get());
        //NS_LOG_LOGIC("come to apply");

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
    //NS_LOG_FUNCTION(this);
    struct ns3PacketAndPort * ret = new struct ns3PacketAndPort;
    // Extract and set buffer
    void *buffer = bm2packet->packet.get()->data();
    size_t length = bm2packet->packet.get()->get_data_size();
    //NS_LOG_LOGIC("get buffer and length");
    ret->packet = new ns3::Packet((uint8_t*)buffer, length);
    // Extract and set port number
    ret->port_num = bm2packet->port_num;
    // Set packet size
    return ret;
}

struct bm2PacketAndPort * P4Model::ns3tobmv2(
        struct ns3PacketAndPort *ns3packet) {
    //NS_LOG_FUNCTION(this);
    int length = ns3packet->packet->GetSize();
    uint8_t* buffer = new uint8_t[length];
    // view ns3packet
    // ****************************************
    /*int buSize = ns3packet->packet->GetSize();
    uint8_t* bu = new uint8_t [buSize];
    ns3packet->packet->CopyData(bu, buSize);
    for (int i = 0; i < buSize; i ++) 
        std::cout << std::setfill('0') << std::setw(2) << std::hex << (int)bu[i] << ' ';
    std::cout << '\n';*/
    // *************************************

    struct bm2PacketAndPort* ret = new struct bm2PacketAndPort;

    int port_num = ns3packet->port_num;
    //NS_LOG_LOGIC("port set");
    if (ns3packet->packet->CopyData(buffer, length)) {
        //NS_LOG_LOGIC("CopyData"<<port_num);
        std::unique_ptr<bm::Packet> packet_ = new_packet_ptr(port_num, pktID++,
            length, bm::PacketBuffer(2048, (char*)buffer, length));
        ret->packet = std::move(packet_);
    }
    //NS_LOG_LOGIC("copy_data");
    ret->port_num = port_num;
    return ret;
}
