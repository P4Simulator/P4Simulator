#include "p4-net-device.h"
#include "helper.h"
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
#include <math.h>
#include <fstream>
#include <map>
using namespace ns3;

#define MAXSIZE 512
typedef std::map<std::string,bm::MatchKeyParam::Type> StrMatchType;
std::string network_func; // define in p4-example.cc,to select nf
std::string flowtable_path;//define in p4-example.cc
std::map<std::string,bm::MatchKeyParam::Type> flowtable_matchtype;
std::string flowtable_matchtype_path;
std::string populate_flowtable_type;
std::string home_path;
std::string ns3_root_name;
std::string ns3_src_name;

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
    //view received packet real content
    int pkgSize = packet_in->GetSize();
    NS_LOG_LOGIC("received packet_in size:"<<pkgSize);
    packet_in->EnablePrinting();
    packet_in->Print(std::cout);
    std::cout<<std::endl;

    Mac48Address src48 = Mac48Address::ConvertFrom (source);
    Mac48Address dst48 = Mac48Address::ConvertFrom (destination);

    int port_num = GetPortNumber(device);
    struct ns3PacketAndPort *ns3packet = new (struct ns3PacketAndPort);
    ns3packet->port_num = port_num;
    ns3packet->packet=(ns3::Packet*)packet_in.operator ->();
    EthernetHeader eeh;
    eeh.SetDestination(dst48);
    eeh.SetSource(src48);
    eeh.SetLengthType(protocol);

    ns3packet->packet->AddHeader(eeh);
    
    struct ns3PacketAndPort* egress_packetandport = p4Model->receivePacket(ns3packet);

    if (egress_packetandport) {
        egress_packetandport->packet->RemoveHeader(eeh);
        int egress_port_num = egress_packetandport->port_num;
        // judge whether packet drop
        if(egress_port_num!=511){
          NS_LOG_LOGIC("egress_port_num: "<<egress_port_num);
          Ptr<NetDevice> outNetDevice = GetBridgePort(egress_port_num);
          outNetDevice->Send(egress_packetandport->packet->Copy(), destination, protocol);
          }
        else
            std::cout<<"drop packet!\n";
    } 
    else 
        std::cout << "Null packet!\n";
}

int P4NetDevice::GetPortNumber(Ptr<NetDevice> port) {
    int ports_num = GetNBridgePorts();
    for (int i = 0; i < ports_num; i++) {
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
    std::string json_path;
    if(network_func.compare("firewall")==0) 
       {
	//a3= (char*) &"/home/kphf1995cm/ns-allinone-3.26/ns-3.26/src/ns4/test/firewall/firewall.json"[0u];
         json_path=home_path+ns3_root_name+ns3_src_name+"src/ns4/test/firewall/firewall.json";
         a3=(char*)json_path.data();

	}
    if(network_func.compare("silkroad")==0)
       {//a3= (char*) &"/home/kphf1995cm/ns-allinone-3.26/ns-3.26/src/ns4/test/silkroad/silkroad.json"[0u];
	json_path=home_path+ns3_root_name+ns3_src_name+"src/ns4/test/silkroad/silkroad.json";
        a3=(char*)json_path.data();
	}
    if(network_func.compare("router")==0)
       {//a3= (char*) &"/home/kphf1995cm/ns-allinone-3.26/ns-3.26/src/ns4/test/router/router.json"[0u];
	NS_LOG_LOGIC("router json_path");
	json_path=home_path+ns3_root_name+ns3_src_name+"src/ns4/test/router/router.json";
	a3=(char*)json_path.data();
	}
    if(network_func.compare("simple_router")==0)
       {
	//a3= (char*) &"/home/kphf1995cm/ns-allinone-3.26/ns-3.26/src/ns4/test/simple_router/simple_router.json"[0u];
	json_path=home_path+ns3_root_name+ns3_src_name+"src/ns4/test/simple_router/simple_router.json";
        a3=(char*)json_path.data();

	}
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


int P4Model::init(int argc, char *argv[]) {

    NS_LOG_FUNCTION(this);
    using ::sswitch_runtime::SimpleSwitchIf;
    using ::sswitch_runtime::SimpleSwitchProcessor;
    
    // use local call to populate flowtable
    if(populate_flowtable_type.compare("local_call")==0)
    {
        this->my_init_from_command_line_options(argc, argv, argParser);
        read_table_action_match_type(flowtable_matchtype_path,flowtable_matchtype);
        populate_flow_table(flowtable_path);
        for(StrMatchType::iterator iter=flowtable_matchtype.begin();iter!=flowtable_matchtype.end();iter++)
	{  view_flowtable_entry_num(iter->first); }
    }
    else
    {
        // start thrift server , use runtime_CLI populate flowtable
        if(populate_flowtable_type.compare("runtime_CLI")==0)
        {
            this->init_from_command_line_options(argc, argv, argParser);
            int thrift_port = this->get_runtime_port();
            bm_runtime::start_server(this, thrift_port);
            std::cout << "\nNight is coming. Sleep for 5 seconds.\n";
            std::this_thread::sleep_for(std::chrono::seconds(5));
            //bm_runtime::add_service<SimpleSwitchIf, SimpleSwitchProcessor>("simple_switch", sswitch_runtime::get_handler(this));
        }
    }
    return 0;
}

void P4Model::view_flowtable_entry_num(std::string flowtable_name)
{
    std::vector<bm::MatchTable::Entry> entries=mt_get_entries(0,flowtable_name);
    std::cout<<flowtable_name<<" entry num:"<<entries.size()<<std::endl;
}

void P4Model::read_table_action_match_type(std::string file_path,std::map<std::string,bm::MatchKeyParam::Type>& tableaction_matchtype)
{

    std::ifstream topgen;
    topgen.open(file_path);

    if (!topgen.is_open())
    {
        std::cout <<file_path<< " can not open!"<< std::endl;
        abort();
    }

    std::string table_action;
    std::string match_type;

    std::istringstream lineBuffer;
    std::string line;

    while (getline(topgen, line))
    {
        lineBuffer.clear();
        lineBuffer.str(line);

        lineBuffer >> table_action >> match_type;
        if (match_type.compare("exact")==0)
        {
            tableaction_matchtype[table_action]=bm::MatchKeyParam::Type::EXACT;
        }
        else
        {
            if (match_type.compare("lpm")==0)
            {
                 tableaction_matchtype[table_action]=bm::MatchKeyParam::Type::LPM;
            }
            else
            {
                if (match_type.compare("ternary")==0)
                {
                     tableaction_matchtype[table_action]=bm::MatchKeyParam::Type::TERNARY;
                }
                else
                {
                    if (match_type.compare("valid")==0)
                    {
                         tableaction_matchtype[table_action]=bm::MatchKeyParam::Type::VALID;
                    }
                    else
                    {
                        if (match_type.compare("range")==0)
                        {
                             tableaction_matchtype[table_action]=bm::MatchKeyParam::Type::RANGE;
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

int P4Model::my_init_from_command_line_options(int argc, char *argv[],bm::TargetParserBasic *tp)
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


void P4Model::populate_flow_table(const std::string command_path)
{

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
            parse_flowtable_command(std::string(row));
        }
    }
}
void P4Model::parse_flowtable_command(const std::string command_row)
{

    std::vector<std::string> parms;
    int last_p = 0, cur_p = 0;
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
    if (parms.size() > 0)
    {
        if (parms[0].compare("table_set_default") == 0)
        {
          
            bm::ActionData action_data;
            if (parms.size() > 3)
            {
                for (size_t i = 3; i < parms.size(); i++)
                    action_data.push_back_action_data(bm::Data(parms[i]));
            }
            mt_set_default_action(0, parms[1], parms[2], action_data);
        }
        else
        {
            if (parms[0].compare("table_add") == 0)
            {
               
                std::vector<bm::MatchKeyParam> match_key;
                bm::ActionData action_data;
                bm::entry_handle_t *handle=new bm::entry_handle_t(1);// table entry num
                bm::MatchKeyParam::Type match_type=flowtable_matchtype[parms[1]];
                
                unsigned int key_num=0;
                unsigned int action_data_num=0;
                size_t i;
                for(i=3;i<parms.size();i++)
                {
                    if(parms[i].compare("=>")!=0)
                    {
                        key_num++;
                        if(match_type==bm::MatchKeyParam::Type::EXACT)
                        {
                            match_key.push_back(bm::MatchKeyParam(match_type,hexstr_to_bytes(parms[i])));
                        }
                        else
                        {
                            if(match_type==bm::MatchKeyParam::Type::LPM)
                            {
                                int pos=parms[i].find("/");
                                std::string prefix=parms[i].substr(0,pos);
                                std::string length=parms[i].substr(pos+1);
                                unsigned int prefix_length=str_to_int(length);
                                match_key.push_back(bm::MatchKeyParam(match_type,hexstr_to_bytes(parms[i],prefix_length),int(prefix_length)));
                            }
                            else
                            {
                                if(match_type==bm::MatchKeyParam::Type::TERNARY)
                                {
                                    int pos=parms[i].find("&&&");
                                    std::string key=hexstr_to_bytes(parms[i].substr(0,pos));
                                    std::string mask=hexstr_to_bytes(parms[i].substr(pos+3));
                                    if(key.size()!=mask.size())
                                    {
                                        std::cout<<"key and mask length unmatch!"<<std::endl;
                                        abort();
                                    }
                                    else
                                    {
                                        match_key.push_back(bm::MatchKeyParam(match_type,key,mask));
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
                if(match_type!=bm::MatchKeyParam::Type::TERNARY&&match_type!=bm::MatchKeyParam::Type::RANGE)
                {
                    for(;i<parms.size();i++)
                    {
                        action_data_num++;
                        action_data.push_back_action_data(bm::Data(parms[i]));
                    }
                    priority=0;
                    // judge action_data_num equal action need num
                     mt_add_entry(0, parms[1], match_key, parms[2], action_data, handle,priority);
                }
                else
                {
                    for(;i<parms.size()-1;i++)
                    {
                        action_data_num++;
                        action_data.push_back_action_data(bm::Data(parms[i]));
                    }
                    // judge action_data_num equal action need num
                    priority=str_to_int(parms[parms.size()-1]);
                    mt_add_entry(0, parms[1], match_key, parms[2], action_data, handle, priority);
                }
            }
        }
    }
}
/*unsigned int P4Model::str_to_int(const std::string str)
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
                    if(str[i]>='A'&&str[i]<='F')
                        res=res*16+str[i]-'A'+10;
                    else
                    {
                        std::cout << "in P4Model::str_to_int, action data error!" << std::endl;
                    }
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
}*/
/*int P4Model::hexchar_to_int(char c)
{

    int temp=0;
    if (c >= '0'&&c <= '9')
        temp = c - '0';
    else
    {
        if (c >= 'a'&&c <= 'f')
            temp = c - 'a' + 10;
        else
        {
            if(c>='A'&&c<='F')
                temp=c-'A'+10;
            else
            {
                std::cout << "str_to_bytes error" << std::endl;
            }
        }
    }
    return temp;
}*/
/*std::string P4Model::hexstr_to_bytes(const std::string str)
{

    std::string hex_str;
    if (str.find("0x") < str.size())
    {
        hex_str = str.substr(2);
    }
    else
    {
        hex_str = str;
    }
    std::string res;
    res.resize(hex_str.size() / 2);
    for (size_t i = 0,j=0; i < hex_str.size(); i += 2,j++)
    {
        res[j] = hexchar_to_int(hex_str[i]) * 16 + hexchar_to_int(hex_str[i + 1]);
    }
    return res;
}*/

/*std::string P4Model::hexstr_to_bytes(const std::string str, unsigned int bit_width)
{

    std::string hex_str;
    if (str.find("0x") < str.size())
    {
        hex_str = str.substr(2);
    }
    else
    {
        hex_str = str;
    }
    std::string res;
    res.resize(ceil(double(bit_width) / 8));
    std::cout<<res.size()<<std::endl;
    for (size_t i = 0, j = 0, w = 0; i<hex_str.size(); i += 2, j++, w += 8)
    {
        if (w + 8<bit_width)
        {
            res[j] = hexchar_to_int(hex_str[i]) * 16 + hexchar_to_int(hex_str[i + 1]);
        }
        else
        {
            if (w + 4 >= bit_width)
            {
                std::string binary_str(4, '0');
                size_t k = 3;
                int value = hexchar_to_int(hex_str[i]);
                while (value)
                {
                    binary_str[k--] = value % 2 + '0';
                    value /= 2;
                }
                res[j] = 0;
                unsigned int left_len = bit_width - w;
                for (size_t t = 0; t<left_len; t++)
                {
                    res[j] = res[j] * 2 + binary_str[t] - '0';
                }
                for (size_t t = left_len; t < 4; t++)
                    res[j] *= 2;
                res[j] *= 16;
            }
            else
            {
                res[j] = hexchar_to_int(hex_str[i]) * 16;
                std::string binary_str(4, '0');
                size_t k = 3;
                int value = hexchar_to_int(hex_str[i+1]);
                while (value)
                {
                    binary_str[k--] = value % 2 + '0';
                    value /= 2;
                }
                int temp = 0;
                unsigned int left_len = bit_width - w - 4;
                for (size_t t = 0; t<left_len; t++)
                {
                    temp = temp * 2 + binary_str[t] - '0';
                }
                for (size_t t = left_len; t < 4; t++)
                    temp *= 2;
                res[j] += temp;
            }
            break;
        }
    }
    return res;
}*/


struct ns3PacketAndPort * P4Model::receivePacket(
        struct ns3PacketAndPort *ns3packet) {
    NS_LOG_FUNCTION(this);
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
    struct ns3PacketAndPort * ret = new struct ns3PacketAndPort;
    void *buffer = bm2packet->packet.get()->data();
    size_t length = bm2packet->packet.get()->get_data_size();
    ret->packet = new ns3::Packet((uint8_t*)buffer, length);
    ret->port_num = bm2packet->port_num;
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
