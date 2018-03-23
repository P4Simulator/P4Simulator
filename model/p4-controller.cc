
#include "ns3/log.h"
#include "ns3/p4-controller.h"
#include <iostream>

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE("P4Controller");

	NS_OBJECT_ENSURE_REGISTERED(P4Controller);

	TypeId P4Controller::GetTypeId(void)
	{
		static TypeId tid = TypeId("ns3::P4Controller")
			.SetParent<Object>()
			.SetGroupName("P4Controller")
			;
		return tid;
	}

	P4Controller::P4Controller()
	{
		NS_LOG_FUNCTION(this);
	}

	P4Controller::~P4Controller()
	{
		for (size_t i = 0; i < m_p4Switches.size(); i++)
		{
			if (m_p4Switches[i] != NULL)
				delete m_p4Switches[i];
		}
		NS_LOG_FUNCTION(this);
	}

	void P4Controller::ViewAllSwitchInfo()
	{
		for (size_t i = 0; i < m_p4Switches.size(); i++)
		{
			ViewP4SwitchInfo(i);
		}
	}

	void P4Controller::ViewP4SwitchInfo(size_t index)//view p4 switch flow table, counter, register, meter info
	{
		if(m_p4Switches[index]!=NULL)
			m_p4Switches[index]->AttainSwitchFlowTableInfo();
		else
		{
			std::cerr << "Call ViewP4SwitchInfo(" << index << "): P4SwitchInterface Pointer is Null" << std::endl;
		}
	}

	P4SwitchInterface* P4Controller::GetP4Switch(size_t index)
	{
		return m_p4Switches[index];
	}

	P4SwitchInterface* P4Controller::AddP4Switch()
	{
		P4SwitchInterface* p4Switch = new P4SwitchInterface;
		m_p4Switches.push_back(p4Switch);
		return p4Switch;
	}

}
