#ifndef P4_SWITCH_INTERFACE_H
#define P4_SWITCH_INTERFACE_H

#include "ns3/p4-net-device.h"
#include "ns3/switch-api.h"
#include "ns3/p4-controller.h"
#include "ns3/object.h"
#include "ns3/p4-model.h"
#include <vector>
#include <string>
#include <unordered_map>


namespace ns3 {

	class P4Model;

	struct Meter_t { // meter attribute
		bool isDirect;
		std::string tableName;
	public:
		Meter_t() {}
		Meter_t(bool isD) {
			isDirect = isD;
		}
		Meter_t(bool isD,const std::string& name)
		{
			isDirect=isD;
			tableName=name;
		}
	};

	struct Counter_t {
		bool isDirect;
		std::string tableName;
	public:
		Counter_t() {}
		Counter_t(bool isD) {
			isDirect = isD;
		}
		Counter_t(bool isD, const std::string& name) {
			isDirect = isD;
			tableName = name;
		}
	};

	struct FlowTable_t {
		bm::MatchKeyParam::Type matchType;
	public:
		FlowTable_t() {}
		FlowTable_t(bm::MatchKeyParam::Type mt) {
			matchType = mt;
		}
	};

	class P4SwitchInterface :public Object
	{

	public:

		friend class P4Controller;

		P4SwitchInterface();

		~P4SwitchInterface();

		static TypeId GetTypeId(void);

		void SetP4Model(P4Model *model)
		{
			m_p4Model = model;
		}

		void SetJsonPath(const std::string& path)
		{
			m_jsonPath = path;
		}

		void SetP4InfoPath(const std::string& path)
		{
			m_p4InfoPath = path;
		}

		void SetFlowTablePath(const std::string& path)
		{
			m_flowTablePath = path;
		}

		void SetViewFlowTablePath(const std::string&path)
		{
			m_viewFlowTablePath=path;
		}

		void SetNetworkFunc(unsigned int func)
		{
			m_networkFunc = func;
		}

		P4Model* GetP4Model()
		{
			return m_p4Model;
		}

		std::string GetJsonPath()
		{
			return m_jsonPath;
		}

		std::string GetP4InfoPath()
		{
			return m_p4InfoPath;
		}

		std::string GetFlowTablePath()
		{
			return m_flowTablePath;
		}

		std::string GetViewFlowTablePath()
		{
			return m_viewFlowTablePath;
		}

		unsigned int GetNetworkFunc()
		{
			return m_networkFunc;
		}

		void PopulateFlowTable();

		void ReadP4Info();

		void ViewFlowtableEntryNum();

		void AttainSwitchFlowTableInfo();

		void ParseAttainFlowTableInfoCommand(const std::string commandRow);

		void ParsePopulateFlowTableCommand(const std::string commandRow);

		void Init();

	private:

		P4Model *m_p4Model;
		std::string m_jsonPath;
		std::string m_p4InfoPath;
		std::string m_flowTablePath;
		std::string m_viewFlowTablePath;
		unsigned int m_networkFunc;



		std::unordered_map<std::string, Meter_t> m_meter;

		std::unordered_map<std::string, FlowTable_t> m_flowTable;

		std::unordered_map<std::string, Counter_t> m_counter;

		P4SwitchInterface(const P4SwitchInterface&);

		P4SwitchInterface& operator= (const P4SwitchInterface&);
	};

}

#endif // !P4_SWITCH_INTERFACE_H


