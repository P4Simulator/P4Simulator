#ifndef BUILD_FLOWTABLE_HELPER_H
#define BUILD_FLOWTABLE_HELPER_H

#include <vector>
#include <string>
#include <iostream>
#include <stack>

namespace ns3 {

	struct TerminalNode
	{
		std::string ipAddr;
		unsigned int linkSwitchIndex;
		unsigned int portIndex;
	public:
		TerminalNode(const std::string &ip, unsigned int lsi, unsigned int pi)
		{
			ipAddr = ip;
			linkSwitchIndex = lsi;
			portIndex = pi;
		}
	};

	struct NodeFlagIndex
	{
		int flag;//0 represent terminal; 1 represent switch;
		unsigned int nodeIndex;
		unsigned int nodePort;
	public:
		NodeFlagIndex(int f,unsigned int ni,unsigned int np)
		{
			flag = f;
			nodeIndex = ni;
			nodePort = np;
		}
	};

	struct FlowTableEntry
	{
		std::string srcIp;
		std::string dstIp;
		std::string outPort;
	public:
		FlowTableEntry(std::string src, std::string dst, std::string port)
		{
			srcIp = src;
			dstIp = dst;
			outPort = port;
		}
	};


	struct SwitchNode 
	{
		std::vector<FlowTableEntry> flowTableEntries;
		std::vector<NodeFlagIndex> portNode;
	public:
		SwitchNode(const std::vector<std::string> &pn)//(s0_0,t0)
		{
			char type;
			int flag;
			unsigned int node_index;
			unsigned int node_port;

			for (size_t i = 0; i < pn.size(); i++)
			{
				type = pn[i][0];
				if (type == 's') //switch
				{
					flag = 1;
					unsigned int pos = pn[i].find("_");
					node_index = str_to_uint(pn[i].substr(1, pos - 1));
					node_port = str_to_uint(pn[i].substr(pos+1));
					portNode.push_back(NodeFlagIndex(flag, node_index, node_port));
				}
				else //terminal
				{
					flag = 0;
					node_index = str_to_uint(pn[i].substr(1));
					node_port = 0;
					portNode.push_back(NodeFlagIndex(flag,node_index,node_port));
				}
			}
		}
	private:
		unsigned int str_to_uint(std::string str)
		{
			unsigned int res = 0;
			for (size_t i = 0; i < str.size(); i++)
			{
				res = res * 10 + str[i] - '0';
			}
			return res;
		}


	};

	struct SaveNode
	{
		unsigned int switch_index;
		unsigned int out_port_index;
	public:
		SaveNode(unsigned int si,unsigned int opi)
		{
			switch_index = si;
			out_port_index = opi;
		}
	};

	class BuildFlowtableHelper
	{
	public:

		BuildFlowtableHelper();
		~BuildFlowtableHelper();
		
		virtual void write(std::string file_dir);

		void build(const std::vector<unsigned int> &terminalLinkSwitchIndex, const std::vector<unsigned int> &terminalLinkSwitchPort,
			const std::vector<std::string> &terminalIp, const std::vector<std::vector<std::string>> &switchPortInfo)
		{
			for (size_t i = 0; i < terminalLinkSwitchIndex.size(); i++)
			{
				m_terminalNodes.push_back(TerminalNode(terminalIp[i], terminalLinkSwitchIndex[i], terminalLinkSwitchPort[i]));
			}
			for (size_t i = 0; i < switchPortInfo.size(); i++)
			{
				m_switchNodes.push_back(SwitchNode(switchPortInfo[i]));
			}
			setSwitchesFlowtableEntries();
		}

		void show()
		{
			for (size_t i = 0; i < m_switchNodes.size(); i++)
			{
				std::cout << "s" << i << ":" << std::endl;
				for (size_t j = 0; j < m_switchNodes[i].flowTableEntries.size(); j++)
					std::cout << m_switchNodes[i].flowTableEntries[j].srcIp<<" "<<
					m_switchNodes[i].flowTableEntries[j].dstIp<<" "<<m_switchNodes[i].flowTableEntries[j].outPort<< std::endl;
			}
		}

	private:

		void setSwitchesFlowtableEntries();

		void dfsFromTerminalIndex(unsigned int terminal_index);

		std::vector<TerminalNode> m_terminalNodes;
		std::vector<SwitchNode> m_switchNodes;

		BuildFlowtableHelper(const BuildFlowtableHelper&);

		BuildFlowtableHelper operator=(const BuildFlowtableHelper&);

		std::string uintToStr(unsigned int num);

		void dfs(unsigned int terminal_index, unsigned int switch_index, unsigned int switch_in_port,std::stack<SaveNode>& pass_switch);

		void addFlowtableEntry(unsigned int switch_index, unsigned int src_ip, unsigned int dst_ip, unsigned int out_port)
		{
			std::string port(1, out_port + '0');
			m_switchNodes[switch_index].flowTableEntries.push_back(FlowTableEntry(m_terminalNodes[src_ip].ipAddr,m_terminalNodes[dst_ip].ipAddr,port));
		}
	};

}

#endif