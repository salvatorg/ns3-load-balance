/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef IPV4_PROBING_H
#define IPV4_PROBING_H

#include "ns3/object.h"
#include "ns3/callback.h"
#include "ns3/traced-callback.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"

#include <vector>
#include <map>

namespace ns3 {

class Packet;
class Socket;
class Node;
class Ipv4Header;

class Ipv4Probing : public Object
{
public:

    static TypeId GetTypeId (void);
    virtual TypeId GetInstanceTypeId (void) const;

    Ipv4Probing ();
    Ipv4Probing (const Ipv4Probing&);
    ~Ipv4Probing ();
    virtual void DoDispose (void);

    void SetSourceAddress (Ipv4Address address);
    void SetProbeAddress (Ipv4Address address, uint32_t nodeId);

    void SetNode (Ptr<Node> node);

    void AddBroadCastAddress (Ipv4Address addr);

    void Init (void);

    void SendProbe (uint32_t path, Ipv4Address m_probeAddress);

    void ReceivePacket (Ptr<Socket> socket);

    // void ProbeEventTimeout (uint32_t id, uint32_t path);

    void StartProbe ();

    void StopProbe (Time stopTime);

private:

    void DoProbe ();
    void DoStop ();

    //void BroadcastBestPathTo (Ipv4Address addr);

    void ForwardPathInfoTo (Ipv4Address addr, uint32_t path, Time oneWayRtt, bool isCE);

    // Parameters
    Ipv4Address m_sourceAddress;
    std::vector<Ipv4Address> 	m_probeAddress;	// The flow destination IP
    std::vector<uint32_t> 		m_probeNode;	// The flow destination NodeID

    Time m_probeTimeout;
    Time m_probeInterval;

    uint16_t m_id;

    std::map <uint32_t, EventId> m_probingTimeoutMap;

    /* Best path related */
    bool m_hasBestPath;
    uint32_t m_bestPath;

    Time m_bestPathRtt;
    //bool m_bestPathECN;
    //uint32_t m_bestPathSize;
    /* ----------------- */

    EventId m_probeEvent;

    Ptr<Socket> m_socket;

    std::vector<Ipv4Address> m_broadcastAddresses;

    Ptr<Node> m_node;

};

}

#endif /* CONGESTION_PROBING_H */

