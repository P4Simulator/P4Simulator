
# This is a installation and usage instructions for NS4.(Under development) 

# A virtual machine version is being prepared and will be released soon. Stay tuned!

We now implemented NS4 prototype based on [ns-3](https://www.nsnam.org ns-3 homepage) and [bmv2](https://github.com/p4lang/behavioral-model behavioral-model). To install NS4 enviornment, please follow  [NS4 Installation Instructions](#install). To run a demo of NS4, follow [NS4 Simulation Instructions](#simulate)

## <a name="install">NS4 Installation Instructions</a>

### System Requirement

This version of NS4 was only tested successfully on Ubuntu 14.04.

It should support other *nix operating systems as long as the OS supports Linux container tools and can successfully install bmv2 and ns-3.

### Install ns-3

ns-3 is the base we build NS4 on and where we run the example.

You can follow the official but lengthy [ns-3 installation guide](https://www.nsnam.org/wiki/Installation).

If you are using Ubuntu, you can run this script to install ns-3 with root priviledge.

```
apt install -y gcc g++ python
apt install -y gcc g++ python python-dev
apt install -y mercurial python-setuptools git
apt install -y qt4-dev-tools libqt4-dev
apt install -y cmake libc6-dev libc6-dev-i386 g++-multilib
apt install -y gdb valgrind 
apt install -y tcpdump
apt install -y sqlite sqlite3 libsqlite3-dev
apt install -y libboost-all-dev
```

### Install bmv2 library

bmv2 is software P4 target, aka a behaviral model, which takes care of all P4 pipeline logic 

You can follow the official installation guide in [BEHAVIORAL MODEL REPOSITORY](https://github.com/p4lang/behavioral-model).

If you are using Ubuntu, you can run this script to install ns-3 with root priviledge.

```
git clone https://github.com/p4lang/behavioral-model.git
cd behavioral-model
./install_deps.sh
./autogen.sh
./configure 'CXXFLAGS=-O0 -g'
make
make install
make check
```

If all test pass, then step forward.

### Install p4c-bm

p4c-bm is a P4 program compiler. It compiles P4 program and generates P4 binary files which we use to configure P4 targets.

```
git clone https://github.com/p4lang/p4c-bm.git


```



### Install Linux Container Tools 

### Install NS4



After install BMv2 to your system, you need to do some addtional linking work enabling NS-3 to find BM library.

`$ sudo vim /usr/local/lib/pkgconfig/bm.pc`

Paste in:
```
prefix=/usr/local
exec_prefix=${prefix}
libdir=${exec_prefix}/lib
includedir=/usr/local/include/bm

Name: BMv2
Description: Behaviral Model
Version: 0.0.0
Libs: -L${libdir} -lbmall
Cflags: -I${includedir}

```

`$ sudo vim /usr/local/lib/pkgconfig/boost_system.pc`

Paste:

```
prefix=/usr/lib/
exec_prefix=${prefix}
libdir=${exec_prefix}
includedir=/usr/local/include/bm

Name: boost_system
Description: Boost System
Version: 0.0.0
Libs: -L. -lboost_system
Cflags: -I${includedir}

```

`$ sudo vim /usr/local/lib/pkgconfig/simple_switch.pc`

Paste:

``` 
prefix=/usr/local
exec_prefix=${prefix}
libdir=${exec_prefix}/lib
includedir=/usr/local/include/bm

Name: simple switch
Description: Behaviral Model
Version: 0.0.0
Libs: -L${libdir} -lsimpleswitch_thrift
Cflags: -I${includedir}
```



Now go back to your ns root folder, and run 

`$ ./waf configure --enable-examples`

`$ ./waf --run src/p4/examples/p4-example`

Cheers!

## <a name="simulate">NS4 Simulation Instructions</a>