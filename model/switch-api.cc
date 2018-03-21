# include "switch-api.h"

namespace ns3{

std::unordered_map<std::string, unsigned int> SwitchApi::g_apiMap;

void SwitchApi::InitApiMap(){

  g_apiMap["table_set_default"] = TABLE_SET_DEFAULT;
  g_apiMap["table_add"] = TABLE_ADD;
  g_apiMap["table_set_timeout"] = TABLE_SET_TIMEOUT;
  g_apiMap["table_modify"] = TABLE_MODIFY;
  g_apiMap["table_delete"] = TABLE_DELETE;
  g_apiMap["meter_array_set_rates"] = METER_ARRAY_SET_RATES;
  g_apiMap["meter_set_rates"] = METER_SET_RATES;
  g_apiMap["table_num_entries"] = TABLE_NUM_ENTRIES;
  g_apiMap["table_clear"] = TABLE_CLEAR;
  g_apiMap["meter_get_rates"] = METER_GET_RATES;
  g_apiMap["counter_read"] = COUNTER_READ;
  g_apiMap["counter_reset"] = COUNTER_RESET;
  g_apiMap["register_read"] = REGISTER_READ;
  g_apiMap["register_write"] = REGISTER_WRITE;
  g_apiMap["register_reset"] = REGISTER_RESET;
  g_apiMap["table_dump_entry"] = TABLE_DUMP_ENTRY;
  g_apiMap["table_dump"] = TABLE_DUMP;

}
}
