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
class P4Model : public bm::Switch{
public:
	/**
	 * \brief Get the type ID.
	 * \return the object TypeId
	 */
	static TypeId GetTypeId (void);

	/**
	 * \brief a function from bm which will be called every time a
	 * packet is received.
	 *
	 * Since we are not letting the bm switch really work in a
	 * simulation enviornment. Instead we just borrow its processing pipeline,
	 * which means this receive_() will never be called, so we just return 0.
	 */
	int receive_(int port_num, const char *buffer, int len) {return 0;}

	/**
	 * \brief a function from bm called to initialize the P4 device.
	 *
	 * Never called either for the same reason with receive_()
	 */
	void start_and_return_(){}


	/**
	 * \brief Process a packet using P4 pipeline. Called every time
	 * there is a packet needing processing from P4 Device.
	 *
	 * \param ns3packet Original ns-3 packet and input port from P4 Device
	 * \return Processed ns-3 packet to be put back to P4 Device for
	 * transmission.
	 */
	struct ns3PacketAndPort * receivePacket(struct ns3PacketAndPort *ns3packet);


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
	struct bm2PacketAndPort * ns3tobmv2(struct ns3PacketAndPort * ns3packet);



	/**
	 * \brief Transform a bm::packet and a ns::packet
	 *
	 * Called when putting a packet back to the P4 Device.
	 */
	struct ns3PacketAndPort * bmv2tons3(struct bm2PacketAndPort *);

	/**
	 * \brief Packet ID
	 */
	int pktID = 0;


    using clock = std::chrono::high_resolution_clock;



    /**
     * \brief Structure of parsers
     */
    bm::TargetParserBasic * argParser;



    /**
     * A simple, 2-level, packet replication engine,
     * configurable by the control plane.
     */
	std::shared_ptr<bm::McSimplePre> pre;
};

/**
 * \brief Structure of a ns packet and port. An inelegant way of representing
 *  both packets along with its metadata.
 *
 * \TODO Find a better way.
 */
struct ns3PacketAndPort{
	int port_num;
	Packet * packet;
};

/**
 * \brief Structure of a bm packet and port. An inelegant way of representing
 *  both packets along with its metadata.
 *
 * \TODO Find a better way.
 */
struct bm2PacketAndPort{
	int port_num;
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
class P4NetDevice :public BridgeNetDevice{
public:
	/**
	 * \brief Add a port connected to the P4 target.
	 * @param
	 */
	void AddBridgePort(Ptr<NetDevice> bridgePort);

	/**
	 * \brief Get TypeId
	 */
	static TypeId GetTypeId (void);

	P4NetDevice();


	/**
	 * \brief Callback function when a new packet is received from one of
	 * underlying channels. It sends the packet to P4 Model for modification
	 * and sends it out.
	 *
	 */
	void ReceiveFromDevice(Ptr<ns3::NetDevice> device, Ptr<const ns3::Packet> packet, uint16_t protocol,
	        Address const &source, Address const &destination, PacketType packetType);
private:
	P4Model* p4Model ;
	int GetPortNumber(Ptr<NetDevice>);
};


#endif
