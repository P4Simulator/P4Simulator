#include "ns3/log.h"
#include "ns3/p4-switch-interface.h"
#include <fstream>
#include <unordered_map>
#include <iostream>
#include <string>
#include "ns3/exception-handle.h"
#include "ns3/helper.h"

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE("P4SwitchInterface");

	NS_OBJECT_ENSURE_REGISTERED(P4SwitchInterface);

	TypeId P4SwitchInterface::GetTypeId(void)
	{
		static TypeId tid = TypeId("ns3::P4SwitchInterface")
			.SetParent<Object>()
			.SetGroupName("P4SwitchInterface")
			;
		return tid;
	}

	P4SwitchInterface::P4SwitchInterface()
	{
		NS_LOG_FUNCTION(this);
	}

	P4SwitchInterface::~P4SwitchInterface()
	{
                //********TO DO (Whether need delete m_p4Model)*******************
		/*if(m_p4Model!=NULL)
		{
			delete m_p4Model;
			m_p4Model=NULL;
		}*/
                //***************************************************************
		NS_LOG_FUNCTION(this);
	}

	void P4SwitchInterface::PopulateFlowTable()
	{
		std::fstream fp;
		fp.open(m_flowTablePath);
		if (!fp)
		{
			std::cout << "in P4Model::PopulateFlowTable, " << m_flowTablePath << " can't open." << std::endl;
		}
		else
		{
			const int maxSize = 500;
			char row[maxSize];
			while (fp.getline(row, maxSize))
			{
				ParsePopulateFlowTableCommand(std::string(row));
				//ParseFlowtableCommand(std::string(row));
			}
		}
	}

	void P4SwitchInterface::ReadP4Info()
	{
		std::ifstream topgen;
		topgen.open(m_p4InfoPath);

		if (!topgen.is_open())
		{
			std::cout << m_p4InfoPath << " can not open!" << std::endl;
			abort();
		}

		std::istringstream lineBuffer;
		std::string line;

		std::string elementType;
		std::string tableName;
		std::string matchType;
		std::string meterName;
		unsigned int isDirect;
		std::string counterName;

		while (getline(topgen, line))
                {
                        lineBuffer.clear();
                        lineBuffer.str(line);

                        lineBuffer >> elementType;
                        if(elementType.compare("table")==0)
                        {
                                lineBuffer>>tableName>>matchType;
                                if (matchType.compare("exact") == 0)
                                {
                                        m_flowTable[tableName].matchType = bm::MatchKeyParam::Type::EXACT;
                                }
                                else
                                {
                                        if (matchType.compare("lpm") == 0)
                                        {
                                                m_flowTable[tableName].matchType = bm::MatchKeyParam::Type::LPM;
                                        }
                                        else
                                        {
                                                if (matchType.compare("ternary") == 0)
                                                {
                                                        m_flowTable[tableName].matchType = bm::MatchKeyParam::Type::TERNARY;
                                                }
                                                else
                                                {
                                                        if (matchType.compare("valid") == 0)
                                                        {
                                                                m_flowTable[tableName].matchType = bm::MatchKeyParam::Type::VALID;
                                                        }
                                                        else
                                                        {
                                                                if (matchType.compare("range") == 0)
                                                                {
                                                                        m_flowTable[tableName].matchType = bm::MatchKeyParam::Type::RANGE;
                                                                }
                                                                else
                                                                {
                                                                        std::cerr<< "p4-switch-interface.cc ReadP4Info() MatchType undefined!!!" << std::endl;
                                                                }
                                                        }
                                                }
                                        }
                                }
                                continue;
                        }
                        if(elementType.compare("meter")==0)
                        {
                                lineBuffer>>meterName>>isDirect>>tableName;
								m_meter[meterName].tableName=tableName;
                                if(isDirect==1)
                                        m_meter[meterName].isDirect=true;
                                else
                                        m_meter[meterName].isDirect=false;
                                continue;
                        }
                        if(elementType.compare("counter")==0)
                        {
                                lineBuffer>>counterName>>isDirect>>tableName;
                                if(isDirect==1)
                                {
                                        m_counter[counterName].isDirect=true;
                                        m_counter[counterName].tableName=tableName;
                                }
                                else
                                {
                                        m_counter[counterName].isDirect=false;
                                        m_counter[counterName].tableName=tableName;
                                }
                                
                                continue;
                        }
                        std::cerr<<"p4-switch-interface.cc ReadP4Info() ElementType undefined!!!"<<std::endl;
                }



	}

	void P4SwitchInterface::ViewFlowtableEntryNum()
	{
		//table_num_entries <table name>
		typedef std::unordered_map<std::string, FlowTable_t>::iterator FlowTableIter_t;
		for (FlowTableIter_t iter = m_flowTable.begin(); iter != m_flowTable.end(); ++iter)
		{
			std::string parm("table_num_entries ");
			ParseAttainFlowTableInfoCommand(parm + iter->first);

			//size_t num_entries;
			//mt_get_num_entries(0, iter->first, &num_entries);
			//std::cout << iter->first << " entry num: " << num_entries << std::endl;
		}
	}

	void P4SwitchInterface::AttainSwitchFlowTableInfo()
	{
		std::fstream fp;
		fp.open(m_viewFlowTablePath);
		if (!fp)
		{
			std::cout << "AttainSwitchFlowTableInfo, " << m_viewFlowTablePath << " can't open." << std::endl;
		}
		else
		{
			const int maxSize = 500;
			char row[maxSize];
			while (fp.getline(row, maxSize))
			{
				ParseAttainFlowTableInfoCommand(std::string(row));
			}
		}
	}

	void P4SwitchInterface::ParseAttainFlowTableInfoCommand(const std::string commandRow)
	{
		std::vector<std::string> parms;
		int lastP = 0, curP = 0;
		for (size_t i = 0; i < commandRow.size(); i++, curP++)
		{
			if (commandRow[i] == ' ')
			{
				parms.push_back(commandRow.substr(lastP, curP - lastP));
				lastP = i + 1;
			}
		}
		if (lastP < curP)
		{
			parms.push_back(commandRow.substr(lastP, curP - lastP));
		}
		if (parms.size() > 0)
		{
			unsigned int commandType = SwitchApi::g_apiMap[parms[0]];
			try {
				if (m_p4Model == NULL)
					throw P4Exception(P4_SWITCH_POINTER_NULL);
				switch (commandType)
				{
				case TABLE_NUM_ENTRIES: { //table_num_entries <table name>
					try {
						if (parms.size() == 2)
						{
							size_t num_entries;
							if (m_p4Model->mt_get_num_entries(0, parms[1], &num_entries) != bm::MatchErrorCode::SUCCESS)
								throw P4Exception(NO_SUCCESS);
							std::cout << parms[1] << " entry num: " << num_entries << std::endl;
						}
						else
							throw P4Exception(PARAMETER_NUM_ERROR);
					}
					catch (P4Exception& e)
					{
						std::cerr << e.what() << std::endl;
						ShowExceptionEntry(commandRow);
					}
					catch (...)
					{
						ShowExceptionEntry(commandRow);
					}
					break;
				}
				case TABLE_CLEAR: {// table_clear <table name>
					try {
						if (parms.size() == 2)
						{
							if (m_p4Model->mt_clear_entries(0, parms[1], false) != bm::MatchErrorCode::SUCCESS)
								throw P4Exception(NO_SUCCESS);
						}
						else
							throw P4Exception(PARAMETER_NUM_ERROR);
					}
					catch (P4Exception& e)
					{
						std::cerr << e.what() << std::endl;
						ShowExceptionEntry(commandRow);
					}
					catch (...)
					{
						ShowExceptionEntry(commandRow);
					}
					break;
				}
				case METER_GET_RATES: {//meter_get_rates <name> <index>
					try {
						if (parms.size() == 3)
						{
							if (m_meter.count(parms[1]) > 0)
							{
								if (m_meter[parms[1]].isDirect)//direct
								{
									bm::entry_handle_t handle(StrToInt(parms[2]));
									std::vector<bm::Meter::rate_config_t> configs;
									if (m_p4Model->mt_get_meter_rates(0, m_meter[parms[1]].tableName, handle, &configs) != bm::MatchErrorCode::SUCCESS)
										throw P4Exception(NO_SUCCESS);
									for (size_t i = 0; i < configs.size(); i++)
									{
										std::cout << "info_rate:" << configs[i].info_rate << " burst_size:" << configs[i].burst_size << std::endl;
									}
								}
								else//indirect
								{
									size_t idx(StrToInt(parms[2]));
									std::vector<bm::Meter::rate_config_t> configs;
									if (m_p4Model->meter_get_rates(0, parms[1], idx, &configs) != 0)
										throw P4Exception(NO_SUCCESS);
									for (size_t i = 0; i < configs.size(); i++)
									{
										std::cout << "info_rate:" << configs[i].info_rate << " burst_size:" << configs[i].burst_size << std::endl;
									}
								}
							}
							else
								throw P4Exception(METER_NO_EXIST);
						}
						else
							throw P4Exception(PARAMETER_NUM_ERROR);
					}
					catch (P4Exception& e)
					{
						std::cerr << e.what() << std::endl;
						ShowExceptionEntry(commandRow);
					}
					catch (...)
					{
						ShowExceptionEntry(commandRow);
					}
					break;
				}
				case COUNTER_READ: { //counter_read <name> <index>
					try {
						if (parms.size() == 3)
						{
							if (m_counter.count(parms[1]) > 0)
							{
								if (m_counter[parms[1]].isDirect)//direct
								{
									bm::entry_handle_t handle(StrToInt(parms[2]));
									bm::MatchTableAbstract::counter_value_t bytes;
									bm::MatchTableAbstract::counter_value_t packets;
									if (m_p4Model->mt_read_counters(0, m_counter[parms[1]].tableName, handle, &bytes, &packets) != bm::MatchErrorCode::SUCCESS)
										throw P4Exception(NO_SUCCESS);
									std::cout << "counter " << parms[1] << "[" << handle << "] size:" << bytes << " bytes " << packets << " packets" << std::endl;
								}
								else
								{
									size_t index(StrToInt(parms[2]));
									bm::MatchTableAbstract::counter_value_t bytes;
									bm::MatchTableAbstract::counter_value_t packets;
									if (m_p4Model->read_counters(0, parms[1], index, &bytes, &packets) != 0)
										throw P4Exception(NO_SUCCESS);
									std::cout << "counter " << parms[1] << "[" << index << "] size:" << bytes << " bytes " << packets << " packets" << std::endl;
								}
							}
							else
								throw P4Exception(COUNTER_NO_EXIST);
						}
						else
							throw P4Exception(PARAMETER_NUM_ERROR);
					}
					catch (P4Exception& e)
					{
						std::cerr << e.what() << std::endl;
						ShowExceptionEntry(commandRow);
					}
					catch (...)
					{
						ShowExceptionEntry(commandRow);
					}
					break;
				}
				case COUNTER_RESET: {//counter_reset <name>
					try {
						if (parms.size() == 2)
						{
							if (m_counter.count(parms[1]) > 0)
							{
								if (m_counter[parms[1]].isDirect)//direct
								{
									if (m_p4Model->mt_reset_counters(0, m_counter[parms[1]].tableName) != bm::MatchErrorCode::SUCCESS)
										throw P4Exception(NO_SUCCESS);
								}
								else //indirect
								{
									if (m_p4Model->reset_counters(0, parms[1]) != 0)
										throw P4Exception(NO_SUCCESS);
								}
							}
							else
								throw P4Exception(COUNTER_NO_EXIST);
						}
						else
							throw P4Exception(PARAMETER_NUM_ERROR);
					}
					catch (P4Exception& e)
					{
						std::cerr << e.what() << std::endl;
						ShowExceptionEntry(commandRow);
					}
					catch (...)
					{
						ShowExceptionEntry(commandRow);
					}
					break;
				}
				case REGISTER_READ: {//register_read <name> [index]
					try {
						if (parms.size() == 3)
						{
							size_t index(StrToInt(parms[2]));
							bm::Data value;
							if (m_p4Model->register_read(0, parms[1], index, &value) != 0)
								throw P4Exception(NO_SUCCESS);
							std::cout << "register " << parms[1] << "[" << index << "] value :" << value << std::endl;
						}
						else
							throw P4Exception(PARAMETER_NUM_ERROR);
					}
					catch (P4Exception& e)
					{
						std::cerr << e.what() << std::endl;
						ShowExceptionEntry(commandRow);
					}
					catch (...)
					{
						ShowExceptionEntry(commandRow);
					}
					break;
				}
				case REGISTER_WRITE: { //register_write <name> <index> <value>
					try {
						if (parms.size() == 4)
						{
							size_t index(StrToInt(parms[2]));
							bm::Data value(parms[3]);
							if (m_p4Model->register_write(0, parms[1], index, value) != 0)
								throw P4Exception(NO_SUCCESS);
						}
						else
							throw P4Exception(PARAMETER_NUM_ERROR);
					}
					catch (P4Exception& e)
					{
						std::cerr << e.what() << std::endl;
						ShowExceptionEntry(commandRow);
					}
					catch (...)
					{
						ShowExceptionEntry(commandRow);
					}
					break;
				}
				case REGISTER_RESET: {//register_reset <name>
					try {
						if (parms.size() == 2)
						{
							if (m_p4Model->register_reset(0, parms[1]) != 0)
								throw P4Exception(NO_SUCCESS);
						}
						else
							throw P4Exception(PARAMETER_NUM_ERROR);
					}
					catch (P4Exception& e)
					{
						std::cerr << e.what() << std::endl;
						ShowExceptionEntry(commandRow);
					}
					catch (...)
					{
						ShowExceptionEntry(commandRow);
					}
					break;
				}
				case TABLE_DUMP_ENTRY: {// table_dump_entry <table name> <entry handle>
					try {
						if (parms.size() == 3)
						{
							bm::entry_handle_t handle(StrToInt(parms[2]));
							bm::MatchTable::Entry entry;
							if (m_p4Model->mt_get_entry(0, parms[1], handle, &entry) != bm::MatchErrorCode::SUCCESS)
								throw P4Exception(NO_SUCCESS);
							std::cout << parms[1] << " entry " << handle << " :" << std::endl;
							std::cout << "MatchKey:";
							for (size_t i = 0; i < entry.match_key.size(); i++)
							{
								std::cout << entry.match_key[i].key << " ";
							}
							std::cout << std::endl << "ActionData:";
							for (size_t i = 0; i < entry.action_data.action_data.size(); i++)
							{
								std::cout << entry.action_data.action_data[i] << " ";
							}
							std::cout << std::endl;
						}
						else
							throw P4Exception(PARAMETER_NUM_ERROR);
					}
					catch (P4Exception& e)
					{
						std::cerr << e.what() << std::endl;
						ShowExceptionEntry(commandRow);
					}
					catch (...)
					{
						ShowExceptionEntry(commandRow);
					}
					break;
				}
				case TABLE_DUMP: {//table_dump <table name>
					try {
						if (parms.size() == 2)
						{
							std::vector<bm::MatchTable::Entry> entries;
							entries = m_p4Model->mt_get_entries(0, parms[1]);
							// TO DO: output entries info
						}
						else
							throw P4Exception(PARAMETER_NUM_ERROR);
					}
					catch (P4Exception& e)
					{
						std::cerr << e.what() << std::endl;
						ShowExceptionEntry(commandRow);
					}
					catch (...)
					{
						ShowExceptionEntry(commandRow);
					}
					break;
				}
				default: {
					throw P4Exception(COMMAND_ERROR);
					break;
				}
				}
			}
			catch (P4Exception& e)
			{
				std::cerr << e.what() << std::endl;
			}
		}
	}

	void P4SwitchInterface::ParsePopulateFlowTableCommand(const std::string commandRow)
	{
		std::vector<std::string> parms;
		int lastP = 0, curP = 0;
		for (size_t i = 0; i < commandRow.size(); i++, curP++)
		{
			if (commandRow[i] == ' ')
			{
				parms.push_back(commandRow.substr(lastP, curP - lastP));
				lastP = i + 1;
			}
		}
		if (lastP < curP)
		{
			parms.push_back(commandRow.substr(lastP, curP - lastP));
		}
		if (parms.size() > 0)
		{
			unsigned int commandType = SwitchApi::g_apiMap[parms[0]];
			try {
				if (m_p4Model == NULL)
					throw P4Exception(P4_SWITCH_POINTER_NULL);
				switch (commandType)
				{
				case TABLE_SET_DEFAULT: {//table_set_default <table name> <action name> <action parameters>
					try {
						//NS_LOG_LOGIC("TABLE_SET_DEFAULT:"<<commandRow);
						if (parms.size() >= 3) {
							bm::ActionData actionData;
							if (parms.size() > 3)// means have ActionData
							{
								for (size_t i = 3; i < parms.size(); i++)
									actionData.push_back_action_data(bm::Data(parms[i]));
							}
							if (m_p4Model->mt_set_default_action(0, parms[1], parms[2], actionData) != bm::MatchErrorCode::SUCCESS)
								throw P4Exception(NO_SUCCESS);
						}
						else
							throw P4Exception(PARAMETER_NUM_ERROR);
					}
					catch (P4Exception& e)
					{
						std::cerr << e.what() << std::endl;
						ShowExceptionEntry(commandRow);
					}
					catch (...)
					{
						ShowExceptionEntry(commandRow);
					}
					break;
				}
										// Just support hexadecimal match fields, action parameters

				case TABLE_ADD: {//table_add <table name> <action name> <match fields> => <action parameters> [priority]
					try {
						//NS_LOG_LOGIC("TABLE_ADD:"<<commandRow);
						std::vector<bm::MatchKeyParam> matchKey;
						bm::ActionData actionData;
						bm::entry_handle_t handle;
						bm::MatchKeyParam::Type matchType = m_flowTable[parms[1]].matchType;
						unsigned int keyNum = 0;
						unsigned int actionDataNum = 0;
						size_t i;
						for (i = 3; i < parms.size(); i++)
						{
							if (parms[i].compare("=>") != 0)
							{
								keyNum++;
								switch (matchType)
								{
								case bm::MatchKeyParam::Type::EXACT:
								{
									//NS_LOG_LOGIC("EXACT");
									matchKey.push_back(bm::MatchKeyParam(matchType, HexstrToBytes(parms[i])));
									break;
								}
								case bm::MatchKeyParam::Type::LPM:
								{
									int pos = parms[i].find("/");
									std::string prefix = parms[i].substr(0, pos);
									std::string length = parms[i].substr(pos + 1);
									unsigned int prefixLength = StrToInt(length);
									matchKey.push_back(bm::MatchKeyParam(matchType, HexstrToBytes(parms[i], prefixLength), int(prefixLength)));
									break;
								}
								case bm::MatchKeyParam::Type::TERNARY:
								{
									int pos = parms[i].find("&&&");
									std::string key = HexstrToBytes(parms[i].substr(0, pos));
									std::string mask = HexstrToBytes(parms[i].substr(pos + 3));
									if (key.size() != mask.size())
									{
										std::cerr << "key and mask length unmatch!" << std::endl;
									}
									else
									{
										matchKey.push_back(bm::MatchKeyParam(matchType, key, mask));
									}
									break;
								}
								case bm::MatchKeyParam::Type::RANGE:
								{
									// TO DO: handle range match type
									break;
								}
								case bm::MatchKeyParam::Type::VALID:
								{
									// TO DO: handle valid match type
									break;
								}
								default:
								{
									throw P4Exception(MATCH_TYPE_ERROR);
									break;
								}
								}
							}
							else
								break;
						}
						i++;
						int priority;
						//TO DO:judge key_num equal table need key num
						if (matchType != bm::MatchKeyParam::Type::TERNARY&&matchType != bm::MatchKeyParam::Type::RANGE)
						{
							//NS_LOG_LOGIC("Parse ActionData from index:"<<i);
							for (; i < parms.size(); i++)
							{
								actionDataNum++;
								actionData.push_back_action_data(bm::Data(parms[i]));
							}
							priority = 0;
							//TO DO:judge action_data_num equal action need num
							if (m_p4Model->mt_add_entry(0, parms[1], matchKey, parms[2], actionData, &handle, priority) != bm::MatchErrorCode::SUCCESS)
								throw P4Exception(NO_SUCCESS);
						}
						else
						{
							for (; i < parms.size() - 1; i++)
							{
								actionDataNum++;
								actionData.push_back_action_data(bm::Data(parms[i]));
							}
							//TO DO:judge action_data_num equal action need num
							priority = StrToInt(parms[parms.size() - 1]);
							if (m_p4Model->mt_add_entry(0, parms[1], matchKey, parms[2], actionData, &handle, priority) != bm::MatchErrorCode::SUCCESS)
								throw P4Exception(NO_SUCCESS);
						}
					}
					catch (P4Exception& e)
					{
						std::cerr << e.what() << std::endl;
						ShowExceptionEntry(commandRow);
					}
					catch (...)
					{
						ShowExceptionEntry(commandRow);
					}
					break;
				}
				case TABLE_SET_TIMEOUT: { //table_set_timeout <table_name> <entry handle> <timeout (ms)>
					try {
						if (parms.size() == 4)
						{
							bm::entry_handle_t handle(StrToInt(parms[2]));
							unsigned int ttl_ms(StrToInt(parms[3]));
							if (m_p4Model->mt_set_entry_ttl(0, parms[1], handle, ttl_ms) != bm::MatchErrorCode::SUCCESS)
								throw P4Exception(NO_SUCCESS);
						}
						else
							throw P4Exception(PARAMETER_NUM_ERROR);
					}
					catch (P4Exception& e)
					{
						std::cerr << e.what() << std::endl;
						ShowExceptionEntry(commandRow);
					}
					catch (...)
					{
						ShowExceptionEntry(commandRow);
					}
					break;
				}
				case TABLE_MODIFY: {//table_modify <table name> <action name> <entry handle> [action parameters]
					try {
						if (parms.size() >= 4) {
							bm::ActionData actionData;
							bm::entry_handle_t handle(StrToInt(parms[3]));
							unsigned int actionDataNum = 0;
							for (size_t i = 4; i < parms.size(); i++)
							{
								actionDataNum++;
								actionData.push_back_action_data(bm::Data(parms[i]));
							}
							//TO DO:judge action_data_num equal action need num
							if (m_p4Model->mt_modify_entry(0, parms[1], handle, parms[2], actionData) != bm::MatchErrorCode::SUCCESS)
								throw P4Exception(NO_SUCCESS);
						}
						else
							throw P4Exception(PARAMETER_NUM_ERROR);
					}
					catch (P4Exception& e)
					{
						std::cerr << e.what() << std::endl;
						ShowExceptionEntry(commandRow);
					}
					catch (...)
					{
						ShowExceptionEntry(commandRow);
					}
					break;
				}
				case TABLE_DELETE: {// table_delete <table name> <entry handle>
					try {
						if (parms.size() == 3)
						{
							bm::entry_handle_t handle(StrToInt(parms[2]));
							if (m_p4Model->mt_delete_entry(0, parms[1], handle) != bm::MatchErrorCode::SUCCESS)
								throw P4Exception(NO_SUCCESS);
						}
						else
							throw P4Exception(PARAMETER_NUM_ERROR);

					}
					catch (P4Exception& e)
					{
						std::cerr << e.what() << std::endl;
						ShowExceptionEntry(commandRow);
					}
					catch (...)
					{
						ShowExceptionEntry(commandRow);
					}
					break;
				}
				case METER_ARRAY_SET_RATES: {//meter_array_set_rates <name> <rate_1>:<burst_1> <rate_2>:<burst_2> ...
					try {
						if (parms.size() > 2) {
							std::vector<bm::Meter::rate_config_t> configs;
							for (size_t i = 2; i < parms.size(); i++)
							{
								int pos = parms[i].find(":");// may be ':' better, should think more...
								std::string rate = parms[i].substr(0, pos);
								std::string burst = parms[i].substr(pos + 1);
								bm::Meter::rate_config_t rateConfig;
								rateConfig.info_rate = StrToDouble(rate);
								rateConfig.burst_size = StrToInt(burst);
								configs.push_back(rateConfig);
							}
							if (m_p4Model->meter_array_set_rates(0, parms[1], configs) != 0)
								throw P4Exception(NO_SUCCESS);
						}
						else
							throw P4Exception(PARAMETER_NUM_ERROR);
					}
					catch (P4Exception& e)
					{
						std::cerr << e.what() << std::endl;
						ShowExceptionEntry(commandRow);
					}
					catch (...)
					{
						ShowExceptionEntry(commandRow);
					}
					break;
				}
				case METER_SET_RATES: {// meter_set_rates <name> <index> <rate_1>:<burst_1> <rate_2>:<burst_2> ...
					try {
						if (parms.size() > 3) {
							std::vector<bm::Meter::rate_config_t> configs;
							for (size_t i = 3; i < parms.size(); i++)
							{
								int pos = parms[i].find(":");
								std::string rate = parms[i].substr(0, pos);
								std::string burst = parms[i].substr(pos + 1);
								bm::Meter::rate_config_t rateConfig;
								rateConfig.info_rate = StrToDouble(rate);
								rateConfig.burst_size = StrToInt(burst);
								configs.push_back(rateConfig);
							}
							if (m_meter[parms[1]].isDirect)//direct
							{
								bm::entry_handle_t handle(StrToInt(parms[2]));
								if (m_p4Model->mt_set_meter_rates(0, m_meter[parms[1]].tableName, handle, configs) != bm::MatchErrorCode::SUCCESS)
									throw P4Exception(NO_SUCCESS);
							}
							else//indirect
							{
								size_t idx(StrToInt(parms[2]));
								if (m_p4Model->meter_set_rates(0, parms[1], idx, configs) != 0)
									throw P4Exception(NO_SUCCESS);
							}
						}
						else
							throw P4Exception(PARAMETER_NUM_ERROR);
					}
					catch (P4Exception& e)
					{
						std::cerr << e.what() << std::endl;
						ShowExceptionEntry(commandRow);
					}
					catch (...)
					{
						ShowExceptionEntry(commandRow);
					}
					break;
				}
				default:
				{
					throw P4Exception(COMMAND_ERROR);
					break;
				}
				}
			}
			catch (P4Exception&e) {
				std::cerr << e.what() << std::endl;
			}
		}
	}

	void P4SwitchInterface::Init()
	{
		ReadP4Info();
		PopulateFlowTable();
		//ViewFlowtableEntryNum();
	}
}

