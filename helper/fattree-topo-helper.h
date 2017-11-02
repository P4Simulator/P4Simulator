/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef FATTREE_TOPO_HELPER_H
#define FATTREE_TOPO_HELPER_H

#include <vector>
#include <string>

namespace ns3 {
	/**
	*
	* \brief build tree topo
	*/
	class FattreeTopoHelper {
	public:
		/*
		* Construct a TreeTopoHelper
		*/
		FattreeTopoHelper(unsigned int podNum, std::string topoFileName = "topo.txt");
		~FattreeTopoHelper();

		unsigned int GetSwitchNum();
		unsigned int GetPodNum();
		unsigned int GetTerminalNum();
		void SetLinkAttr(std::string attr = "");
		std::string GetLinkAttr();
		unsigned int GetLinkNum();
		void SetTopoFileName(const std::string & topoFileName);
		void Write();
		void Show();

	private:
		void Build(unsigned int podNum);
		FattreeTopoHelper(const FattreeTopoHelper&);
		FattreeTopoHelper & operator =(const FattreeTopoHelper&);

		unsigned int m_switchNum;
		//unsigned int m_treeLevelNum;
		unsigned int m_terminalNum;
		unsigned int m_podNum;
		unsigned int m_coreSwitchNum;
		std::vector<std::vector<std::string>> m_switchLinkNodeTypeIndex;
		std::string m_topoFileName;
		std::string m_linkAttr;
		unsigned int m_linkNum;
		std::string UintToStr(unsigned int temp);

	};
}

#endif /* FATTREE_TOPO_HELPER_H */

