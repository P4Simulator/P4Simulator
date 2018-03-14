# include "ns3/log.h"
# include "ns3/global.h"

namespace ns3 {

  NS_LOG_COMPONENT_DEFINE("P4GlobalVar");
  NS_OBJECT_ENSURE_REGISTERED(P4GlobalVar);

  // init default static global variable
  unsigned int P4GlobalVar::g_networkFunc=ROUTER;
  std::string P4GlobalVar::g_flowTablePath="";
  std::string P4GlobalVar::g_p4MatchTypePath="";
  unsigned int P4GlobalVar::g_populateFlowTableWay=LOCAL_CALL;
  std::string P4GlobalVar::g_p4JsonPath="";

  std::string P4GlobalVar::g_homePath="/home/kphf1995cm/";
  std::string P4GlobalVar::g_ns3RootName="ns-allinone-3.26/";
  std::string P4GlobalVar::g_ns3SrcName="ns-3.26/";
  std::string P4GlobalVar::g_nfDir=P4GlobalVar::g_homePath+P4GlobalVar::g_ns3RootName+P4GlobalVar::g_ns3SrcName+"src/ns4/test/";
  std::string P4GlobalVar::g_topoDir=P4GlobalVar::g_homePath+P4GlobalVar::g_ns3RootName+P4GlobalVar::g_ns3SrcName+"src/ns4/topo/";
  std::string P4GlobalVar::g_flowTableDir=P4GlobalVar::g_homePath+P4GlobalVar::g_ns3RootName+P4GlobalVar::g_ns3SrcName+"src/ns4/flowtable/";
  unsigned int P4GlobalVar::g_nsType=NS4;
  std::map<std::string,unsigned int> P4GlobalVar::g_nfStrUintMap;

  unsigned long getTickCount(void)
  {
    unsigned long currentTime=0;
    #ifdef WIN32
      currentTime = GetTickCount();
    #endif
      struct timeval current;
      gettimeofday(&current, NULL);
      currentTime = current.tv_sec * 1000 + current.tv_usec / 1000;
    #ifdef OS_VXWORKS
      ULONGA timeSecond = tickGet() / sysClkRateGet();
      ULONGA timeMilsec = tickGet() % sysClkRateGet() * 1000 / sysClkRateGet();
      currentTime = timeSecond * 1000 + timeMilsec;
    #endif
    return currentTime;
  } 
  void P4GlobalVar::SetP4MatchTypeJsonPath()
  {
    if(P4GlobalVar::g_networkFunc==FIREWALL)
       {
          P4GlobalVar::g_p4JsonPath=P4GlobalVar::g_nfDir+"firewall/firewall.json";
          P4GlobalVar::g_p4MatchTypePath=P4GlobalVar::g_nfDir + "firewall/mtype.txt";
       }
    if(P4GlobalVar::g_networkFunc==SILKROAD)
       {
          P4GlobalVar::g_p4JsonPath=P4GlobalVar::g_nfDir+"silkroad/silkroad.json";
          P4GlobalVar::g_p4MatchTypePath=P4GlobalVar::g_nfDir + "silkroad/mtype.txt";
       }
    if(P4GlobalVar::g_networkFunc==ROUTER)
       {
          P4GlobalVar::g_p4JsonPath=P4GlobalVar::g_nfDir+"router/router.json";
          P4GlobalVar::g_p4MatchTypePath=P4GlobalVar::g_nfDir + "router/mtype.txt";
       }
    if(P4GlobalVar::g_networkFunc==SIMPLE_ROUTER)
       {
          P4GlobalVar::g_p4JsonPath=P4GlobalVar::g_nfDir+"simple_router/simple_router.json";
          P4GlobalVar::g_p4MatchTypePath=P4GlobalVar::g_nfDir + "simple_router/mtype.txt";
       }
  }

  void P4GlobalVar::InitNfStrUintMap()
  {
    P4GlobalVar::g_nfStrUintMap["ROUTER"]=ROUTER;
    P4GlobalVar::g_nfStrUintMap["SIMPLE_ROUTER"]=SIMPLE_ROUTER;
    P4GlobalVar::g_nfStrUintMap["FIREWALL"]=FIREWALL;
    P4GlobalVar::g_nfStrUintMap["SILKROAD"]=SILKROAD;
  }

  TypeId P4GlobalVar::GetTypeId(void)
  {
	  static TypeId tid = TypeId("ns3::P4GlobalVar")
		  .SetParent<Object>()
		  .SetGroupName("P4GlobalVar")
		  ;
	  return tid;
  }
  P4GlobalVar::P4GlobalVar()
  {
	  NS_LOG_FUNCTION(this);
  }

  P4GlobalVar::~P4GlobalVar()
  {
	  NS_LOG_FUNCTION(this);
  }
}
