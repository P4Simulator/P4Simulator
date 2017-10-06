header_type th {
    fields {
        a : 2;
        b : 1;
    }
}

header th th_t;

parser start {
    extract(th_t);
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
        th_t.a : exact;
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