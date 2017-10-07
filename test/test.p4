#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_ARP 0x0806

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type arp_t {
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

header_type ipv4_t {
    fields {
        version:         4;
        ihl:             4;
        diffserv:        8;
        totalLen:        16;
        identification:  16;
        flags:           3;
        fragOffset:      13;
        ttl:             8;
        protocol:        8;
        hdrChecksum:     16;
        srcIp:           32;
        dstIp:           32;
    }
}

header_type my_metadata_t {
    fields {
        dstIp : 32;
    }
}

header ethernet_t ethernet;
header arp_t arp;
header ipv4_t ipv4;

metadata my_metadata_t my_metadata;

parser start {
    extract(ethernet);
    return select(latest.etherType) {
        ETHERTYPE_ARP : parse_arp;
        ETHERTYPE_IPV4 : parse_ipv4;
        default : ingress;
    }
}

parser parse_arp {
    extract(arp);
    return ingress;
}

parser parse_ipv4 {
    extract(ipv4);
    return ingress;
}

action _drop() {
    drop();
}

action try_modify_egress(port) {
    modify_field(standard_metadata.egress_spec, port);
}

action modify_ipv4_ip() {
    modify_field(my_metadata.dstIp, ipv4.dstIp);
}

action modify_arp_ip() {
    modify_field(my_metadata.dstIp, arp.dstIp);
}

table test0 {
    reads {
        ethernet.etherType : exact;
    }
    actions {
        modify_arp_ip;
        modify_ipv4_ip;
    }
}

table test {
    reads {
        my_metadata.dstIp : exact;
    }
    actions {
        try_modify_egress;
        _drop;
    }
    size:512;
}

control ingress {
    apply(test0);
    apply(test);
}

control egress {}
