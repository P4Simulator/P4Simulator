# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

def configure(conf):
    have_bm = conf.check_cfg(package='bm', uselib_store='BM',
                                  args=['--cflags', '--libs'],
                                  mandatory=True)
    have_boost = conf.check_cfg(package='boost_system',uselib_store='BOOST' ,
                                  args=['--cflags', '--libs'],
                                  mandatory=True)
#    conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')
#    conf.env.append_value("LINKFLAGS", ["-L/usr/local/lib","bmall"])
def build(bld):
    all_modules = [mod[len("ns3-"):] for mod in bld.env['NS3_ENABLED_MODULES']]
    module = bld.create_ns3_module('p4', ['antenna', 'aodv', 'applications', 'bridge', 'buildings', 'config-store', 'core', 'csma', 'csma-layout', 'dsdv', 'dsr', 'energy', 'fd-net-device', 'flow-monitor', 'internet', 'internet-apps', 'lr-wpan', 'lte', 'mesh', 'mobility', 'mpi', 'netanim', 'network', 'nix-vector-routing', 'olsr', 'point-to-point', 'point-to-point-layout', 'propagation', 'sixlowpan', 'spectrum', 'stats', 'test', 'topology-read', 'traffic-control', 'uan', 'virtual-net-device', 'wave', 'wifi', 'wimax'])
    module.source = [
        'model/p4-net-device.cc',
        'helper/p4-helper.cc',
        ]

    module_test = bld.create_ns3_module_test_library('p4')
    module_test.source = [
        'test/p4-test-suite.cc',
        ]

    headers = bld(features='ns3header')
    headers.module = 'p4'
    headers.source = [
        'model/p4-net-device.h',
        'helper/p4-helper.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')
    # bld.ns3_python_bindings()
    module.use.append("BM")
    module.use.append("BOOST")
