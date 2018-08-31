# include "ns3/log.h"
# include "ns3/global.h"

namespace ns3 {

  NS_LOG_COMPONENT_DEFINE("P4GlobalVar");
  NS_OBJECT_ENSURE_REGISTERED(P4GlobalVar);

  P4Controller P4GlobalVar::g_p4Controller;

  // init default static global variable
  unsigned int P4GlobalVar::g_networkFunc=ROUTER;
  std::string P4GlobalVar::g_flowTablePath="";
  std::string P4GlobalVar::g_viewFlowTablePath="";
  std::string P4GlobalVar::g_p4MatchTypePath="";
  unsigned int P4GlobalVar::g_populateFlowTableWay=LOCAL_CALL;
  std::string P4GlobalVar::g_p4JsonPath="";

  std::string P4GlobalVar::g_homePath="/home/netarchlab/";
  std::string P4GlobalVar::g_ns3RootName="ns-allinone-3.27/";
  std::string P4GlobalVar::g_ns3SrcName="ns-3.27/";
  std::string P4GlobalVar::g_nfDir=P4GlobalVar::g_homePath+P4GlobalVar::g_ns3RootName+P4GlobalVar::g_ns3SrcName+"src/p4simulator/test/";
  std::string P4GlobalVar::g_topoDir=P4GlobalVar::g_homePath+P4GlobalVar::g_ns3RootName+P4GlobalVar::g_ns3SrcName+"src/p4simulator/topo/";
  std::string P4GlobalVar::g_flowTableDir=P4GlobalVar::g_homePath+P4GlobalVar::g_ns3RootName+P4GlobalVar::g_ns3SrcName+"src/p4simulator/flowtable/";
  unsigned int P4GlobalVar::g_nsType=P4Simulator;
  unsigned int P4GlobalVar::g_runtimeCliTime=10;
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
	switch (P4GlobalVar::g_networkFunc)
	{
	case FIREWALL: {
		P4GlobalVar::g_p4JsonPath = P4GlobalVar::g_nfDir + "firewall/firewall.json";
		P4GlobalVar::g_p4MatchTypePath = P4GlobalVar::g_nfDir + "firewall/mtype.txt";
		break;
	}
	case SILKROAD: {
		P4GlobalVar::g_p4JsonPath = P4GlobalVar::g_nfDir + "silkroad/silkroad.json";
		P4GlobalVar::g_p4MatchTypePath = P4GlobalVar::g_nfDir + "silkroad/mtype.txt";
		break;
	}
	case ROUTER: {
		P4GlobalVar::g_p4JsonPath = P4GlobalVar::g_nfDir + "router/router.json";
		P4GlobalVar::g_p4MatchTypePath = P4GlobalVar::g_nfDir + "router/mtype.txt";
		break;
	}
	case SIMPLE_ROUTER: {
		P4GlobalVar::g_p4JsonPath = P4GlobalVar::g_nfDir + "simple_router/simple_router.json";
		P4GlobalVar::g_p4MatchTypePath = P4GlobalVar::g_nfDir + "simple_router/mtype.txt";
		break;
	}
	case COUNTER: {
		P4GlobalVar::g_p4JsonPath = P4GlobalVar::g_nfDir + "counter/counter.json";
		P4GlobalVar::g_p4MatchTypePath = P4GlobalVar::g_nfDir + "counter/mtype.txt";
		break;
	}
	case METER: {
		P4GlobalVar::g_p4JsonPath = P4GlobalVar::g_nfDir + "meter/meter.json";
		P4GlobalVar::g_p4MatchTypePath = P4GlobalVar::g_nfDir + "meter/mtype.txt";
		break;
	}
	case REGISTER: {
		P4GlobalVar::g_p4JsonPath = P4GlobalVar::g_nfDir + "register/register.json";
		P4GlobalVar::g_p4MatchTypePath = P4GlobalVar::g_nfDir + "register/mtype.txt";
		break;
	}
	default: {
		std::cerr << "NETWORK_FUNCTION_NO_EXIST!!!" << std::endl;
		break;
	}
	}
  } 

  void P4GlobalVar::InitNfStrUintMap()
  {
    P4GlobalVar::g_nfStrUintMap["ROUTER"]=ROUTER;
    P4GlobalVar::g_nfStrUintMap["SIMPLE_ROUTER"]=SIMPLE_ROUTER;
    P4GlobalVar::g_nfStrUintMap["FIREWALL"]=FIREWALL;
    P4GlobalVar::g_nfStrUintMap["SILKROAD"]=SILKROAD;
    P4GlobalVar::g_nfStrUintMap["COUNTER"]=COUNTER;
    P4GlobalVar::g_nfStrUintMap["METER"]=METER;
    P4GlobalVar::g_nfStrUintMap["REGISTER"]=REGISTER;
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
