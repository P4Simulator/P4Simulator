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

parser start {
    return parse_ethernet;
}

#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_ARP 0x0806

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
    return ingress;
}
parser parse_arp {
    extract(arp);
    return ingress;
}


action _drop() {
    drop();
}

header_type routing_metadata_t {
    fields {
        nhop_ipv4 : 32;
    }
}

metadata routing_metadata_t routing_metadata;

action set_ipv4_nhop(nhop_ipv4) {
    modify_field(routing_metadata.nhop_ipv4, nhop_ipv4);
    add_to_field(ipv4.ttl, -1);
}

action set_arp_nhop(nhop_ipv4) {

    modify_field(routing_metadata.nhop_ipv4, nhop_ipv4);
}

table ipv4_nhop {
    reads {
        ipv4.dstAddr : exact;
    }
    actions {
        set_ipv4_nhop;
        _drop;
    }
    size: 8192;
}
table arp_nhop{
    reads {
        arp.dstIp : exact;
    }
    actions {
        set_arp_nhop;
        _drop;
    }
   size: 8192;
}

action set_port(port) {
    modify_field(standard_metadata.egress_spec, port);
}

table forward_table {
    reads {
        routing_metadata.nhop_ipv4 : exact;
    }
    actions {
        set_port;
        _drop;
    }
    size: 8192;
}


control ingress {

    if((valid(ipv4) and ipv4.ttl>0) or valid(arp)){
        if(valid(ipv4) and ipv4.ttl > 0) {
            apply(ipv4_nhop);
        }
        else if(valid(arp)){
            apply(arp_nhop);
        }
        apply(forward_table);
    }
}
