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
#include <string>

namespace ns3 {

#define LOCAL_CALL 0
#define RUNTIME_CLI 1
#define NS3 1
#define NS4 0

/*
// siwtch info
extern unsigned int g_networkFunc;
extern std::string g_p4MatchTypePath; 
extern std::string g_flowtablePath;

extern unsigned int g_populateFlowTableWay;

// path info
extern std::string g_homePath;
extern std::string g_ns3RootName;
extern std::string g_ns3SrcName;
extern unsigned int g_nsType;
extern std::string g_nfDir;
*/
// nf info
unsigned const int ROUTER=0;
unsigned const int FIREWALL=1;
unsigned const int SILKROAD=2;
unsigned const int SIMPLE_ROUTER=3;

// match type
unsigned const int EXACT=0;
unsigned const int LPM=1;
unsigned const int TERNARY=2;
unsigned const int VALID=3;
unsigned const int RANGE=4;


}
#endif /*GLOBAL_H*/


