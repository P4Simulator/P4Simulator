# include "build-flowtable-helper.h"
# include <fstream>
# include <sstream>
# include <unordered_set>
#include "ns3/log.h"


namespace ns3 {

	NS_LOG_COMPONENT_DEFINE ("BuildFlowtableHelper");

	BuildFlowtableHelper::BuildFlowtableHelper()
	{
		NS_LOG_FUNCTION (this);
	}


	BuildFlowtableHelper::~BuildFlowtableHelper()
	{
		NS_LOG_FUNCTION (this);
	}


	void BuildFlowtableHelper::setSwitchesFlowtableEntries()
	{
		for (size_t i = 0; i < m_terminalNodes.size(); i++)
			dfsFromTerminalIndex(i);
	}


	void BuildFlowtableHelper::dfsFromTerminalIndex(unsigned int terminal_index)
	{
		std::stack<SaveNode> pass_switch;
		dfs(terminal_index, m_terminalNodes[terminal_index].linkSwitchIndex, m_terminalNodes[terminal_index].portIndex, pass_switch);
	}


	void BuildFlowtableHelper::dfs(unsigned int terminal_index, unsigned int switch_index, unsigned int switch_in_port, std::stack<SaveNode>& pass_switch)
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
				// make sure stay in init switch num
				while (pass_switch.size() > cur_pass_switch_num)
					pass_switch.pop();
				pass_switch.push(SaveNode(switch_index, t));
				dfs(terminal_index, m_switchNodes[switch_index].portNode[t].nodeIndex, m_switchNodes[switch_index].portNode[t].nodePort, pass_switch);
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
					line_buffer << "table_add forward_table set_port " << m_switchNodes[i].flowTableEntries[j].dstIp << " => " << m_switchNodes[i].flowTableEntries[j].outPort << std::endl;
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