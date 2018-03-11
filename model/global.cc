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

  std::string P4GlobalVar::g_homePath="/home/kphf1995cm/";
  std::string P4GlobalVar::g_ns3RootName="ns-allinone-3.26/";
  std::string P4GlobalVar::g_ns3SrcName="ns-3.26/";
  std::string P4GlobalVar::g_nfDir=P4GlobalVar::g_homePath+P4GlobalVar::g_ns3RootName+P4GlobalVar::g_ns3SrcName+"src/ns4/test/";

  unsigned int P4GlobalVar::g_nsType=NS4;

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
