/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "binary-tree-topo-helper.h"
#include "ns3/log.h"
#include <math.h>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE("BinaryTreeTopoHelper");
    //NS_OBJECT_ENSURE_REGISTERED(TreeTopoHelper);


    BinaryTreeTopoHelper::BinaryTreeTopoHelper(unsigned int treeLevelNum, std::string topoFileName) {
        //NS_LOG_FUNCTION();
        NS_LOG_FUNCTION_NOARGS ();
        Build(treeLevelNum);
        m_topoFileName = topoFileName;
    }
    BinaryTreeTopoHelper::~BinaryTreeTopoHelper()
    {
        delete[] m_switchLinkTmlIndex;
    }
    void BinaryTreeTopoHelper::SetTopoFileName(std::string & topoFileName)
    {
        m_topoFileName = topoFileName;
    }

    /*unsigned int m_switchNum;
    unsigned int m_treeLevelNum;
    unsigned int m_terminalNum;
    std::vector<unsigned int> m_switchLinkTmlIndex;*/

    unsigned int BinaryTreeTopoHelper::GetSwitchNum()
    {
        return m_switchNum;
    }
    unsigned int BinaryTreeTopoHelper::GetTreeLevelNum()
    {
        return m_treeLevelNum;
    }
    unsigned int BinaryTreeTopoHelper::GetTerminalNum()
    {
        return m_terminalNum;
    }

    void BinaryTreeTopoHelper::SetLinkDataRate(std::string dataRate)
    {
        m_linkDataRate=dataRate;
    }

    std::string BinaryTreeTopoHelper::GetLinkDataRate()
    {
        return m_linkDataRate;
    }

    void BinaryTreeTopoHelper::SetLinkDelay(std::string delay)
    {
        m_linkDelay=delay;
    }

    std::string BinaryTreeTopoHelper::GetLinkDelay()
    {
        return m_linkDelay;
    }

    unsigned int BinaryTreeTopoHelper::GetLinkNum()
    {
        return m_linkNum;
    }

    std::vector<unsigned int>* BinaryTreeTopoHelper::GetSwitchLinkTmlIndex()
    {
        return m_switchLinkTmlIndex;
    }

    void BinaryTreeTopoHelper::Build(unsigned int treeLevelNum)
    {
        m_treeLevelNum = treeLevelNum;
        // maybe overflow
        m_switchNum = (unsigned int)pow(2, treeLevelNum) - 1;
        m_switchLinkTmlIndex = new std::vector<unsigned int>[m_switchNum];

        unsigned int curTmlIndex = m_switchNum;
        unsigned int curSwitchIndex = 0;

        // handle first level
        m_switchLinkTmlIndex[curSwitchIndex].push_back(curTmlIndex++);
        m_switchLinkTmlIndex[curSwitchIndex].push_back(curTmlIndex++);

        curSwitchIndex++;
        // handle middle level
        for (unsigned int level = 2; level<treeLevelNum; level++)
        {
            unsigned int curLevelSwitchNum = (unsigned int)pow(2, level - 1);

            for (unsigned int k = 0; k<curLevelSwitchNum; k++)
            {
                m_switchLinkTmlIndex[curSwitchIndex++].push_back(curTmlIndex++);
            }

        }
        // handle last level

        unsigned int lastLevelSwitchNum = (unsigned int)pow(2, treeLevelNum - 1);

        for (unsigned int k = 0; k<lastLevelSwitchNum; k++)
        {
            for (int i = 0; i<3; i++)
                m_switchLinkTmlIndex[curSwitchIndex].push_back(curTmlIndex++);
            curSwitchIndex++;
        }

        m_terminalNum = curTmlIndex - curSwitchIndex;
        m_linkNum = m_terminalNum + m_switchNum - 1;
    }

    void BinaryTreeTopoHelper::Write()
    {
        std::ofstream file;
        file.open(m_topoFileName);

        char fromType='h';
        char toType='s';
        std::string linkDataRate;
        std::string linkDelay;

        std::ostringstream lineBuffer;
        std::string line;
        lineBuffer << m_switchNum<<" "<<m_terminalNum << " " << m_linkNum << std::endl;
        file << lineBuffer.str();

        for (unsigned int i = 0; i < m_switchNum; i++)
        {
            for (std::vector<unsigned int>::iterator iter = m_switchLinkTmlIndex[i].begin(); iter != m_switchLinkTmlIndex[i].end(); iter++)
            {
                //from,fromType,*iter,*iter,toType,*iter,linkAttr
                lineBuffer.clear();
                lineBuffer.str("");
                lineBuffer << *iter << " " << fromType << " " << i <<" "<< toType << " " << linkDataRate <<" "<< linkDelay << std::endl;
                file << lineBuffer.str();
            }
            
            if (i != 0)
            {
                lineBuffer.clear();
                lineBuffer.str("");
                lineBuffer << (i-1)/2 << " " << 's' << " " << i << " " << toType << " " << linkDataRate << " " << linkDelay << std::endl;
                file << lineBuffer.str();
            }
        }
	for(unsigned int i=0;i<m_switchNum;i++)
	{
		lineBuffer.clear();
		lineBuffer.str("");
		lineBuffer << i << " " <<"SIMPLE_ROUTER" << std::endl;
		file << lineBuffer.str();
	}
        file.close();
    }
}

