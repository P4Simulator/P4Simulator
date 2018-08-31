/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
* Copyright (c) YEAR COPYRIGHTHOLDER
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation;
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
* Author: PengKuang <kphf1995cm@outlook.com>
*/

#ifndef GLOBAL_H
#define GLOBAL_H
#include <cstring>
#include <map>
#include "ns3/object.h"
#include <sys/time.h>
#include "ns3/p4-controller.h"

namespace ns3 {

#define LOCAL_CALL 0
#define RUNTIME_CLI 1
#define NS3 1
#define P4Simulator 0

// nf info
unsigned const int ROUTER = 0;
unsigned const int FIREWALL = 1;
unsigned const int SILKROAD = 2;
unsigned const int SIMPLE_ROUTER = 3;
unsigned const int COUNTER = 4;
unsigned const int METER = 5;
unsigned const int REGISTER = 6;
// match type
unsigned const int EXACT = 0;
unsigned const int LPM = 1;
unsigned const int TERNARY = 2;
unsigned const int VALID = 3;
unsigned const int RANGE = 4;

//get current time (ms)
unsigned long getTickCount(void);

class P4Controller;
class P4GlobalVar : public Object
{
public:
	static TypeId GetTypeId(void);
	//controller
	static P4Controller g_p4Controller;

	// siwtch info
	static unsigned int g_networkFunc;
	static std::string g_p4MatchTypePath;
	static std::string g_flowTablePath;
	static std::string g_viewFlowTablePath;
	static std::string g_p4JsonPath;
	static unsigned int g_populateFlowTableWay;

	// path info
	static std::string g_homePath;
	static std::string g_ns3RootName;
	static std::string g_ns3SrcName;
	static unsigned int g_nsType;
	static std::string g_nfDir;
	static std::string g_topoDir;
	static std::string g_flowTableDir;

	// runtime CLI wait time
	static unsigned int g_runtimeCliTime;//s

	static std::map<std::string,unsigned int> g_nfStrUintMap;	
	//set g_p4MatchTypePath,g_p4JsonPath according to g_networkFunc
	static void SetP4MatchTypeJsonPath();
	static void InitNfStrUintMap();

private:
	P4GlobalVar();
	~P4GlobalVar();
	P4GlobalVar(const P4GlobalVar&);
	P4GlobalVar& operator= (const P4GlobalVar&);
};
}
#endif /*GLOBAL_H*/



