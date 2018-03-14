/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

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
#ifndef FATTREE_TOPO_HELPER_H
#define FATTREE_TOPO_HELPER_H

#include <vector>
#include <string>

namespace ns3 {
	/**
	*
	* \brief build fattree topo
	*/
	struct LinkNodeTypeIndex_t
	{

		char type;
		unsigned int index;
	public:
		LinkNodeTypeIndex_t(char t,unsigned int i)
		{
			type=t;
			index=i;
		}
	};

	class FattreeTopoHelper {
	public:
		/*
		* Construct a FattreeTopoHelper
		*/
		FattreeTopoHelper(unsigned int podNum, std::string topoFileName = "topo.txt");
		~FattreeTopoHelper();

		unsigned int GetSwitchNum();
		unsigned int GetPodNum();
		unsigned int GetTerminalNum();
		void SetLinkDataRate(std::string dataRate = "");
		std::string GetLinkDataRate();
		void SetLinkDelay(std::string delay = "");
		std::string GetLinkDelay();
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
		std::vector<std::vector<LinkNodeTypeIndex_t>> m_switchLinkNodeTypeIndex;
		std::string m_topoFileName;
		std::string m_linkDataRate; // eg: 1000Mbps
		std::string m_linkDelay; // eg: 0.01ms
		unsigned int m_linkNum;
		std::string UintToStr(unsigned int temp);

	};
}

#endif /* FATTREE_TOPO_HELPER_H */


