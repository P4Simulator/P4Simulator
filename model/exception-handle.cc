# include"ns3/exception-handle.h"
# include<iostream>

namespace ns3 {

	void ShowExceptionEntry(const std::string &entry)
	{
		std::cerr << "exception entry: " << entry << std::endl;
	}

}


