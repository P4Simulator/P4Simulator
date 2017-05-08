#include "p4-net-device.h"
#include "ns3/log.h"
#include <bm/bm_sim/switch.h>

using namespace ns3;
NS_OBJECT_ENSURE_REGISTERED(P4NetDevice);
NS_OBJECT_ENSURE_REGISTERED(P4Model);

NS_LOG_COMPONENT_DEFINE ("P4NetDevice");
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
int P4NetDevice::ReceiveFromDevice(Ptr<NetDevice> device, Ptr<ns3::Packet> packet, uint16_t protocol,
        Address const &source, Address const &destination, PacketType packetType){
	int port_num = GetPortNumber(device);
	struct ns3PacketAndPort *ns3packet = new (struct ns3PacketAndPort);
	ns3packet->port_num = port_num;
	ns3packet->packet = packet.operator ->();
	struct ns3PacketAndPort * egress_packetandport = p4Model->receivePacket(ns3packet);
	Ptr<ns3::Packet> egress_packet = egress_packetandport->packet;
	int egress_port_num = egress_packetandport->port_num;
	Ptr<NetDevice>outNetDevice = GetBridgePort(egress_port_num);
	Address * tmp_address = new Address;
	int result = outNetDevice->Send(packet,*tmp_address,0);
	return result;
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
	p4Model->init(0,0);
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
	bm::Packet *packet = bm2packet->packet;
	int port_num = bm2packet->port_num;
	int len = packet->get_data_size();
	packet->set_ingress_port(port_num);
    bm::PHV *phv = packet->get_phv();
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
    phv = packet->get_phv();

    parser->parse(packet);

    ingress_mau->apply(packet);

    packet->reset_exit();


    bm::Field &f_egress_spec = phv->get_field("standard_metadata.egress_spec");
    int egress_port = f_egress_spec.get_int();

    // Egress
    bm::Deparser *deparser = this->get_deparser("deparser");
    bm::Pipeline *egress_mau = this->get_pipeline("egress");
    f_egress_spec = phv->get_field("standard_metadata.egress_spec");
    f_egress_spec.set(0);
    egress_mau->apply(packet);
    deparser->deparse(packet);

    // Build return value
    struct bm2PacketAndPort * outPacket = new struct bm2PacketAndPort;
    outPacket->packet = packet;
    outPacket->port_num = egress_port;
    return bmv2tons3(outPacket);

}



struct ns3PacketAndPort * P4Model::bmv2tons3(struct bm2PacketAndPort *bm2packet){
	struct ns3PacketAndPort * ret = new struct ns3PacketAndPort;
	// Extract and set buffer
	void *buffer_start = bm2packet->packet->data();
	size_t buffer_length = bm2packet->packet->get_data_size();
	ret->packet = new ns3::Packet((unsigned char *)buffer_start,buffer_length);
	// Extract and set port number
	ret->port_num = bm2packet->port_num;
	// Set packet size
	return ret;
}

struct bm2PacketAndPort * P4Model::ns3tobmv2(struct ns3PacketAndPort *ns3packet){
	struct bm2PacketAndPort * ret = new struct bm2PacketAndPort;
	int len = ns3packet->packet->GetSize();
	int port_num = ns3packet->port_num;
	void * buffer = new uint8_t *[sizeof(uint8_t)*MAXSIZE];
	if (ns3packet->packet->Serialize((uint8_t *)buffer,MAXSIZE)){
		 std::unique_ptr<bm::Packet> packet_= new_packet_ptr(port_num, pktID++, len,
				bm::PacketBuffer(len + 512, (char *)buffer, len));
		 ret->packet = packet_.get();
	}
	ret-> port_num = port_num;
	return ret;
}


