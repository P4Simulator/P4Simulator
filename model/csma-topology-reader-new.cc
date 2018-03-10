/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
* Copyright (c)
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
* Author:
*/

#include <fstream>
#include <cstdlib>
#include <sstream>
#include <set>
#include "ns3/log.h"

#include "csma-topology-reader-new.h"


namespace ns3 {

	NS_LOG_COMPONENT_DEFINE("CsmaTopologyReader");

	NS_OBJECT_ENSURE_REGISTERED(CsmaTopologyReader);

	TypeId CsmaTopologyReader::GetTypeId(void)
	{
		static TypeId tid = TypeId("ns3::CsmaTopologyReader")
			.SetParent<P4TopologyReader>()
			.SetGroupName("P4TopologyReader")
			.AddConstructor<CsmaTopologyReader>()
			;
		return tid;
	}

	CsmaTopologyReader::CsmaTopologyReader()
	{
		NS_LOG_FUNCTION(this);
	}

	CsmaTopologyReader::~CsmaTopologyReader()
	{
		NS_LOG_FUNCTION(this);
	}

	void
		CsmaTopologyReader::Read(void)
	{
		std::ifstream topgen;
		topgen.open(GetFileName().c_str());

		(Ptr<Node>)* nodes;
		//NodeContainer nodes;

		if (!topgen.is_open())
		{
			NS_LOG_WARN("Csma topology file object is not open, check file name and permissions");
			//return nodes;
			abort();
		}

		unsigned int fromIndex;
		char fromType;
		unsigned int toIndex;
		char toType;

		std::string linkAttrValue;

		int switchNum = 0;
		int hostNum = 0;
		int linkNum = 0;
		int nodeNum = 0;

		int curLinkNumber = 0;
		int curNodeNumber = 0;
	
		std::istringstream lineBuffer;
		std::string line;

		getline(topgen, line);
		lineBuffer.str(line);

		lineBuffer >> switchNum >> hostNum >> linkNum;
		
		NS_LOG_INFO("Csma topology should have " << switchNum << " switches and " << hostNum << " hosts and "<< linkNum <<" links");

		nodeNum = switchNum + hostNum;
		nodes = new (Ptr<Node>)[nodeNum];

		for (int i = 0; i < nodes[i]; i++)
			nodes[i] = 0;

		for (int i = 0; i < linkNum && !topgen.eof(); i++)
		{
			getline(topgen, line);
			lineBuffer.clear();
			lineBuffer.str(line);

			lineBuffer >> fromIndex >> fromType >> toIndex >> toType >> linkAttrValue;

			NS_LOG_INFO("Link " << curLinkNumber << " from: " << fromIndex << " " << fromType << " to: " << toIndex << " " << toType);

			if (nodes[fromIndex] == 0)
			{
				NS_LOG_INFO("Node " << curNodeNumber << " index: " << fromIndex);
				Ptr<Node> tmpNode = CreateObject<Node>();
				nodes[fromIndex] = tmpNode;
				curNodeNumber++;
			}

			if (nodes[toIndex] == 0)
			{
				NS_LOG_INFO("Node " << curNodeNumber << " index: " << toIndex);
				Ptr<Node> tmpNode = CreateObject<Node>();
				nodes[toIndex] = tmpNode;
				curNodeNumber++;
			}

			Link link(nodes[fromIndex], fromIndex, fromType, nodes[toIndex], toIndex, toType);

			if (!linkAttrValue.empty())
			{
				NS_LOG_INFO("Link " << curLinkNumber << " weight: " << linkAttrValue);
				link.SetAttribute("weight", linkAttrValue);
			}

			AddLink(link);

			curLinkNumber++;
		}

		for (int i = 0; i < switchNum; i++)
			m_switches.Add(nodes[i]);
		for (int i = 0; i < hostNum; i++)
			m_hosts.Add(nodes[switchNum + i]);

		NS_LOG_INFO("Csma topology created with " << curNodeNumber << " nodes and " << curLinkNumber << " links");
		topgen.close();
	}

} /* namespace ns3 */
