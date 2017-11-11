1 How to use.
  run run.sh script,maybe some path information need to change according to your host directory.You can try to modify p4-example.cc to select different network function writed by p4, and it has detailed illustration in p4-example.cc.

2 How to develop your own NS4NetDevice.
  First, write a p4 program according to your application scenarios, we provide some p4 program examples in the directory of test, such as firewall, router silkroad.   
  Second, compile p4 program to json style using p4c-bmv2. The instrcution is: p4c-bmv2 --json demo.json demo.p4. 
  Third, add json path compiled by p4 program to p4-net-device.cc, which is in the directory of model. The moethod of adding json path is provided by P4NetDevice constructor function.
  Fourth, configure flowtable entries according the definition of p4 program. You can refer to command.txt in the directory of router.
  Fifth, build a suitable network topology to verify p4 program, can refer to p4-example.cc.Do not forget to add program path in wscript.
  Sixth, run it.   

3 Something need to pay attention.
  There are some path information need to modify in p4-example.cc and p4-net-device.cc(P4NetDevice::P4NetDevice()). 
