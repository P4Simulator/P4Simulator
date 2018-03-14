/*
target network topo
4 2 6 (switchNum hostNum linkNum)
0 s 2 s 1000Mbps 0.01ms (index type index type dataRate delay)
1 s 2 s 1000Mbps 0.01ms
0 s 3 s 1000Mbps 0.01ms
1 s 3 s 1000Mbps 0.01ms
2 s 4 h 1000Mbps 0.01ms
3 s 5 h
0 SIMPLE_ROUTER (switchIndex switchNf)
1 SIMPLE_ROUTER
2 SIMPLE_ROUTER
3 SIMPLE_ROUTER
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef BINARY_TREE_TOPO_HELPER_H
#define BINARY_TREE_TOPO_HELPER_H

#include <vector>
#include <string>

namespace ns3 {
	/**
	*
	* \brief build binary tree topo
	*/
	class BinaryTreeTopoHelper {
	public:

		/*
		* Construct a BinaryTreeTopoHelper
		*/
		BinaryTreeTopoHelper(unsigned int treeLevelNum, std::string topoFileName="topo.txt");
		
		~BinaryTreeTopoHelper();

		unsigned int GetSwitchNum();

		unsigned int GetTreeLevelNum();

		unsigned int GetTerminalNum();

		void SetLinkDataRate(std::string dataRate="");

		std::string GetLinkDataRate();

		void SetLinkDelay(std::string delay="");

		std::string GetLinkDelay();

		unsigned int GetLinkNum();

		void SetTopoFileName(std::string & topoFileName);

		std::vector<unsigned int>* GetSwitchLinkTmlIndex();

		void Write();

	private:
		void Build(unsigned int treeLevelNum);

		BinaryTreeTopoHelper(const BinaryTreeTopoHelper&);

		BinaryTreeTopoHelper & operator =(const BinaryTreeTopoHelper&);

		unsigned int m_switchNum;

		unsigned int m_treeLevelNum;

		unsigned int m_terminalNum;

		std::vector<unsigned int> *m_switchLinkTmlIndex;

		std::string m_topoFileName;

		std::string m_linkDataRate;

		std::string m_linkDelay;
		
		unsigned int m_linkNum;

	};
}

#endif /* BINARY_TREE_TOPO_HELPER_H */




