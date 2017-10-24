/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef TREE_TOPO_HELPER_H
#define TREE_TOPO_HELPER_H

#include <vector>
#include <string>

namespace ns3 {
	/**
	*
	* \brief build tree topo
	*/
	class TreeTopoHelper {
	public:
		/*
		* Construct a TreeTopoHelper
		*/
		TreeTopoHelper(unsigned int treeLevelNum, std::string topoFileName="topo.txt");
		~TreeTopoHelper();

		unsigned int GetSwitchNum();
		unsigned int GetTreeLevelNum();
		unsigned int GetTerminalNum();
		void SetLinkAttr(std::string attr="");
		std::string GetLinkAttr();
		unsigned int GetLinkNum();
		void SetTopoFileName(const std::string & topoFileName);
		std::vector<unsigned int>* GetSwitchLinkTmlIndex();
		void Write();

	private:
		void Build(unsigned int treeLevelNum);
		TreeTopoHelper(const TreeTopoHelper&);
		TreeTopoHelper & operator =(const TreeTopoHelper&);

		unsigned int m_switchNum;
		unsigned int m_treeLevelNum;
		unsigned int m_terminalNum;
		std::vector<unsigned int> *m_switchLinkTmlIndex;
		std::string m_topoFileName;
		std::string m_linkAttr;
		unsigned int m_linkNum;

	};
}

#endif /* TREE_TOPO_HELPER_H */



