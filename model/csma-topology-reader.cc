/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Universita' di Firenze, Italy
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
 * Author: Tommaso Pecorella (tommaso.pecorella@unifi.it)
 * Author: Valerio Sartini (Valesar@gmail.com)
 */

#include <fstream>
#include <cstdlib>
#include <sstream>
#include <set>
#include "ns3/log.h"

#include "csma-topology-reader.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CsmaTopologyReader");

NS_OBJECT_ENSURE_REGISTERED (CsmaTopologyReader);

TypeId CsmaTopologyReader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CsmaTopologyReader")
    .SetParent<P4TopologyReader> ()
    .SetGroupName ("P4TopologyReader")
    .AddConstructor<CsmaTopologyReader> ()
  ;
  return tid;
}

CsmaTopologyReader::CsmaTopologyReader ()
{
  NS_LOG_FUNCTION (this);
}

CsmaTopologyReader::~CsmaTopologyReader ()
{
  NS_LOG_FUNCTION (this);
}

struct NameNode{
	std::string name;
	Ptr<Node> node;
	NameNode(const std::string& n,const Ptr<Node>& no)
	{
	 name=n;
	 node=no;
	}
        unsigned int str_to_uint(std::string str)const
	{
		unsigned int res = 0;
		for (size_t i = 0; i < str.size(); i++)
			res = res * 10 + str[i] - '0';
		return res;
	}
	bool operator<(const NameNode& r)const
	{
		if (str_to_uint(name) < str_to_uint(r.name))
			return true;
		else
			return false;
	}
};
void
CsmaTopologyReader::Read (void)
{
  std::ifstream topgen;
  topgen.open (GetFileName ().c_str ());
  std::map<std::string, Ptr<Node> > nodeMap;
  //NodeContainer nodes;

  if ( !topgen.is_open () )
    {
      NS_LOG_WARN ("Csma topology file object is not open, check file name and permissions");
      //return nodes;
      abort();
    }

  std::string from;
  std::string fromType;
  unsigned int fromLinkedSwitchIndex;
  std::string to;
  std::string toType;
  unsigned int toLinkedSwitchIndex;

  std::string linkAttr;
  std::set<NameNode> terminalSet;
  std::set<NameNode> switchSet;

  int linksNumber = 0;
  int nodesNumber = 0;

  int totnode = 0;
  int totlink = 0;

  std::istringstream lineBuffer;
  std::string line;

  getline (topgen,line);
  lineBuffer.str (line);

  lineBuffer >> totnode;
  lineBuffer >> totlink;
  NS_LOG_INFO ("Csma topology should have " << totnode << " nodes and " << totlink << " links");

  /*for (int i = 0; i < totnode && !topgen.eof (); i++)
    {
      getline (topgen,line);
    }*/

  for (int i = 0; i < totlink && !topgen.eof (); i++)
    {
      getline (topgen,line);
      lineBuffer.clear ();
      lineBuffer.str (line);

      lineBuffer >> from >>fromType>>fromLinkedSwitchIndex;
      lineBuffer >> to>>toType>>toLinkedSwitchIndex;
      lineBuffer >> linkAttr;

      if ( (!from.empty ()) && (!to.empty ()) )
        {
          NS_LOG_INFO ( "Link " << linksNumber << " from: " << from<<" "<<fromType<<" "<<fromLinkedSwitchIndex << " to: " << to<<" "<<toType<<" "<<toLinkedSwitchIndex);

          if ( nodeMap[from] == 0 )
            {
              NS_LOG_INFO ( "Node " << nodesNumber << " name: " << from);
              Ptr<Node> tmpNode = CreateObject<Node> ();
              nodeMap[from] = tmpNode;
              //nodes.Add (tmpNode);
              if(fromType.compare("terminal")==0)
              {
                //terminals.Add(tmpNode);
		terminalSet.insert(NameNode(from,tmpNode));
              }
              else
              {
                if(fromType.compare("switch")==0)
                {
                 // switches.Add(tmpNode);
		   switchSet.insert(NameNode(from,tmpNode));

                }
                else
                {
                  NS_LOG_INFO("type error");
                  abort();
                }
              }
              nodesNumber++;
            }

          if (nodeMap[to] == 0)
            {
              NS_LOG_INFO ( "Node " << nodesNumber << " name: " << to);
              Ptr<Node> tmpNode = CreateObject<Node> ();
              nodeMap[to] = tmpNode;
              //nodes.Add (tmpNode);
              if(toType.compare("terminal")==0)
              {
                //terminals.Add(tmpNode);
		terminalSet.insert(NameNode(to,tmpNode));

              }
              else
              {
                if(toType.compare("switch")==0)
                {
                 // switches.Add(tmpNode);
		 switchSet.insert(NameNode(to,tmpNode));

                }
                else
                {
                  NS_LOG_INFO("type error");
                  abort();
                }
              }
              nodesNumber++;
            }

          Link link ( nodeMap[from], from, fromType,fromLinkedSwitchIndex,nodeMap[to], to,toType,toLinkedSwitchIndex);
          if ( !linkAttr.empty () )
            {
              NS_LOG_INFO ( "Link " << linksNumber << " weight: " << linkAttr);
              link.SetAttribute ("Weight", linkAttr);
            }
          AddLink (link);

          linksNumber++;
        }
    }
  for (std::set<NameNode>::iterator iter = terminalSet.begin(); iter != terminalSet.end(); iter++)
      {
        std::cout<<"t"<<iter->name<<" ";
        terminals.Add(iter->node);
      }
  for (std::set<NameNode>::iterator iter = switchSet.begin(); iter != switchSet.end(); iter++)
      {
        std::cout<<"s"<<iter->name<<" ";
        switches.Add(iter->node);
      }

  NS_LOG_INFO ("Csma topology created with " << nodesNumber << " nodes and " << linksNumber << " links");
  topgen.close ();
}

} /* namespace ns3 */
