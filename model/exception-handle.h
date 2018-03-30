#ifndef EXCEPTION_HANDLE_H
#define EXCEPTION_HANDLE_H

#include<exception>
#include<string>

namespace ns3 {

	unsigned int const PARAMETER_NUM_ERROR = 0;
	unsigned int const MATCH_KEY_NUM_ERROR = 1;
	unsigned int const MATCH_KEY_TYPE_ERROR = 2;
	unsigned int const ACTION_DATA_NUM_ERROR = 3;
	unsigned int const ACTION_DATA_TYPE_ERROR = 4;
	unsigned int const NO_SUCCESS = 5;
	unsigned int const COMMAND_ERROR = 6;
	unsigned int const METER_NO_EXIST = 7;
	unsigned int const COUNTER_NO_EXIST = 8;
	unsigned int const P4_SWITCH_POINTER_NULL=9;
	unsigned int const MATCH_TYPE_ERROR=10;
	unsigned int const OTHER_ERROR = 20;

	class P4Exception :public std::exception
	{
	public:
		P4Exception(unsigned int code = OTHER_ERROR, const std::string& entry = "")
		{
			m_exceptionCode = code;
			m_entry = entry;
		}
		const char * what() const throw ()
		{
			switch (m_exceptionCode)
			{

			case PARAMETER_NUM_ERROR:return "PARAMETER_NUM_ERROR";
			case MATCH_KEY_NUM_ERROR:return "MATCH_KEY_NUM_ERROR";
			case MATCH_KEY_TYPE_ERROR:return "MATCH_KEY_TYPE_ERROR";
			case ACTION_DATA_NUM_ERROR:return "ACTION_DATA_NUM_ERROR";
			case ACTION_DATA_TYPE_ERROR:return "ACTION_DATA_TYPE_ERROR";
			case NO_SUCCESS:return "NO_SUCCESS";
			case COMMAND_ERROR:return "COMMAND_ERROR";
			case METER_NO_EXIST:return "METER_NO_EXIST";
			case COUNTER_NO_EXIST:return "COUNTER_NO_EXIST";
			case P4_SWITCH_POINTER_NULL:return "P4_SWITCH_POINTER_NULL";
			case MATCH_TYPE_ERROR:return "MATCH_TYPE_ERROR";

			default:return "OTHER_ERROR";
			}
		}
		std::string info()
		{
			return m_entry;
		}
		~P4Exception() {}

	private:
		unsigned int m_exceptionCode;
		std::string m_entry;
	};

	void ShowExceptionEntry(const std::string &entry);
}


#endif // !EXCEPTION_HANDLE_H


