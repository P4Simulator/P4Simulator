#include "p4-net-device.h"
#include "ns3/log.h"
#include <bm/bm_sim/switch.h>
#include "ns3/node.h"
#include <memory.h>
using namespace ns3;
NS_OBJECT_ENSURE_REGISTERED(P4NetDevice);
NS_OBJECT_ENSURE_REGISTERED(P4Model);

NS_LOG_COMPONENT_DEFINE ("P4NetDevice");

void
P4NetDevice::AddBridgePort (Ptr<NetDevice> bridgePort)
{
  //NS_LOG_FUNCTION_NOARGS ();
 // NS_ASSERT (bridgePort != this);
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

 // NS_LOG_DEBUG ("RegisterProtocolHandler for " << bridgePort->GetInstanceTypeId ().GetName ());
  m_node->RegisterProtocolHandler (MakeCallback (&P4NetDevice::ReceiveFromDevice, this),
                                   0, bridgePort, true);
  m_ports.push_back (bridgePort);
  m_channel->AddChannel (bridgePort->GetChannel ());
}


TypeId P4NetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::P4NetDevice")
    .SetParent<Object> ()
    .SetGroupName ("Network")
    .AddConstructor<P4NetDevice> ()

    ;
  return tid;
}
TypeId P4Model::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::P4Model")
    .SetParent<Object> ()
    .SetGroupName ("Network")
   // .AddConstructor<P4Model> ()

    ;
  return tid;
}
void
P4NetDevice::ReceiveFromDevice(Ptr<ns3::NetDevice> device, Ptr<const ns3::Packet> packet_in, uint16_t protocol,
        Address const &source, Address const &destination, PacketType packetType){
	int port_num = GetPortNumber(device);
	//int port_num = device->get;
	struct ns3PacketAndPort *ns3packet = new (struct ns3PacketAndPort);
	ns3packet->port_num = port_num;
	ns3packet->packet = packet_in->Copy().operator ->();
	struct ns3PacketAndPort * egress_packetandport = p4Model->receivePacket(ns3packet);
	Ptr<ns3::Packet> egress_packet = egress_packetandport->packet;
	int egress_port_num = egress_packetandport->port_num;
	Ptr<NetDevice>outNetDevice = GetBridgePort(egress_port_num);
	Address * tmp_address = new Address;
	outNetDevice->Send(egress_packet,*tmp_address,0);
}


int P4NetDevice::GetPortNumber(Ptr<NetDevice> port){
	int ports_num = GetNBridgePorts();
	for (int i = 0;i<ports_num;i++){
		if (GetBridgePort(i)==port)
			return i;
	}
	return -1;
}


P4NetDevice::P4NetDevice(){
	NS_LOG_FUNCTION_NOARGS ();
	p4Model = new P4Model;
	//char * a1 =(char*) &("--thrift-port"[0u]);
	//char * a2 =(char*) &"9091"[0u];
	char * a3 =(char*) &"/home/mark/workspace/l2_switch.json"[0u];
	char * args[2] = {NULL,a3};
	p4Model->init(2,args);
	NS_LOG_LOGIC("A P4 Netdevice was initialized.");
	}



#define MAXSIZE 100000

P4Model::P4Model(){
	argParser = new bm::TargetParserBasic();
}


int P4Model::init(int argc, char *argv[]){
    this->init_from_command_line_options(argc, argv, argParser);
    return 0;
}




struct ns3PacketAndPort * P4Model::receivePacket(struct ns3PacketAndPort *ns3packet){
	struct bm2PacketAndPort * bm2packet= ns3tobmv2(ns3packet);
	std::unique_ptr<bm::Packet> packet = std::move(bm2packet->packet);
	int port_num = bm2packet->port_num;
	int len = packet.get()->get_data_size();
	packet.get()->set_ingress_port(port_num);
    bm::PHV *phv = packet.get()->get_phv();
    phv->reset_metadata();
    phv->get_field("standard_metadata.ingress_port").set(port_num);
    phv->get_field("standard_metadata.packet_length").set(len);
    //Field &f_instance_type = phv->get_field("standard_metadata.instance_type");

    if (phv->has_field("intrinsic_metadata.ingress_global_timestamp")) {
        phv->get_field("intrinsic_metadata.ingress_global_timestamp")
                .set(0);
    }

    // Ingress
    bm::Parser *parser = this->get_parser("parser");
    bm::Pipeline *ingress_mau = this->get_pipeline("ingress");
    phv = packet.get()->get_phv();

    parser->parse(packet.get());

    ingress_mau->apply(packet.get());

    packet.get()->reset_exit();


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
    struct bm2PacketAndPort * outPacket = new struct bm2PacketAndPort;
    outPacket->packet = std::move(packet);
    outPacket->port_num = egress_port;
    return bmv2tons3(outPacket);

}



struct ns3PacketAndPort * P4Model::bmv2tons3(struct bm2PacketAndPort *bm2packet){
	struct ns3PacketAndPort * ret = new struct ns3PacketAndPort;
	// Extract and set buffer
	void *buffer_start = bm2packet->packet.get()->data();
	size_t buffer_length = bm2packet->packet.get()->get_data_size();
	ret->packet = new ns3::Packet((unsigned char *)buffer_start,buffer_length);
	// Extract and set port number
	ret->port_num = bm2packet->port_num;
	// Set packet size
	return ret;
}

struct bm2PacketAndPort * P4Model::ns3tobmv2(struct ns3PacketAndPort *ns3packet){
	void * buffer = new uint8_t *[sizeof(uint8_t)*MAXSIZE];
	ns3packet->packet->SetNixVector(NULL);
	struct bm2PacketAndPort * ret = new struct bm2PacketAndPort;
	int len = ns3packet->packet->GetSize();
	int port_num = ns3packet->port_num;
	if (ns3packet->packet->Serialize((uint8_t *)buffer,MAXSIZE)){
		 std::unique_ptr<bm::Packet> packet_= new_packet_ptr(port_num, pktID++, len,
				bm::PacketBuffer(len + 512, (char *)buffer, len));
		 ret->packet = std::move(packet_);
	}
	ret-> port_num = port_num;
	return ret;
}
