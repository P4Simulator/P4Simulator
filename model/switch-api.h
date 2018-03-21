#ifndef SWITCH_API_H
#define SWITCH_API_H

#include <unordered_map>
#include <string>

namespace ns3{

// some api used to populate flow table

//"Set default action for a match table: table_set_default <table name> <action name> <action parameters>"
unsigned int const TABLE_SET_DEFAULT = 0;

//"Add entry to a match table: table_add <table name> <action name> <match fields> => <action parameters> [priority]"
unsigned int const TABLE_ADD = 3;

//"Set a timeout in ms for a given entry; the table has to support timeouts: table_set_timeout <table_name> <entry handle> <timeout (ms)>"
unsigned int const TABLE_SET_TIMEOUT = 4;

//"Add entry to a match table: table_modify <table name> <action name> <entry handle> [action parameters]"
unsigned int const TABLE_MODIFY = 5;

//"Delete entry from a match table: table_delete <table name> <entry handle>"
unsigned int const TABLE_DELETE = 6;

//"Configure rates for an entire meter array: meter_array_set_rates <name> <rate_1>:<burst_1> <rate_2>:<burst_2> ..."
unsigned int const METER_ARRAY_SET_RATES = 7;

//"Configure rates for a meter: meter_set_rates <name> <index> <rate_1>:<burst_1> <rate_2>:<burst_2> ..."
unsigned int const METER_SET_RATES = 8;

//some api used to attain flow table information

//"Return the number of entries in a match table (direct or indirect): table_num_entries <table name>"
unsigned int const TABLE_NUM_ENTRIES = 1;

//"Clear all entries in a match table (direct or indirect), but not the default entry: table_clear <table name>"
unsigned int const TABLE_CLEAR = 2;

//"Retrieve rates for a meter: meter_get_rates <name> <index>"
unsigned int const METER_GET_RATES = 9;

//"Read counter value: counter_read <name> <index>"
unsigned int const COUNTER_READ = 10;

//"Reset counter: counter_reset <name>"
unsigned int const COUNTER_RESET = 11;

//"Read register value: register_read <name> [index]"
unsigned int const REGISTER_READ = 12;

//"Write register value: register_write <name> <index> <value>"
unsigned int const REGISTER_WRITE = 13;

//"Reset all the cells in the register array to 0: register_reset <name>"
unsigned int const REGISTER_RESET = 14;

//"Display some information about a table entry: table_dump_entry <table name> <entry handle>"
unsigned int const TABLE_DUMP_ENTRY = 15;

//"Display entries in a match-table: table_dump <table name>"
unsigned int const TABLE_DUMP = 16;

class SwitchApi{

  public:
    static std::unordered_map<std::string, unsigned int> g_apiMap;
    static void InitApiMap();

};

}

#endif
