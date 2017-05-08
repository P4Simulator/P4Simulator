After install BMv2 to your system, you need to do some addtional linking work enabling NS-3 to find BM library.

`$ sudo vim /usr/local/lib/pkgconfig/bm.pc`
paste in:
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

paste in:

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

Now go back to your ns root folder, and run 

`$ ./waf configure --enable-examples`
`$ ./waf --run src/p4/examples/p4-example`

Cheers!
