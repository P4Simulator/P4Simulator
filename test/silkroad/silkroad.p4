header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type ipv4_t {
    fields {
        version : 4;
        ihl : 4;
        diffserv : 8;
        totalLen : 16;
        identification : 16;
        flags : 3;
        fragOffset : 13;
        ttl : 8;
        protocol : 8;
        hdrChecksum : 16;
        srcAddr : 32;
        dstAddr: 32;
    }
}

header_type arp_t{
 fields {
        hw_type : 16;
        protocol_type : 16;
        hw_size : 8;
        protocol_size : 8;
        opcode : 16;
        srcMac : 48;
        srcIp : 32;
        dstMac : 48;
        dstIp : 32;
    }
}

header_type tcp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        seqNo : 32;
        ackNo : 32;
        dataOffset : 4;
        res : 3;
        ecn : 3;
        ctrl : 6;
        window : 16;
        checksum : 16;
        urgentPtr : 16;
    }
}

header_type udp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        len     : 16;
        checksum: 16;
    }
}

parser start {
    return parse_ethernet;
}

#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_ARP 0x0806
#define IPPROTCOL_UDP 17
#define IPPROTCOL_TCP 6

header ethernet_t ethernet;

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        ETHERTYPE_IPV4 : parse_ipv4;
        ETHERTYPE_ARP : parse_arp;
        default: ingress;
    }
}

header ipv4_t ipv4;
header arp_t arp;

parser parse_ipv4 {
    extract(ipv4);
    return select(ipv4.protocol) {
        IPPROTCOL_TCP : tcp_parser;
        IPPROTCOL_UDP : udp_parser;
        default : ingress;
    }
}
parser parse_arp {
    extract(arp);
    return ingress;
}

header tcp_t  tcp;
header udp_t  udp;

parser tcp_parser {
    extract(tcp);
    return ingress;
}

parser udp_parser {
    extract(udp);
    return ingress;
}

header_type routing_metadata_t {
    fields {
        nhop_ipv4 : 32;
    }
}

metadata routing_metadata_t routing_metadata;

action modify_dip(dip){
	modify_field(ipv4.dstAddr,dip);
	modify_field(routing_metadata.nhop_ipv4, dip);
	add_to_field(ipv4.ttl, -1);
}
action modify_ipv4_vip(vip){
	modify_field(ipv4.srcAddr,vip);
}
action modify_arp_vip(vip){
	modify_field(arp.srcIp,vip);
}

action set_arp_nhop(dip) {
    modify_field(arp.dstIp,dip);
    modify_field(routing_metadata.nhop_ipv4, dip);
}

action noop() {
    no_op();
}

action set_port(port) {
    modify_field(standard_metadata.egress_spec, port);
}

action _drop() {
    drop();
}

table vipt_with_tcp{
	reads{
	ipv4.dstAddr: exact;
	ipv4.protocol: exact;
	tcp.dstPort: exact;
	}
	actions{
		modify_dip;
		noop;
	}
}

table vipt_with_udp{
	reads{
	ipv4.dstAddr: exact;
	ipv4.protocol: exact;
	udp.dstPort: exact;
	}
	actions{
		modify_dip;
		noop;
	}
}
table arp_nhop{
    reads {
        arp.dstIp : exact;
    }
    actions {
        set_arp_nhop;
        _drop;
    }
}
table forward_table {
    reads {
        routing_metadata.nhop_ipv4 : exact;
    }
    actions {
        set_port;
        _drop;
    }
}
table set_ipv4_srcip{
	reads{
	ipv4.srcAddr: exact;
	}
	actions{
		modify_ipv4_vip;
		noop;
	}
}
table set_arp_srcip{
        reads{
        arp.srcIp: exact;
        }
        actions{
                modify_arp_vip;
                noop;
        }
}

control ingress{
	if(valid(ipv4))
	{
		if(valid(tcp))
		{
			apply(vipt_with_tcp);
		}
		else if(valid(udp))
		{
			apply(vipt_with_udp);
		}
                apply(set_ipv4_srcip);
	}
	else if(valid(arp))
	{
		apply(arp_nhop);
		apply(set_arp_srcip);
	}
	apply(forward_table);
}
