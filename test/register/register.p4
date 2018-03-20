/*
Copyright 2013-present Barefoot Networks, Inc. 
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#define ETHERTYPE_ARP 0x0806

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
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

header_type intrinsic_metadata_t {
    fields {
        mcast_grp : 4;
        egress_rid : 4;
        mcast_hash : 16;
        lf_field_list: 32;
    }
}

header_type meta_t {
    fields {
        register_tmp : 32;
    }
}

metadata meta_t meta;

parser start {
    return parse_ethernet;
}

header ethernet_t ethernet;
header arp_t arp;
metadata intrinsic_metadata_t intrinsic_metadata;


parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        ETHERTYPE_ARP : parse_arp;
        default: ingress;
    }
}

parser parse_arp {
    extract(arp);
    return ingress;
}

action _drop() {
    drop();
}

action _nop() {
}

register my_register {
    width: 32;
    static: m_table;
    instance_count: 16384;
}

action m_action(register_idx) {
    // modify_field_rng_uniform(meta.register_tmp, 100, 200);

    register_write(my_register, register_idx, 10);
    register_read(meta.register_tmp, my_register, register_idx);
    modify_field(arp.srcIp,meta.register_tmp);//view my_register[register_idx] value
    modify_field(standard_metadata.egress_spec, 0);//modify egress_spec to dst port
    // TODO
}

table m_table {
    reads {
        ethernet.srcAddr : exact;
    }
    actions {
        m_action; _nop;
    }
    size : 16384;
}

control ingress {
    apply(m_table);
}

control egress {
}
