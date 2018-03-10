
#include "ns3/global.h"

namespace ns3 {

std::string g_flowTablePath;
std::string g_p4MatchTypePath;
std::string g_nfDir;
unsigned int g_networkFunc;

// set switch network function, flowtable path and flowtable match type path
// firewall router silkroad
void SetSwitchConfigInfo(std::string ftPath,std::string mtPath)
{
  g_flowTablePath=ftPath;
  g_p4MatchTypePath=mtPath;
}

void InitSwitchConfig()
{
  switch(g_networkFunc)
    {
    case FIREWALL:
      {
        SetSwitchConfigInfo(g_nfDir+"firewall/command.txt",g_nfDir+"firewall/mtype.txt");
        break;
      }
    case ROUTER:
      {
        SetSwitchConfigInfo(g_nfDir+"router/command.txt",g_nfDir+"router/mtype.txt");
        break;
      }
    case SILKROAD:
      {
        SetSwitchConfigInfo(g_nfDir+"silkroad/command.txt",g_nfDir+"silkroad/mtype.txt");
        break;
      }
    default:
      {
        break;
      }
    }
}

}
