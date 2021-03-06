/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ipv4-clove.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv4Clove");

NS_OBJECT_ENSURE_REGISTERED (Ipv4Clove);

Ipv4Clove::Ipv4Clove () :
    m_flowletTimeout (MicroSeconds (200)),
    m_runMode (CLOVE_RUNMODE_EDGE_FLOWLET),
    m_halfRTT (MicroSeconds (40)),
    m_disToUncongestedPath (false)
	//m_pathTrace (this.m_pathTrace)
{
    NS_LOG_FUNCTION (this);
}

Ipv4Clove::Ipv4Clove (const Ipv4Clove &other) :
    m_flowletTimeout (other.m_flowletTimeout),
    m_runMode (other.m_runMode),
    m_halfRTT (other.m_halfRTT),
    m_disToUncongestedPath (other.m_disToUncongestedPath)
	//m_pathTrace (other.m_pathTrace)
{
    NS_LOG_FUNCTION (this);
}

TypeId
Ipv4Clove::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::Ipv4Clove")
        .SetParent<Object> ()
        .SetGroupName ("Clove")
        .AddConstructor<Ipv4Clove> ()
        .AddAttribute ("FlowletTimeout", "FlowletTimeout",
                       TimeValue (MicroSeconds (40)),
                       MakeTimeAccessor (&Ipv4Clove::m_flowletTimeout),
                       MakeTimeChecker ())
        .AddAttribute ("RunMode", "RunMode",
                       UintegerValue (0),
                       MakeUintegerAccessor (&Ipv4Clove::m_runMode),
                       MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("HalfRTT", "HalfRTT",
                       TimeValue (MicroSeconds (40)),
                       MakeTimeAccessor (&Ipv4Clove::m_halfRTT),
                       MakeTimeChecker ())
        .AddAttribute ("DisToUncongestedPath", "DisToUncongestedPath",
                       BooleanValue (false),
                       MakeBooleanAccessor (&Ipv4Clove::m_disToUncongestedPath),
                       MakeBooleanChecker ())
        .AddTraceSource ("ClovePathTrace",
                         "Check the new path taken cause of ECN",
                         MakeTraceSourceAccessor (&Ipv4Clove::m_pathTrace),
                         "ns3::Ipv4Clove::ClovePathCallback")
        // .AddTraceSource ("CloveQueuesTrace",
        //                  "Check the queue sizes when ECN occurs",
        //                  MakeTraceSourceAccessor (&Ipv4Clove::m_queuesTrace),
        //                  "ns3::Ipv4Clove::CloveQueuesCallback")
    ;

    return tid;
}

void
Ipv4Clove::AddAddressWithTor (Ipv4Address address, uint32_t torId)
{
    m_ipTorMap[address] = torId;
}

void
Ipv4Clove::AddAvailPath (uint32_t destTor, uint32_t path)
{
    std::map<uint32_t, std::vector<uint32_t> >::iterator itr = m_availablePath.find (destTor);
    if (itr == m_availablePath.end ())
    {
        std::vector<uint32_t> paths;
        m_availablePath[destTor] = paths;
        itr = m_availablePath.find (destTor);
    }
    (itr->second).push_back (path);

    std::pair<uint32_t, uint32_t> key = std::make_pair (destTor, path);
    m_pathWeight[key] = 1;
}

std::vector<uint32_t>
Ipv4Clove::GetAvailPath (Ipv4Address daddr)
{
    std::vector<uint32_t> emptyVector;
    uint32_t destTor = 0;
    if (!Ipv4Clove::FindTorId (daddr, destTor))
    {
        NS_LOG_ERROR ("Cannot find dest tor id based on the given dest address");
        return emptyVector;
    }

    std::map<uint32_t, std::vector<uint32_t> >::iterator itr = m_availablePath.find (destTor);
    if (itr == m_availablePath.end ())
    {
        return emptyVector;
    }
    return itr->second;
}

uint32_t
Ipv4Clove::GetPath (uint32_t flowId, Ipv4Address saddr, Ipv4Address daddr, uint32_t ackNum, uint32_t remainingData, uint32_t availableWin)
{
    uint32_t destTor = 0;
    if (!Ipv4Clove::FindTorId (daddr, destTor))
    {
        NS_LOG_ERROR ("Cannot find dest tor id based on the given dest address");
        return 0;
    }

    uint32_t sourceTor = 0;
    if (!Ipv4Clove::FindTorId (saddr, sourceTor))
    {
        NS_LOG_ERROR ("Cannot find source tor id based on the given source address");
		return 0;
    }

	// Avoid to count the call of this process in the handshake
	if (ackNum!=0 && remainingData!=0 && availableWin!=0)	numPacketsSent++;

    struct CloveFlowlet flowlet;
	uint32_t old_path;

    std::map<uint32_t, CloveFlowlet>::iterator flowletItr = m_flowletMap.find (flowId);
	// If the flow is in the flow table, get it
    if (flowletItr != m_flowletMap.end ())
    {
        flowlet = flowletItr->second;
		//  NS_LOG_UNCOND ("Clove Flow FOUND in the flow map . FlowID: "<< flowId << " selects path: "<<flowlet.path <<" , " << Simulator::Now () << " from " << sourceTor << " to " << destTor);
    }
	// otherwise calculate the path for the new flowlet and later add it to the map
	// This should happen only for a new flowlet
    else
    {
		flowlet.old_path	= flowlet.path;
		flowlet.longFlow	= 0;
		flowlet.shortFlow	= 0;
		flowlet.maxCntLog	= 0;
		flowlet.groupID		= 0;
		flowlet.neverChangePath = 1;
		// flowlet.avgAvailWinSize	= 0;
		flowlet.numTimeouts		= 0;
        flowlet.path		= Ipv4Clove::CalPath (destTor);
		// NS_LOG_UNCOND ("Clove Flow NOT FOUND in the flow map. FlowID: "<< flowId << " selects path: " << flowlet.path<< " last seen: " << flowlet.lastSeen << ", " << Simulator::Now ());
    }


	// If the flowlet has a timeout, then get the (new) path
	// If previously the flowlet was not found in the table , then this if statement
	// should always happen, assuming that flowlet.lastSeen=0 (from instatiation)
    if (Simulator::Now () - flowlet.lastSeen >= m_flowletTimeout)
    {
		// Groups numbering start from 1
		flowlet.groupID		+= 1;
		flowlet.old_path	= flowlet.path;
        flowlet.path		= Ipv4Clove::CalPath (destTor);
		// if(!isAck)
		// m_pathTrace(flowId,sourceTor,destTor,flowlet.path,m_flowletMap[flowId].path,saddr,daddr,nodeID);
		if(flowlet.old_path!=flowlet.path && flowlet.old_path!=0)	flowlet.neverChangePath	= 0;
		flowlet.maxCntLog			= 0;
		flowlet.lastDecisionnMade	= Simulator::Now ();
		flowlet.numTimeouts++;

	// NS_LOG_UNCOND ("\t>"<< saddr << " -> " << daddr);
	// NS_LOG_UNCOND ("\t> Flow: " << flowId << " (" << sourceTor << " -> "<< (flowlet.path-17)%100 <<" -> " << destTor << ") selects path: " << flowlet.path << "( "<< old_path <<" ) at: " << Simulator::Now () );
		// NS_LOG_DEBUG ("\tTIMEOUT Flowlet [ " << flowId << " ]  " << Simulator::Now () << " New Path: " << flowlet.path << " GroupID: " << flowlet.groupID << " LongFlow: " << flowlet.longFlow << " " " AvailableWin: " << availableWin);
	}

	// salvatorg
	// Tag the flow if it is long, so if we ever see remainingData >= 10000000 
	if(remainingData >= 10000000 && flowlet.longFlow==0)	flowlet.longFlow	= 1;
	// Tag the flow if it is short, so if we ever see remainingData <= 100000 and havent set the tag
	if(remainingData <= 100000 && flowlet.shortFlow==0)	flowlet.shortFlow	= 1;

	// salvatorg
	// Track v1 only the first n=10 packets for the ECN bit from the time of timeout, iff the path has changed
	if( (Simulator::Now () - flowlet.lastSeen <= m_flowletTimeout) && (flowlet.maxCntLog < 10) && (flowlet.old_path!=flowlet.path))
	{
		// Skip the handshake
		if( ackNum != 0 )
		{
			std::pair<uint32_t, int> key = std::make_pair (flowId, ackNum);
			// <pair<flowID,ackNum>, pair<GroupID,ecnSeen>>
			std::map<std::pair<uint32_t, int>, std::pair<uint32_t, int> >::iterator itr = m_trackFlowECNs.find (key);
			if (itr == m_trackFlowECNs.end ())	
			{
				// track the size of the available window size the time that the first packet from a new flowlet goes out
				if(flowlet.maxCntLog==0)
				{
					sumAvailWinSize		+=  availableWin/1400;
					cntDecisionsDiffPath++;
				}
				// Initialise
				m_trackFlowECNs[key].second	= -1;
				m_trackFlowECNs[key].first	= flowlet.groupID;
				NS_LOG_DEBUG ("Inserting: FlowID: " << flowId << " ACK: " << ackNum << " New path: " << flowlet.path << " Old path: " << flowlet.old_path << " GroupID: " << flowlet.groupID << " LongFlow: " << flowlet.longFlow << " RemainingData:  " << remainingData << " AvailableWin: " << availableWin << " LogCnt: " << flowlet.maxCntLog);
				// Keep a counter for only the first n=10 packets
				flowlet.maxCntLog++;
			}
		}
	}

	if (Simulator::Now () - flowlet.lastSeen >= m_flowletTimeout)
		NS_LOG_DEBUG ("\tTIMEOUT Flowlet [ " << flowId << " ]  " << Simulator::Now () << " New Path: " << flowlet.path << " GroupID: " << flowlet.groupID << " LongFlow: " << flowlet.longFlow );

	// Update the actvie time
    flowlet.lastSeen = Simulator::Now ();
	// Update the table
    m_flowletMap[flowId] = flowlet;
    return flowlet.path;
}


bool
Ipv4Clove::FindTorId (Ipv4Address daddr, uint32_t &destTorId)
{
    std::map<Ipv4Address, uint32_t>::iterator torItr = m_ipTorMap.find (daddr);

    if (torItr == m_ipTorMap.end ())
    {
        return false;
    }
    destTorId = torItr->second;
    return true;
}

uint32_t
Ipv4Clove::CalPath (uint32_t destTor)
{
	// salvatorg comment
	// Returns all the paths(vector<uint32_t>) to that spesific ToR
    std::map<uint32_t, std::vector<uint32_t> >::iterator itr = m_availablePath.find (destTor);
    if (itr == m_availablePath.end ())
    {
        return 0;
    }
    std::vector<uint32_t> paths = itr->second;
    if (m_runMode == CLOVE_RUNMODE_EDGE_FLOWLET)
    {
		// salvatorg 
		// Check for out of bound index
		uint32_t idx	= 0;
		do{
			idx	= rand() % paths.size ();
		}while(idx>=paths.size ());

        return paths[idx];
    }
    else if (m_runMode == CLOVE_RUNMODE_ECN)
    {
        double r = ((double) rand () / RAND_MAX);
        std::vector<uint32_t>::iterator itr = paths.begin ();
        double weightSum = 0.0;
        for ( ; itr != paths.end (); ++itr)
        {
            uint32_t path = *itr;
            std::pair<uint32_t, uint32_t> key = std::make_pair (destTor, path);
            std::map<std::pair<uint32_t, uint32_t>, double>::iterator pathWeightItr = m_pathWeight.find (key);
            if (pathWeightItr != m_pathWeight.end ())
            {
                weightSum += pathWeightItr->second;
            }
			// salvatorg comment
			// paths.size should be constant(?) for a spesific topo
			// get a random path to the DestToR 
			// NS_LOG_DEBUG ("DEBUG_LOG [CalPath] , weightSum " << weightSum << " (destTor, path) " << destTor << " " << path );
            if (r <= (weightSum / (double) paths.size ()))
            {
                return path;
            }
        }
        return 0;
    }
    else if (m_runMode == CLOVE_RUNMODE_INT)
    {
        return 0;
    }
    return 0;
}

// salvatorg
// Update the weight in the specific path if the received ACK has an ECE bit
// bool withECN var. is set in the tcp-socket-base.cc
void
Ipv4Clove::FlowRecv (uint32_t path, Ipv4Address daddr, bool withECN, uint32_t flowId, int ackNum)
{
	numAckPackets++;
	// salvatorg
	// Traceing v1
	// Avoid handshake (hanshake must not have ECN bits anyway)
	// if( ackNum>0 )
	// {
		std::pair<uint32_t, int> k = std::make_pair (flowId, ackNum);
		// <pair<flowID,ackNum>, pair<GroupID,ECN>>
		std::map<std::pair<uint32_t, int>, std::pair<uint32_t, int> >::iterator Itr = m_trackFlowECNs.find (k);
		if(Itr != m_trackFlowECNs.end ())
		{	//pair<flowID,GroupID>
			std::pair<uint32_t, uint32_t> kk = std::make_pair (flowId, m_trackFlowECNs[k].first);
			// Avoid multiple updates on the same entry
			if(m_trackFlowECNs[k].second == -1)
			{
				NS_LOG_DEBUG ("Updating FlowID: " << flowId << " ackNum: " << ackNum << " GroupID: " << m_trackFlowECNs[k].first << " ECN: " << withECN );
				m_trackFlowECNs[k].second	= withECN ? 1 : 0;

				if(withECN)
					m_trackFlowECNgroupStats[kk].ECN++;
				else 
					m_trackFlowECNgroupStats[kk].noECN++;
				// else
				// 	m_trackFlowECNgroupStats[kk].unknown++;

				// Free some space
				//m_trackFlowECNs.erase(Itr);
			}
		}
		// else
		// 	NS_LOG_DEBUG ("DID NOT FOUND FlowID: " << flowId << " " << ackNum << " " << withECN );
	// }


    if (!withECN)
    {
        return;
    }
	// else
	// 	NS_LOG_DEBUG ("DEBUG_LOG: FlowID " << flowId << " ACK: " << ackNum << " ECN: " << withECN);

    uint32_t destTor = 0;
    if (!Ipv4Clove::FindTorId (daddr, destTor))
    {
        NS_LOG_ERROR ("Cannot find dest tor id based on the given dest address");
        return;
    }

    std::map<uint32_t, std::vector<uint32_t> >::iterator itr = m_availablePath.find (destTor);
    if (itr == m_availablePath.end ())
    {
        return;
    }

    std::vector<uint32_t> paths = itr->second;
    std::pair<uint32_t, uint32_t> key = std::make_pair (destTor, path);

    std::map<std::pair<uint32_t, uint32_t>, Time>::iterator ecnItr = m_pathECNSeen.find (key);

    if (ecnItr == m_pathECNSeen.end ()
            || Simulator::Now () - ecnItr->second >= m_halfRTT)
    {
        // Update the weight
        m_pathECNSeen[key] = Simulator::Now ();

        double originalPathWeight = 1.0;
        std::map<std::pair<uint32_t, uint32_t>, double>::iterator weightItr = m_pathWeight.find (key);
        if (weightItr != m_pathWeight.end ())
        {
            originalPathWeight = weightItr->second;
        }

        m_pathWeight[key] = 0.67 * originalPathWeight;

        std::vector<uint32_t>::iterator pathItr = paths.begin ();

        uint32_t uncongestedPathCount = 0;

        for ( ; pathItr != paths.end (); ++pathItr)
        {
            if (*pathItr != path)
            {
                std::pair<uint32_t, uint32_t> uKey = std::make_pair (destTor, *pathItr);
                std::map<std::pair<uint32_t, uint32_t>, Time>::iterator uEcnItr = m_pathECNSeen.find (uKey);
                if (!m_disToUncongestedPath ||
                        (m_disToUncongestedPath && Simulator::Now () - uEcnItr->second < m_halfRTT))
                {
                    uncongestedPathCount ++;
                }
            }
        }

        if (uncongestedPathCount == 0)
        {
            m_pathWeight[key] = originalPathWeight;
            return;
        }

        pathItr = paths.begin ();
        for ( ; pathItr != paths.end (); ++pathItr)
        {
            if (*pathItr != path)
            {
                std::pair<uint32_t, uint32_t> uKey = std::make_pair (destTor, *pathItr);
                std::map<std::pair<uint32_t, uint32_t>, Time>::iterator uEcnItr = m_pathECNSeen.find (uKey);
                if (!m_disToUncongestedPath ||
                        (m_disToUncongestedPath && Simulator::Now () - uEcnItr->second < m_halfRTT))
                {
                    double uPathWeight = 1.0;
                    std::map<std::pair<uint32_t, uint32_t>, double>::iterator uPathWeightItr = m_pathWeight.find (uKey);
                    if (uPathWeightItr != m_pathWeight.end ())
                    {
                        uPathWeight = uPathWeightItr->second;
                    }
                    m_pathWeight[uKey] = uPathWeight + (0.33 * originalPathWeight) / uncongestedPathCount;
                }
            }
        }
    }
}

// void
// Ipv4Clove::GetDistrStats (void)
// {


// }


void
Ipv4Clove::CreateGroupStats (void)
{
	// <pair<flowID,ackNum>, pair<GroupID,ecnSeen>>
	for(std::map<std::pair<uint32_t, int>, std::pair<uint32_t, int> >::iterator iter1 = m_trackFlowECNs.begin(); iter1 != m_trackFlowECNs.end(); ++iter1)
	{
		// NS_LOG_UNCOND ("FlowID: " << (iter1->first).first << " ackNum: " << (iter1->first).second << " GroupID: "<< (iter1->second).first << " ECN: " << (iter1->second).second);

		// The unknown label means that we have tracked some packets going out with a spesific sequence number 
		// but we never saw ACK back (maybe its because of cumulative ACKs)
		if((iter1->second).second == -1)
			// <pair<flowID,GroupID>, <GroupStats>>
			// i.e m_trackFlowECNgroupStats[flowID,GroupID].unknown++
			m_trackFlowECNgroupStats[std::make_pair ((iter1->first).first, (iter1->second).first)].unknown++;
		else
			// i.e m_trackFlowECNgroupStats[flowID,GroupID].ACKsECNs += vector<pair<ackNum,ecnSeen>>
			(m_trackFlowECNgroupStats[std::make_pair ((iter1->first).first, (iter1->second).first)].ACKsECNs).push_back(std::make_pair ((iter1->first).second,(iter1->second).second));
	}

	for(std::map<std::pair<uint32_t, uint32_t>, GroupStats >::iterator iter2 = m_trackFlowECNgroupStats.begin(); iter2 != m_trackFlowECNgroupStats.end(); ++iter2)
		std::sort ( ((iter2->second).ACKsECNs).begin(), ((iter2->second).ACKsECNs).end() ); 
}


void 
Ipv4Clove::GetStats ()
{

	CreateGroupStats();

	uint32_t good=0;
	uint32_t bad=0;
	uint32_t unknown=0;
	uint32_t demi=0;

	// Distribution vector of 10 first ECNs
	std::vector<uint32_t> ECNdistrVec(10,0);
	// Distribution vector of the 1st occurance of ECN in the first 10 ECNs
	std::vector<uint32_t> firstECNdistrVec(10,0);
	uint32_t numOf10pktsLogged = 0;
	//<pair<flowID,GroupID>, <GroupStats>>
	for(std::map<std::pair<uint32_t, uint32_t>, GroupStats >::iterator iter2 = m_trackFlowECNgroupStats.begin(); iter2 != m_trackFlowECNgroupStats.end(); ++iter2)
	{
		// NS_LOG_UNCOND ("FlowID: " << (iter2->first).first << " GroupID: " << (iter2->first).second << " ECNs: "<< (iter2->second).ECN << " noECNs: " << (iter2->second).noECN << " Unknown: " << (iter2->second).unknown );
		// Sum up everything, all the stas from all the flows
		// If the unknown counter is >2 then dont take into account this flowlet stas
		if((iter2->second).unknown>2)
			unknown++;
		else
		{
			if((iter2->second).ECN>=( ((iter2->second).ECN + (iter2->second).noECN))*0.75 )
				bad++;
			else if((iter2->second).noECN>=( ((iter2->second).ECN + (iter2->second).noECN))*0.75 )
				good++;
			else
				demi++;
		}
		// NS_LOG_UNCOND ( "FlowID: " << (iter2->first).first << " GroupID: " << (iter2->first).second << "\n" << ((iter2->second).ACKsECNs).size() );
		// for(int i = 0; i < ((iter2->second).ACKsECNs).size(); i++ )	NS_LOG_UNCOND ( ((iter2->second).ACKsECNs[i]).first << " " << ((iter2->second).ACKsECNs[i]).second );

		// Skip if we havent logged 10pkts
		if( ((iter2->second).ACKsECNs).size() == 10 )
		{
			for( uint32_t i = 0; i < 10; i++ )	ECNdistrVec[i] += ((iter2->second).ACKsECNs[i]).second;
			for( uint32_t i = 0; i < 10; i++ )	{ if(((iter2->second).ACKsECNs[i]).second == 1) { firstECNdistrVec[i] += 1; break; } } 
			numOf10pktsLogged++;
		}
	}

	// NS_LOG_UNCOND ("### All Flows ###");
	uint32_t o=0;
	uint32_t t=0;
	uint32_t w=0;
    for (std::map<uint32_t, CloveFlowlet>::iterator flowletItr = m_flowletMap.begin(); flowletItr != m_flowletMap.end (); ++flowletItr)
	{	
		// Flows that never ever changed path
		if ((flowletItr->second).neverChangePath == 1)	o++;
		// Total number of flowlet timeouts
		t	+= (flowletItr->second).numTimeouts;
	}
	// Average all the window sizeσ
	w	= (cntDecisionsDiffPath!=0) ? sumAvailWinSize/cntDecisionsDiffPath : 0;
	NS_LOG_DEBUG ("sumAvailWinSize/cntDecisionsDiffPath " << sumAvailWinSize << " " << cntDecisionsDiffPath);

	
	NS_LOG_UNCOND ("TotalNum.Flows: " << m_flowletMap.size() << " Num.FlowsOnePath: " << o << " TotalNum.Decisions: " << t << " AverageAvailableWindowSize: " << w << " numPacketsSent: " << numPacketsSent << " numAckPackets " << numAckPackets);
	NS_LOG_UNCOND ("Num.Decisions: " << m_trackFlowECNgroupStats.size() << " Of10pktsLogged: " << numOf10pktsLogged << " Sum: " << unknown+bad+good+demi << " Good: " << good << " Bad: " << bad << " Demi: " << demi << " Unknown: " << unknown);

	// for( uint32_t i = 0; i < 10; i++ )	NS_LOG_UNCOND (" " << ECNdistrVec[i] );
	NS_LOG_UNCOND ("Distribution of ECNs in the 10pkts logs flowlets");
	NS_LOG_UNCOND (ECNdistrVec[0] << " " << ECNdistrVec[1] << " " << ECNdistrVec[2] << " " << ECNdistrVec[3] << " " << ECNdistrVec[4] << " " << ECNdistrVec[5] << " " << ECNdistrVec[6] << " " << ECNdistrVec[7] << " " << ECNdistrVec[8] << " " << ECNdistrVec[9] );
	NS_LOG_UNCOND ("Distribution of the first occurance of ECN in the 10pkts logs flowlets");
	NS_LOG_UNCOND (firstECNdistrVec[0] << " " << firstECNdistrVec[1] << " " << firstECNdistrVec[2] << " " << firstECNdistrVec[3] << " " << firstECNdistrVec[4] << " " << firstECNdistrVec[5] << " " << firstECNdistrVec[6] << " " << firstECNdistrVec[7] << " " << firstECNdistrVec[8] << " " << firstECNdistrVec[9] );
}

void 
Ipv4Clove::GetStatsLongFlows (void)
{

	CreateGroupStats();

	uint32_t good=0;
	uint32_t bad=0;
	uint32_t unknown=0;
	uint32_t demi=0;
	uint32_t numDecis=0;

	std::vector<uint32_t> ECNdistrVec(10,0);
	std::vector<uint32_t> firstECNdistrVec(10,0);
	uint32_t numOf10pktsLogged = 0;
	//<pair<flowID,GroupID>, <GroupStats>>
	for(std::map<std::pair<uint32_t, uint32_t>, GroupStats >::iterator iter2 = m_trackFlowECNgroupStats.begin(); iter2 != m_trackFlowECNgroupStats.end(); ++iter2)
	{
		// NS_LOG_UNCOND ("FlowID: " << (iter2->first).first << " GroupID: " << (iter2->first).second << " ECNs: "<< (iter2->second).ECN << " noECNs: " << (iter2->second).noECN << " Unknown: " << (iter2->second).unknown );
		// Sum up everything, all the stas from all the flows
		// If the unknown counter is >2 then dont take into account this flowlet stas
		std::map<uint32_t, CloveFlowlet>::iterator flowletItr = m_flowletMap.find ((iter2->first).first);
		// If the flow is in the flow table, get it
		if (flowletItr != m_flowletMap.end ())
		{
			struct CloveFlowlet flowlet = flowletItr->second;
			if(flowlet.longFlow==1)
			{
				if((iter2->second).unknown>2)
					unknown++;
				else
				{
					if((iter2->second).ECN>=( ((iter2->second).ECN + (iter2->second).noECN))*0.75 )
						bad++;
					else if((iter2->second).noECN>=( ((iter2->second).ECN + (iter2->second).noECN))*0.75 )
						good++;
					else
						demi++;
				}
				numDecis++;
				// NS_LOG_UNCOND ( "FlowID: " << (iter2->first).first << " GroupID: " << (iter2->first).second << "\n" << ((iter2->second).ACKsECNs).size() );
				// for(int i = 0; i < ((iter2->second).ACKsECNs).size(); i++ )	NS_LOG_UNCOND ( ((iter2->second).ACKsECNs[i]).first << " " << ((iter2->second).ACKsECNs[i]).second );

				// Skip if we havent logged 10pkts
				if( ((iter2->second).ACKsECNs).size() == 10 )
				{
					for( uint32_t i = 0; i < 10; i++ )	ECNdistrVec[i] += ((iter2->second).ACKsECNs[i]).second;
					for( uint32_t i = 0; i < 10; i++ )	{ if(((iter2->second).ACKsECNs[i]).second == 1) { firstECNdistrVec[i] += 1; break; } } 
					numOf10pktsLogged++;
				}
			}
		}
	}
	// NS_LOG_UNCOND ("### All Flows ###");
	uint32_t o=0;
	uint32_t t=0;
	uint32_t w=0;
	for (std::map<uint32_t, CloveFlowlet>::iterator flowletItr = m_flowletMap.begin(); flowletItr != m_flowletMap.end (); ++flowletItr)
	{	
		if((flowletItr->second).longFlow==1)
		{
			// Flows that never ever changed path
			if ((flowletItr->second).neverChangePath == 1)	o++;
			// Total number of flowlet timeouts
			t	+= (flowletItr->second).numTimeouts;
		}
	}
	// Average all the window sizeσ
	w	= (cntDecisionsDiffPath!=0) ? sumAvailWinSize/cntDecisionsDiffPath : 0;
	NS_LOG_DEBUG ("sumAvailWinSize/cntDecisionsDiffPath " << sumAvailWinSize << " " << cntDecisionsDiffPath);

	
	NS_LOG_UNCOND ("TotalNum.Flows: " << m_flowletMap.size() << " Num.FlowsOnePath: " << o << " TotalNum.Decisions: " << t << " AverageAvailableWindowSize: " << w << " numPacketsSent: " << numPacketsSent << " numAckPackets " << numAckPackets);
	NS_LOG_UNCOND ("Num.Decisions: " << numDecis << " Of10pktsLogged: " << numOf10pktsLogged << " Sum: " << unknown+bad+good+demi << " Good: " << good << " Bad: " << bad << " Demi: " << demi << " Unknown: " << unknown);

	// for( uint32_t i = 0; i < 10; i++ )	NS_LOG_UNCOND (" " << ECNdistrVec[i] );
	NS_LOG_UNCOND ("Distribution of ECNs in the 10pkts logs flowlets");
	NS_LOG_UNCOND (ECNdistrVec[0] << " " << ECNdistrVec[1] << " " << ECNdistrVec[2] << " " << ECNdistrVec[3] << " " << ECNdistrVec[4] << " " << ECNdistrVec[5] << " " << ECNdistrVec[6] << " " << ECNdistrVec[7] << " " << ECNdistrVec[8] << " " << ECNdistrVec[9] );
	NS_LOG_UNCOND ("Distribution of the first occurance of ECN in the 10pkts logs flowlets");
	NS_LOG_UNCOND (firstECNdistrVec[0] << " " << firstECNdistrVec[1] << " " << firstECNdistrVec[2] << " " << firstECNdistrVec[3] << " " << firstECNdistrVec[4] << " " << firstECNdistrVec[5] << " " << firstECNdistrVec[6] << " " << firstECNdistrVec[7] << " " << firstECNdistrVec[8] << " " << firstECNdistrVec[9] );
}


void 
Ipv4Clove::GetStatsShortFlows (void)
{

	CreateGroupStats();

	uint32_t good=0;
	uint32_t bad=0;
	uint32_t unknown=0;
	uint32_t demi=0;
	uint32_t numDecis=0;

	std::vector<uint32_t> ECNdistrVec(10,0);
	std::vector<uint32_t> firstECNdistrVec(10,0);
	uint32_t numOf10pktsLogged = 0;
	//<pair<flowID,GroupID>, <GroupStats>>
	for(std::map<std::pair<uint32_t, uint32_t>, GroupStats >::iterator iter2 = m_trackFlowECNgroupStats.begin(); iter2 != m_trackFlowECNgroupStats.end(); ++iter2)
	{
		// NS_LOG_UNCOND ("FlowID: " << (iter2->first).first << " GroupID: " << (iter2->first).second << " ECNs: "<< (iter2->second).ECN << " noECNs: " << (iter2->second).noECN << " Unknown: " << (iter2->second).unknown );
		// Sum up everything, all the stas from all the flows
		// If the unknown counter is >2 then dont take into account this flowlet stas
		std::map<uint32_t, CloveFlowlet>::iterator flowletItr = m_flowletMap.find ((iter2->first).first);
		// If the flow is in the flow table, get it
		if (flowletItr != m_flowletMap.end ())
		{
			struct CloveFlowlet flowlet = flowletItr->second;
			if(flowlet.shortFlow==1)
			{
				if((iter2->second).unknown>2)
					unknown++;
				else
				{
					if((iter2->second).ECN>=( ((iter2->second).ECN + (iter2->second).noECN))*0.75 )
						bad++;
					else if((iter2->second).noECN>=( ((iter2->second).ECN + (iter2->second).noECN))*0.75 )
						good++;
					else
						demi++;
				}
				numDecis++;
				// NS_LOG_UNCOND ( "FlowID: " << (iter2->first).first << " GroupID: " << (iter2->first).second << "\n" << ((iter2->second).ACKsECNs).size() );
				// for(int i = 0; i < ((iter2->second).ACKsECNs).size(); i++ )	NS_LOG_UNCOND ( ((iter2->second).ACKsECNs[i]).first << " " << ((iter2->second).ACKsECNs[i]).second );

				// Skip if we havent logged 10pkts
				if( ((iter2->second).ACKsECNs).size() == 10 )
				{
					for( uint32_t i = 0; i < 10; i++ )	ECNdistrVec[i] += ((iter2->second).ACKsECNs[i]).second;
					for( uint32_t i = 0; i < 10; i++ )	{ if(((iter2->second).ACKsECNs[i]).second == 1) { firstECNdistrVec[i] += 1; break; } } 
					numOf10pktsLogged++;
				}
			}
		}
	}
	// NS_LOG_UNCOND ("### All Flows ###");
	uint32_t o=0;
	uint32_t t=0;
	uint32_t w=0;
    for (std::map<uint32_t, CloveFlowlet>::iterator flowletItr = m_flowletMap.begin(); flowletItr != m_flowletMap.end (); ++flowletItr)
	{
		if((flowletItr->second).shortFlow==1)
		{
			// Flows that never ever changed path
			if ((flowletItr->second).neverChangePath == 1)	o++;
			// Total number of flowlet timeouts
			t	+= (flowletItr->second).numTimeouts;
		}
	}
	// Average all the window sizeσ
	w	= (cntDecisionsDiffPath!=0) ? sumAvailWinSize/cntDecisionsDiffPath : 0;
	NS_LOG_DEBUG ("sumAvailWinSize/cntDecisionsDiffPath " << sumAvailWinSize << " " << cntDecisionsDiffPath);

	
	NS_LOG_UNCOND ("TotalNum.Flows: " << m_flowletMap.size() << " Num.FlowsOnePath: " << o << " TotalNum.Decisions: " << t << " AverageAvailableWindowSize: " << w << " numPacketsSent: " << numPacketsSent << " numAckPackets " << numAckPackets);
	NS_LOG_UNCOND ("Num.Decisions: " << numDecis << " Of10pktsLogged: " << numOf10pktsLogged << " Sum: " << unknown+bad+good+demi << " Good: " << good << " Bad: " << bad << " Demi: " << demi << " Unknown: " << unknown);

	// for( uint32_t i = 0; i < 10; i++ )	NS_LOG_UNCOND (" " << ECNdistrVec[i] );
	NS_LOG_UNCOND ("Distribution of ECNs in the 10pkts logs flowlets");
	NS_LOG_UNCOND (ECNdistrVec[0] << " " << ECNdistrVec[1] << " " << ECNdistrVec[2] << " " << ECNdistrVec[3] << " " << ECNdistrVec[4] << " " << ECNdistrVec[5] << " " << ECNdistrVec[6] << " " << ECNdistrVec[7] << " " << ECNdistrVec[8] << " " << ECNdistrVec[9] );
	NS_LOG_UNCOND ("Distribution of the first occurance of ECN in the 10pkts logs flowlets");
	NS_LOG_UNCOND (firstECNdistrVec[0] << " " << firstECNdistrVec[1] << " " << firstECNdistrVec[2] << " " << firstECNdistrVec[3] << " " << firstECNdistrVec[4] << " " << firstECNdistrVec[5] << " " << firstECNdistrVec[6] << " " << firstECNdistrVec[7] << " " << firstECNdistrVec[8] << " " << firstECNdistrVec[9] );

}



}
