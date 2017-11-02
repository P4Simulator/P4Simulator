# include "build-flowtable-helper.h"
# include <fstream>
# include <sstream>
# include <unordered_set>
#include "ns3/log.h"
# include <time.h>
# include <vector>
# include <unordered_map>

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE ("BuildFlowtableHelper");

	BuildFlowtableHelper::BuildFlowtableHelper(std::string buildType, unsigned int podNum)
	{
		NS_LOG_FUNCTION (this);
		m_buildType = buildType;
		m_podNum = podNum;
	}


	BuildFlowtableHelper::~BuildFlowtableHelper()
	{
		NS_LOG_FUNCTION (this);
	}


	void BuildFlowtableHelper::setSwitchesFlowtableEntries()
	{
		if (m_buildType == "default")
		{
			for (size_t i = 0; i < m_terminalNodes.size(); i++)
				dfsFromTerminalIndex(i);
		}
		else
		{
			if (m_buildType == "fattree")
			{
				unsigned int coreSwitchNum = (m_podNum / 2)*(m_podNum/2);
				unsigned int terminalNum = coreSwitchNum*m_podNum;
				unsigned int switchNum = 5 * coreSwitchNum;
				unsigned int halfPodNum = m_podNum / 2;
				unsigned int aggrStartIndex = coreSwitchNum;
				unsigned int edgeStartIndex = 3 * coreSwitchNum;
				//unsigned int tmlStartIndex = 5 * coreSwitchNum;
				unsigned int curAggrSwitchIndex;
				unsigned int curEdgeSwitchIndex;

				std::vector<std::vector<unsigned int>> switchReachTerminalIndexMap(switchNum);
				
				// add edge switch flowtable
				for (unsigned int i = 0; i < m_podNum; i++)// number pod
				{
					for (unsigned int j = 0; j < halfPodNum; j++)
					{
						curEdgeSwitchIndex = edgeStartIndex + i*halfPodNum + j;
						std::unordered_set<unsigned int> curEdgeSwitchLinkTerminalIndex;
						std::vector<unsigned int> otherPortIndex;
						//add flowtable entry for these packet which send to linked terminal
						for (size_t p = 0; p < m_switchNodes[curEdgeSwitchIndex].portNode.size(); p++)
						{
							if (m_switchNodes[curEdgeSwitchIndex].portNode[p].flag == 0)//represent terminal
							{
								//attain terminal ipAddr
								curEdgeSwitchLinkTerminalIndex.insert(m_switchNodes[curEdgeSwitchIndex].portNode[p].nodeIndex);
								switchReachTerminalIndexMap[curEdgeSwitchIndex].push_back(m_switchNodes[curEdgeSwitchIndex].portNode[p].nodeIndex);
								std::string dstIp = m_terminalNodes[m_switchNodes[curEdgeSwitchIndex].portNode[p].nodeIndex].ipAddr;
								m_switchNodes[curEdgeSwitchIndex].flowTableEntries.push_back(FlowTableEntry("", dstIp, uintToPortStr(p)));
							}
							else
								otherPortIndex.push_back(p);
						}
						//add flowtable entry for other packet
						for (unsigned int p = 0; p < terminalNum; p++)
						{
							if (curEdgeSwitchLinkTerminalIndex.count(p) == 0)
							{
								std::string dstIp = m_terminalNodes[p].ipAddr;
								//random select a out port
								unsigned int transfer_port = otherPortIndex[rand() % otherPortIndex.size()];
								m_switchNodes[curEdgeSwitchIndex].flowTableEntries.push_back(FlowTableEntry("", dstIp, uintToPortStr(transfer_port)));
							}
						}
					}
				}

				// add aggrate switch flowtable
				for (unsigned int i = 0; i < m_podNum; i++)// number pod
				{
					for (unsigned int j = 0; j < halfPodNum; j++)
					{
						curAggrSwitchIndex=aggrStartIndex+ i*halfPodNum + j;
						std::unordered_set<unsigned int> curAggrSwitchLinkTerminalIndex;
						std::vector<unsigned int> otherPortIndex;
						//add flowtable entry for these packet which send to linked terminal
						for (size_t p = 0; p < m_switchNodes[curAggrSwitchIndex].portNode.size(); p++)
						{
							if (m_switchNodes[curAggrSwitchIndex].portNode[p].nodeIndex > curAggrSwitchIndex)//mean link edge switch
							{
								unsigned int linkSIndex = m_switchNodes[curAggrSwitchIndex].portNode[p].nodeIndex;
								for (size_t q = 0; q < switchReachTerminalIndexMap[linkSIndex].size(); q++)
								{
									unsigned int linkTIndex = switchReachTerminalIndexMap[linkSIndex][q];
									std::string dstIp = m_terminalNodes[linkTIndex].ipAddr;
									curAggrSwitchLinkTerminalIndex.insert(linkTIndex);
									switchReachTerminalIndexMap[curAggrSwitchIndex].push_back(linkTIndex);
									m_switchNodes[curAggrSwitchIndex].flowTableEntries.push_back(FlowTableEntry("", dstIp, uintToPortStr(p)));

								}
							}
							else
								otherPortIndex.push_back(p);
						}

						//add flowtable entry for other packet
						for (unsigned int p = 0; p < terminalNum; p++)
						{
							if (curAggrSwitchLinkTerminalIndex.count(p) == 0)
							{
								std::string dstIp = m_terminalNodes[p].ipAddr;
								//random select a out port
								unsigned int transfer_port = otherPortIndex[rand() % otherPortIndex.size()];
								m_switchNodes[curAggrSwitchIndex].flowTableEntries.push_back(FlowTableEntry("", dstIp, uintToPortStr(transfer_port)));
							}
						}
					}
				}

				//add core switch flowtable
				for (unsigned int i = 0; i < coreSwitchNum; i++)
				{
					for (size_t p = 0; p < m_switchNodes[i].portNode.size(); p++)
					{
						unsigned int linkSIndex = m_switchNodes[i].portNode[p].nodeIndex;
						for (size_t q = 0; q < switchReachTerminalIndexMap[linkSIndex].size(); q++)
						{
							unsigned int linkTIndex = switchReachTerminalIndexMap[linkSIndex][q];
							std::string dstIp = m_terminalNodes[linkTIndex].ipAddr;
							m_switchNodes[i].flowTableEntries.push_back(FlowTableEntry("", dstIp, uintToPortStr(p)));
						}
					}
				}
			}
		}
	}


	void BuildFlowtableHelper::dfsFromTerminalIndex(unsigned int terminal_index)
	{
		std::stack<SaveNode> pass_switch;
                std::set<unsigned int> record_pass_switch;
		dfs(terminal_index, m_terminalNodes[terminal_index].linkSwitchIndex, m_terminalNodes[terminal_index].portIndex, pass_switch,record_pass_switch);
	
	}
        void BuildFlowtableHelper::dfs(unsigned int terminal_index, unsigned int switch_index, unsigned int switch_in_port, std::stack<SaveNode>& pass_switch, std::set<unsigned int> &record_pass_switch)
	{
		unsigned int cur_pass_switch_num = pass_switch.size();
		for (size_t t = 0; t < m_switchNodes[switch_index].portNode.size(); t++)//traver port number
		{
			if (t == switch_in_port) continue;
			if (m_switchNodes[switch_index].portNode[t].flag == 0)// represent terminal
			{
				std::stack<SaveNode> s = pass_switch;
				// add passed switch entry
				while (s.empty() == false)
				{
					addFlowtableEntry(s.top().switch_index, terminal_index, m_switchNodes[switch_index].portNode[t].nodeIndex, s.top().out_port_index);
					s.pop();
				}
				// add cur switch entry
				addFlowtableEntry(switch_index, terminal_index, m_switchNodes[switch_index].portNode[t].nodeIndex, t);
			}
			else //represent switch
			{
				unsigned next_switch_index = m_switchNodes[switch_index].portNode[t].nodeIndex;
				unsigned next_switch_port = m_switchNodes[switch_index].portNode[t].nodePort;
				if (record_pass_switch.count(next_switch_index)==0)
				{// make sure stay in init switch num
					while (pass_switch.size() > cur_pass_switch_num)
						pass_switch.pop();
					pass_switch.push(SaveNode(switch_index, t));
					record_pass_switch.insert(switch_index);
					dfs(terminal_index, next_switch_index, next_switch_port, pass_switch,record_pass_switch);
				}
			}
		}
	}


	std::string BuildFlowtableHelper::uintToStr(unsigned int num)
	{
		if (num != 0)
		{
			std::string res;
			while (num)
			{
				res.insert(res.begin(), num % 10 + '0');
				num /= 10;
			}
			return res;
		}
		else
			return std::string("0");
	}
        
        std::string BuildFlowtableHelper::uintToPortStr(unsigned int num)
	{
		if (num != 0)
		{
			std::string res("0x0000");
			unsigned int k = 5;
			int tmp;
			while (num)
			{
				tmp = num % 16;
				if (tmp < 10)
					res[k] = tmp + '0';
				else
					res[k] = tmp - 10 + 'a';
				k--;
				num /= 16;
			}
			return res;
		}
		else
		{
			return std::string("0x0000");
		}
	}



	void BuildFlowtableHelper::write(std::string file_dir)
	{
		std::ofstream fp;

		std::ostringstream line_buffer;

		std::unordered_set <std::string> handledDstIpSet;

		for (size_t i = 0; i < m_switchNodes.size(); i++)
		{
			std::string file_name = uintToStr(i);
			fp.open(file_dir + "/" + file_name);
			//set default action
			std::string default_action[] = { "table_set_default ipv4_nhop _drop","table_set_default arp_nhop _drop","table_set_default forward_table _drop" };
			for (int k = 0; k < 3; k++)
			{
				line_buffer.clear();
				line_buffer.str("");
				line_buffer << default_action[k]<<std::endl;
				fp << line_buffer.str();
			}

			//add flowtable entry
			//dst port decided by dstIp, so can combile flowtable entries
			//the best solution is to change FlowTableEntry struct
			handledDstIpSet.clear();

			for (size_t j = 0; j < m_switchNodes[i].flowTableEntries.size(); j++)
			{
				if (handledDstIpSet.count(m_switchNodes[i].flowTableEntries[j].dstIp) == 0)
				{
					handledDstIpSet.insert(m_switchNodes[i].flowTableEntries[j].dstIp);
					line_buffer.clear();
					line_buffer.str("");
					line_buffer << "table_add ipv4_nhop set_ipv4_nhop " << m_switchNodes[i].flowTableEntries[j].dstIp << " => " << m_switchNodes[i].flowTableEntries[j].dstIp << std::endl;
					fp << line_buffer.str();

					line_buffer.clear();
					line_buffer.str("");
					line_buffer << "table_add arp_nhop set_arp_nhop " << m_switchNodes[i].flowTableEntries[j].dstIp << " => " << m_switchNodes[i].flowTableEntries[j].dstIp << std::endl;
					fp << line_buffer.str();

					line_buffer.clear();
					line_buffer.str("");
					line_buffer << "table_add forward_table set_port " << m_switchNodes[i].flowTableEntries[j].dstIp << " => " <<m_switchNodes[i].flowTableEntries[j].outPort << std::endl;
					fp << line_buffer.str();
				}
			}
			fp.close();
		}
	}
}
/*
table_add ipv4_nhop set_ipv4_nhop dstIp => dstIp
table_add arp_nhop set_arp_nhop dstIp => dstIp
table_add forward_table set_port dstIp => port
*/
