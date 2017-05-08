#ifndef NS4_NS4_MODEL_H_H
#define NS4_NS4_MODEL_H_H
#include "ns3/log.h"
#include <ns3/bridge-net-device.h>
#include <ns3/net-device.h>
#include <ns3/ptr.h>
#include <ns3/mac48-address.h>
#include <memory>
#include "ns3/object.h"
#include "ns3/packet.h"
#include <bm/bm_sim/queue.h>
#include <bm/bm_sim/queueing.h>
#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/switch.h>
#include <bm/bm_sim/event_logger.h>
#include <bm/bm_sim/simple_pre_lag.h>
#include <bm/bm_sim/parser.h>
#include <bm/bm_sim/tables.h>
#include <bm/bm_sim/logger.h>
#include <bm/bm_sim/switch.h>
#include <chrono>




using namespace ns3;
#define MAXSIZE 100000


class P4Model : public bm::Switch{
public:
	static TypeId GetTypeId (void);
	int receive_(int port_num, const char *buffer, int len) {return 0;}
	void start_and_return_(){}
	struct ns3PacketAndPort * receivePacket(struct ns3PacketAndPort *ns3packet);
	int init(int argc, char *argv[]);
	P4Model();
private:

	struct bm2PacketAndPort * ns3tobmv2(struct ns3PacketAndPort *);
	struct ns3PacketAndPort * bmv2tons3(struct bm2PacketAndPort *);
	int pktID = 0;
    using clock = std::chrono::high_resolution_clock;
    bm::TargetParserBasic * argParser;

};
struct ns3PacketAndPort{
	int port_num;
	ns3::Packet * packet;
};

struct bm2PacketAndPort{
	int port_num;
	bm::Packet * packet;
};


class P4NetDevice :public BridgeNetDevice{
public:
	static TypeId GetTypeId (void);
	P4NetDevice();
	int ReceiveFromDevice(Ptr<NetDevice> device, Ptr<ns3::Packet> packet, uint16_t protocol,
	        Address const &source, Address const &destination, PacketType packetType);
private:
	P4Model* p4Model ;
	int GetPortNumber(Ptr<NetDevice>);
};


#endif
