#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_ARP 0x0806

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

header arp_t arp;

parser start {
    extract(arp);
    return ingress;
}

action _drop() {
    drop();
}

action try_modify_egress(port) {
    modify_field(standard_metadata.egress_spec, port);
}

table test {
    reads {
        arp.dstIp : exact;
    }
    actions {
        try_modify_egress;
        _drop;
    }
    size:512;
}

control ingress {
    apply(test);
}

control egress {}
