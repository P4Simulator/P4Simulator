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
* Author: Valerio Sartini (valesar@gmail.com)
*/

#include "ns3/log.h"

#include "p4-topology-reader.h"


namespace ns3 {

	NS_LOG_COMPONENT_DEFINE("P4TopologyReader");

	NS_OBJECT_ENSURE_REGISTERED(P4TopologyReader);

	TypeId P4TopologyReader::GetTypeId(void)
	{
		static TypeId tid = TypeId("ns3::P4TopologyReader")
			.SetParent<Object>()
			.SetGroupName("P4TopologyReader")
			;
		return tid;
	}

	P4TopologyReader::P4TopologyReader()
	{
		NS_LOG_FUNCTION(this);
	}

	P4TopologyReader::~P4TopologyReader()
	{
		NS_LOG_FUNCTION(this);
	}

	void
		P4TopologyReader::SetFileName(const std::string &fileName)
	{
		m_fileName = fileName;
	}

	std::string
		P4TopologyReader::GetFileName() const
	{
		return m_fileName;
	}

	/* Manipulating the address block */

	P4TopologyReader::ConstLinksIterator_t
		P4TopologyReader::LinksBegin(void) const
	{
		return m_linksList.begin();
	}

	P4TopologyReader::ConstLinksIterator_t
		P4TopologyReader::LinksEnd(void) const
	{
		return m_linksList.end();
	}

	int
		P4TopologyReader::LinksSize(void) const
	{
		return m_linksList.size();
	}

	bool
		P4TopologyReader::LinksEmpty(void) const
	{
		return m_linksList.empty();
	}

	void
		P4TopologyReader::AddLink(Link link)
	{
		m_linksList.push_back(link);
		return;
	}

	P4TopologyReader::Link::Link(Ptr<Node> fromPtr, unsigned int fromIndex, char fromType, Ptr<Node> toPtr, unsigned int toIndex, char toType)
	{
		m_fromPtr = fromPtr;
		m_fromType = fromType;
		m_fromIndex = fromIndex;
		m_toPtr = toPtr;
		m_toType = toType;
		m_toIndex = toIndex;
	}

	P4TopologyReader::Link::Link()
	{
	}


	Ptr<Node> P4TopologyReader::Link::GetFromNode(void) const
	{
		return m_fromPtr;
	}

	Ptr<Node>
		P4TopologyReader::Link::GetToNode(void) const
	{
		return m_toPtr;
	}

	char
		P4TopologyReader::Link::GetFromType(void) const
	{
		return m_fromType;
	}
	char
		P4TopologyReader::Link::GetToType(void) const
	{
		return m_toType;
	}
	unsigned int
		P4TopologyReader::Link::GetFromIndex(void) const
	{
		return m_fromIndex;
	}
	unsigned
		int P4TopologyReader::Link::GetToIndex(void) const
	{
		return m_toIndex;
	}

	std::string
		P4TopologyReader::Link::GetAttribute(const std::string &name) const
	{
		NS_ASSERT_MSG(m_linkAttr.find(name) != m_linkAttr.end(), "Requested topology link attribute not found");
		return m_linkAttr.find(name)->second;
	}

	bool
		P4TopologyReader::Link::GetAttributeFailSafe(const std::string &name, std::string &value) const
	{
		if (m_linkAttr.find(name) == m_linkAttr.end())
		{
			return false;
		}
		value = m_linkAttr.find(name)->second;
		return true;
	}

	void
		P4TopologyReader::Link::SetAttribute(const std::string &name, const std::string &value)
	{
		m_linkAttr[name] = value;
	}

	P4TopologyReader::Link::ConstAttributesIterator_t
		P4TopologyReader::Link::AttributesBegin(void) const
	{
		return m_linkAttr.begin();
	}

	P4TopologyReader::Link::ConstAttributesIterator_t
		P4TopologyReader::Link::AttributesEnd(void) const
	{
		return m_linkAttr.end();
	}
} /* namespace ns3 */
