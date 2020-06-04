/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef IPV4_CLOVE_H
#define IPV4_CLOVE_H

#include "ns3/object.h"
#include "ns3/callback.h"
#include "ns3/traced-value.h"
#include "ns3/nstime.h"
#include "ns3/ipv4-address.h"
#include "ns3/node.h"

#include <vector>
#include <map>
#include <utility>

#define CLOVE_RUNMODE_EDGE_FLOWLET 0
#define CLOVE_RUNMODE_ECN 1
#define CLOVE_RUNMODE_INT 2

namespace ns3 {

struct CloveFlowlet {
    Time lastSeen;
	Time lastDecisionnMade;
	uint32_t maxCntLog;
    uint32_t path;
    uint32_t old_path;
	uint32_t groupID;
	uint32_t groupPktNum;
	uint32_t longFlow;
	uint32_t neverChangePath;
	// uint32_t avgAvailWinSize;
	uint32_t numTimeouts;
};

struct GroupStats {
	uint32_t ECN;
	uint32_t noECN;
	uint32_t unknown;
};

class Ipv4Clove : public Object {

public:
    Ipv4Clove ();
    Ipv4Clove (const Ipv4Clove&);

    static TypeId GetTypeId (void);

    void AddAddressWithTor (Ipv4Address address, uint32_t torId);
    void AddAvailPath (uint32_t destTor, uint32_t path);

    uint32_t GetPath (uint32_t flowId, Ipv4Address saddr, Ipv4Address daddr, uint32_t ackNum, uint32_t remainingData, uint32_t availableWin);

    void FlowRecv (uint32_t path, Ipv4Address daddr, bool withECN, uint32_t flowId, int ackNum);

    bool FindTorId (Ipv4Address daddr, uint32_t &torId);

	// salvatorg
    TracedCallback < uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, Ipv4Address, Ipv4Address, Ptr<Node> > m_pathTrace;
    typedef void (* ClovePathCallback) (uint32_t flowId, uint32_t fromTor, uint32_t toTor,
            uint32_t newPath, uint32_t oldPath, Ipv4Address saddr, Ipv4Address daddr, Ptr<Node> nodeID);
    // TracedCallback <uint32_t> m_queuesTrace;
    // typedef void (* CloveQueuesCallback) (uint32_t path);
	
	// salvatorg
	// <pair<flowID,ackNum>, pair<GroupID,ecnSeen>>
	std::map<std::pair<uint32_t, int>, std::pair<uint32_t, int> > m_trackFlowECNs;
	// <pair<flowID,GroupID>, pair<GroupStats>>
	std::map<std::pair<uint32_t, uint32_t>, GroupStats > m_trackFlowECNgroupStats;
	// <pair<flowID,ackNum>, groupID>
	std::map<std::pair<uint32_t, int>, uint32_t> m_trackFlowletGroups;
	// <pair<flowID,groupID>, ECNs, NoECNs>
	std::map< std::pair<uint32_t, uint32_t>, std::pair<uint32_t, uint32_t> > m_trackFlowletGroupsStats;

	void GetStats (void);
	void GetStatsLongFlows (void);

	uint32_t sumAvailWinSize;
	uint32_t cntDecisionsDiffPath;

private:
    uint32_t CalPath (uint32_t destTor);

    Time m_flowletTimeout;
    uint32_t m_runMode;

    std::map<uint32_t, std::vector<uint32_t> > m_availablePath;
    std::map<Ipv4Address, uint32_t> m_ipTorMap;
    std::map<uint32_t, CloveFlowlet> m_flowletMap;

    // Clove ECN
    Time m_halfRTT;
    bool m_disToUncongestedPath;
    std::map<std::pair<uint32_t, uint32_t>, double> m_pathWeight;
    std::map<std::pair<uint32_t, uint32_t>, Time> m_pathECNSeen;


//protected:


};
}

#endif /* IPV4_CLOVE_H */

