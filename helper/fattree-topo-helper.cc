/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "fattree-topo-helper.h"
#include <math.h>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include "ns3/log.h"


namespace ns3 {

	NS_LOG_COMPONENT_DEFINE("FattreeTopoHelper");

	FattreeTopoHelper::FattreeTopoHelper(unsigned int podNum, std::string topoFileName) {
		NS_LOG_FUNCTION_NOARGS();
		Build(podNum);
		m_topoFileName = topoFileName;
	}
	FattreeTopoHelper::~FattreeTopoHelper()
	{}
	void FattreeTopoHelper::SetTopoFileName(const std::string & topoFileName)
	{
		m_topoFileName = topoFileName;
	}


	unsigned int FattreeTopoHelper::GetSwitchNum()
	{
		return m_switchNum;
	}
	unsigned int FattreeTopoHelper::GetPodNum()
	{
		return m_podNum;
	}
	unsigned int FattreeTopoHelper::GetTerminalNum()
	{
		return m_terminalNum;
	}
	void FattreeTopoHelper::SetLinkAttr(std::string attr)
	{
		m_linkAttr = attr;
	}
	std::string FattreeTopoHelper::GetLinkAttr()
	{
		return m_linkAttr;
	}
	unsigned int FattreeTopoHelper::GetLinkNum()
	{
		return m_linkNum;
	}
	std::string  FattreeTopoHelper::UintToStr(unsigned int temp)
	{
		if (temp == 0)
			return std::string("0");
		std::string res;
		while (temp)
		{
			res.insert(res.begin(), temp % 10 + '0');
			temp /= 10;
		}
		return res;
	}

	void FattreeTopoHelper::Build(unsigned int podNum)
	{
		m_podNum = podNum;
		m_coreSwitchNum = (podNum / 2)*(podNum / 2);
		// maybe overflow
		m_switchNum = 5*m_coreSwitchNum;
		m_terminalNum = m_coreSwitchNum*podNum;
		m_switchLinkNodeTypeIndex.resize(m_switchNum);
		m_linkNum = 0;
		unsigned int tmlIndex = m_switchNum;
		unsigned int aggrSwitchIndex = m_coreSwitchNum;
		unsigned int edgeSwitchIndex = 3 * m_coreSwitchNum;
		// from core switch start number
		unsigned int halfPodNum = podNum / 2;
		unsigned int curAggrSwitchIndex;
		unsigned int curEdgeSwitchIndex;
		for (unsigned int i = 0; i < podNum; i++)// number pod
		{
			for (unsigned int j = 0; j < halfPodNum; j++) // number switch in pod
			{
				curAggrSwitchIndex = aggrSwitchIndex + i*halfPodNum + j;
				curEdgeSwitchIndex = edgeSwitchIndex + i*halfPodNum + j;
				for (unsigned int p = 0; p < halfPodNum; p++)
				{
					m_switchLinkNodeTypeIndex[curAggrSwitchIndex].push_back("s"+UintToStr(j*halfPodNum + p));//link core switch
					m_switchLinkNodeTypeIndex[curAggrSwitchIndex].push_back("s" + UintToStr(edgeSwitchIndex + i*halfPodNum + p));//link edge switch
					m_switchLinkNodeTypeIndex[curEdgeSwitchIndex].push_back("t"+UintToStr(tmlIndex + i*m_coreSwitchNum+j*halfPodNum + p));//link terminal
					m_linkNum += 3;
				}
			}
		}
	}

	void FattreeTopoHelper::Write()
	{
		std::ofstream file;
		file.open(m_topoFileName);

		std::string fromType = "terminal";
		std::string toType = "switch";
		std::string linkAttr;

		std::ostringstream lineBuffer;
		std::string line;
		lineBuffer << m_switchNum + m_terminalNum << " " << m_linkNum << std::endl;
		file << lineBuffer.str();

		for (unsigned int i = 0; i < m_switchNum; i++)
		{
			for (std::vector<std::string>::iterator iter = m_switchLinkNodeTypeIndex[i].begin(); iter != m_switchLinkNodeTypeIndex[i].end(); iter++)
			{
				//from,fromType,*iter,*iter,toType,*iter,linkAttr
				lineBuffer.clear();
				lineBuffer.str("");
				if ((*iter)[0] == 's')
				{
					fromType = "switch";
					lineBuffer << iter->substr(1) << " " << fromType << " " << iter->substr(1) << " " << i << " " << toType << " " << i << " " << linkAttr << std::endl;
				}
				else
				{
					fromType = "terminal";
					lineBuffer << iter->substr(1) << " " << fromType << " " << i << " " << i << " " << toType << " " << i << " " << linkAttr << std::endl;
				}
				file << lineBuffer.str();
			}
		}

		file.close();
	}

	void FattreeTopoHelper::Show()
	{
		for (unsigned int i = 0; i < m_switchNum; i++)
		{
			std::cout << i << ": ";
			for (std::vector<std::string>::iterator iter = m_switchLinkNodeTypeIndex[i].begin(); iter != m_switchLinkNodeTypeIndex[i].end(); ++iter)
			{
				std::cout << *iter << " ";
			}
			std::cout << std::endl;
		}
	}
}

