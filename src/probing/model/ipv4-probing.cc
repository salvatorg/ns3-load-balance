/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ipv4-probing.h"

#include "ns3/ipv4-probing-tag.h"

#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/packet.h"
#include "ns3/socket.h"
#include "ns3/ipv4-raw-socket-factory.h"
#include "ns3/ipv4-header.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-clove.h"
#include "ns3/ipv4-xpath-tag.h"

#include <sys/socket.h>
#include <set>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv4Probing");

NS_OBJECT_ENSURE_REGISTERED (Ipv4Probing);

TypeId
Ipv4Probing::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::Ipv4Probing")
		.SetParent<Object> ()
		.SetGroupName ("Probing")
		.AddConstructor<Ipv4Probing> ()
		.AddAttribute ("ProbingInterval", "Probing Interval",
					  TimeValue (MicroSeconds (100)),
					  MakeTimeAccessor (&Ipv4Probing::m_probeInterval),
					  MakeTimeChecker ())
		.AddTraceSource ("Probe1wayDelay",
						"When a probe is received reports the time, id, src, dst, path, delay",
						MakeTraceSourceAccessor (&Ipv4Probing::m_probeDelay),
						"ns3::Ipv4Probing::Probe1wayDelayCallback")
	;

	return tid;
}

TypeId
Ipv4Probing::GetInstanceTypeId () const
{
	return Ipv4Probing::GetTypeId ();
}

Ipv4Probing::Ipv4Probing ()
	: m_sourceAddress (Ipv4Address ("127.0.0.1")),
	  m_probeAddress (std::vector<Ipv4Address> ()),
	  m_probeTimeout (Seconds (0.1)),
	  m_probeInterval (MicroSeconds (100)),
	  m_id (0),
	  m_hasBestPath (false),
	  m_bestPath (0),
	  m_bestPathRtt (Seconds (666)),
	  //m_bestPathECN (false),
	  //m_bestPathSize (0),
	  m_node ()
{
	NS_LOG_FUNCTION (this);
}

Ipv4Probing::Ipv4Probing (const Ipv4Probing &other)
	: m_sourceAddress (other.m_sourceAddress),
	  m_probeAddress (other.m_probeAddress),
	  m_probeTimeout (other.m_probeTimeout),
	  m_probeInterval (other.m_probeInterval),
	  m_id (0),
	  m_hasBestPath (false),
	  m_bestPath (0),
	  m_bestPathRtt (Seconds (666)),
	  //m_bestPathECN (false),
	  //m_bestPathSize (0),
	  m_node ()
{
	NS_LOG_FUNCTION (this);
}

Ipv4Probing::~Ipv4Probing ()
{
	NS_LOG_FUNCTION (this);
}

void
Ipv4Probing::DoDispose ()
{
	std::map <uint32_t, EventId>::iterator itr =
		m_probingTimeoutMap.begin ();
	for ( ; itr != m_probingTimeoutMap.end (); ++itr)
	{
		(itr->second).Cancel ();
		m_probingTimeoutMap.erase (itr);
	}
}

void
Ipv4Probing::SetSourceAddress (Ipv4Address address)
{
	m_sourceAddress = address;
}

void
Ipv4Probing::SetProbeAddress (Ipv4Address address, uint32_t nodeId)
{
	// m_probeAddress = address;
	m_probeAddress.push_back(address);
	m_probeNode.push_back(nodeId);
}

void
Ipv4Probing::SetNode (Ptr<Node> node)
{
	m_node = node;
}

// void
// Ipv4Probing::SetId (uint32_t id)
// {
// 	m_id = static_cast<uint16_t>(id);
// }

void
Ipv4Probing::AddBroadCastAddress (Ipv4Address addr)
{
	m_broadcastAddresses.push_back (addr);
}

void
Ipv4Probing::Init ()
{
	m_socket = m_node->GetObject<Ipv4RawSocketFactory> ()->CreateSocket ();
	m_socket->SetRecvCallback (MakeCallback (&Ipv4Probing::ReceivePacket, this));
	m_socket->Bind (InetSocketAddress (Ipv4Address ("0.0.0.0"), 0));
	m_socket->SetAttribute ("IpHeaderInclude", BooleanValue (true));
}

void
Ipv4Probing::SendProbe (uint32_t path, Ipv4Address m_probeAddress)
{
	// Create the probe identifier (SrcNodeID+Path+DstNodeID)
	// m_id		= static_cast<uint16_t>(m_probeNode);
	// NS_LOG_DEBUG ("m_id: " << m_id << " , " << m_node->GetId() << " , " << path  << " , " << m_probeNode);
	Address to 	= InetSocketAddress (m_probeAddress, 0);

	Ptr<Packet> packet = Create<Packet> (0);
	Ipv4Header newHeader;
	newHeader.SetSource (m_sourceAddress);
	newHeader.SetDestination (m_probeAddress);
	newHeader.SetProtocol (0);
	newHeader.SetPayloadSize (packet->GetSize ());
	newHeader.SetEcn (Ipv4Header::ECN_ECT1);
	newHeader.SetTtl (255);
	packet->AddHeader (newHeader);

	// XPath tag
	Ipv4XPathTag ipv4XPathTag;
	ipv4XPathTag.SetPathId (path);
	packet->AddPacketTag (ipv4XPathTag);

	// Probing tag
	Ipv4ProbingTag probingTag;
	probingTag.SetId (m_id);
	probingTag.SetPath (path);
	probingTag.SetProbeAddress (m_probeAddress);
	probingTag.SetSrcNodeID (m_node->GetId());
	probingTag.SetIsReply (0);
	probingTag.SetTime (Simulator::Now ());
	probingTag.SetIsCE (0);
	probingTag.SetIsBroadcast (0);
	packet->AddPacketTag (probingTag);

	m_socket->SendTo (packet, 0, to);
	
	// m_id ++;
	// Add timeout
	// m_probingTimeoutMap[m_id] = Simulator::Schedule (m_probeTimeout, &Ipv4Probing::ProbeEventTimeout, this, m_id, path);

	// Ptr<Ipv4Clove> ipv4Clove = m_node->GetObject<Ipv4Clove> ();
	// ipv4Clove->ProbeSend (m_probeAddress, path);
}

// void
// Ipv4Probing::ProbeEventTimeout (uint32_t id, uint32_t path)
// {
//     m_probingTimeoutMap.erase (id);
//     Ptr<Ipv4Clove> ipv4Clove = m_node->GetObject<Ipv4Clove> ();
//     ipv4Clove->ProbeTimeout (path, m_probeAddress);
// }

void
Ipv4Probing::ReceivePacket (Ptr<Socket> socket)
{
	Ptr<Packet> packet = m_socket->Recv (std::numeric_limits<uint32_t>::max (), MSG_PEEK);

	Ipv4ProbingTag probingTag;
	bool found = packet->RemovePacketTag (probingTag);

	if (!found)
	{
		return;
	}

	Ipv4Header ipv4Header;
	found = packet->RemoveHeader (ipv4Header);
	// NS_LOG_FUNCTION (this);
	// NS_LOG_LOGIC ("Time["<<Simulator::Now()<<"] Ipv4 SRC Header: " << ipv4Header.GetSource () << " Ipv4 DST Header: " << ipv4Header.GetDestination () <<" End-to-end delay: " << Simulator::Now() - probingTag.GetTime ());
	// NS_LOG_INFO ("Time["<<Simulator::Now()<<"] DstNodeID[" << m_node->GetId() << "," << ipv4Header.GetDestination () <<"] SrcNodeID[" << probingTag.GetId () << "," << ipv4Header.GetSource () << "] Path: " << probingTag.GetPath () << "] Latency: " << Simulator::Now() - probingTag.GetTime ());
	NS_LOG_INFO ("Time["<<Simulator::Now()<< "] ProbeID[" << probingTag.GetId() << "] SrcNodeID[" << probingTag.GetSrcNodeID () << "] DstNodeID[" << m_node->GetId() << "] Path[" << probingTag.GetPath () << "] Latency: " << Simulator::Now() - probingTag.GetTime ());

	// m_probeDelay(Simulator::Now ().GetSeconds (), probingTag.GetId(), probingTag.GetSrcNodeID (),m_node->GetId(), probingTag.GetPath (), Simulator::Now() - probingTag.GetTime ());
	// m_probeDelay(Simulator::Now ().GetSeconds (), probingTag.GetId(), probingTag.GetSrcNodeID ());
	
	NS_LOG_UNCOND ((probingTag.GetTime ()).GetSeconds () << " " << probingTag.GetId() << " " << probingTag.GetSrcNodeID () << " " << m_node->GetId() << " " << probingTag.GetPath () << " " << (Simulator::Now() - probingTag.GetTime ()).GetNanoSeconds () );
	// if (probingTag.GetIsReply () == 0)
	// {
	//     // Reply to this packet
	//     Ptr<Packet> newPacket = Create<Packet> (0);
	//     Ipv4Header newHeader;
	//     newHeader.SetSource (m_sourceAddress);
	//     newHeader.SetDestination (ipv4Header.GetSource ());
	//     newHeader.SetProtocol (0);
	//     newHeader.SetPayloadSize (packet->GetSize ());
	//     newHeader.SetTtl (255);
	//     newPacket->AddHeader (newHeader);

	//     Ipv4ProbingTag replyProbingTag;
	//     replyProbingTag.SetId (probingTag.GetId ());
	//     replyProbingTag.SetPath (probingTag.GetPath ());
	//     replyProbingTag.SetProbeAddress (probingTag.GetProbeAddres ());
	//     replyProbingTag.SetIsReply (1);
	//     replyProbingTag.SetIsBroadcast (0);
	//     replyProbingTag.SetTime (Simulator::Now() - probingTag.GetTime ());
	//     if (ipv4Header.GetEcn () == Ipv4Header::ECN_CE)
	//     {
	//         replyProbingTag.SetIsCE (1);
	//     }
	//     else
	//     {
	//         replyProbingTag.SetIsCE (0);
	//     }
	//     newPacket->AddPacketTag (replyProbingTag);

	//     Address to = InetSocketAddress (ipv4Header.GetSource (), 0);

	//     m_socket->SendTo (newPacket, 0, to);
	// }
	// else
	// {
	//     std::map<uint32_t, EventId>::iterator itr =
	//             m_probingTimeoutMap.find (probingTag.GetId ());

	//     if (itr == m_probingTimeoutMap.end () )//&& !probingTag.GetIsBroadcast ())
	//     {
	//         // The reply has incurred timeout
	//         return;
	//     }

	//     // if (!probingTag.GetIsBroadcast ())
	//     // {
	//     //     // Cancel the probing timeout timer
	//     //     (itr->second).Cancel ();
	//     //     m_probingTimeoutMap.erase (itr);
	//     // }

	//     uint32_t path = probingTag.GetPath ();
	//     Time oneWayRtt = probingTag.GetTime ();
	//     bool isCE = probingTag.GetIsCE () == 1 ? true : false;
	//     uint32_t size = packet->GetSize () + ipv4Header.GetSerializedSize ();

	//     if (oneWayRtt < m_bestPathRtt)
	//     {
	//         m_hasBestPath = true;
	//         //m_bestPath = path;
	//         //m_bestPathRtt = oneWayRtt;
	//         //m_bestPathECN = isCE;
	//         //m_bestPathSize = size;
	//     }

	//     // Ptr<Ipv4TLB> ipv4TLB = m_node->GetObject<Ipv4TLB> ();
	//     // ipv4TLB->ProbeRecv (path, probingTag.GetProbeAddres (), size, isCE, oneWayRtt);

	//     // if (!probingTag.GetIsBroadcast ())
	//     // {
	//     //     // Forward path information to servers under the same rack
	//     //     std::vector<Ipv4Address>::iterator broadcastItr = m_broadcastAddresses.begin ();
	//     //     for ( ; broadcastItr != m_broadcastAddresses.end (); broadcastItr ++)
	//     //     {
	//     //         Ipv4TLBProbing::ForwardPathInfoTo(*broadcastItr, path, oneWayRtt, isCE);
	//     //     }
	//     // }
	// }
}

void
Ipv4Probing::StartProbe ()
{
	m_probeEvent = Simulator::ScheduleNow (&Ipv4Probing::DoProbe, this);
}

void
Ipv4Probing::StopProbe (Time stopTime)
{
	Simulator::Schedule (stopTime, &Ipv4Probing::DoStop, this);
}

void
Ipv4Probing::DoProbe ()
{
	uint32_t probingCount = 3;
	std::set<uint32_t> pathSet;

	// if (m_hasBestPath && m_bestPath != 0)
	// {
	//     /*
	//     std::vector<Ipv4Address>::iterator itr = m_broadcastAddresses.begin ();
	//     for ( ; itr != m_broadcastAddresses.end (); itr ++)
	//     {
	//         Ipv4Probing::BroadcastBestPathTo (*itr);
	//     }
	//     */

	//     Ipv4Probing::SendProbe (m_bestPath);
	//     probingCount --;
	//     pathSet.insert (m_bestPath);
	// }
	Ptr<Ipv4Clove> ipv4Clove = m_node->GetObject<Ipv4Clove> ();

	for (uint32_t j = 0; j < m_probeAddress.size(); j++)
	{
		std::vector<uint32_t> availPaths = ipv4Clove->GetAvailPath (m_probeAddress[j]);
		if (!availPaths.empty ())
		{
			for (uint32_t i = 0; i < availPaths.size(); i++) 
			{
				// uint32_t path = availPaths[rand() % availPaths.size ()];
				uint32_t path = availPaths[i];
				// if (pathSet.find (path) != pathSet.end ())
				// {
				// 	continue;
				// }
				// probingCount --;
				// if (probingCount == 0)
				// {
				//     break;
				// }
				// pathSet.insert (path);
				Ipv4Probing::SendProbe (path, m_probeAddress[j]);
				NS_LOG_LOGIC ("Time["<<Simulator::Now()<<"] SrcNodeID[" << m_node->GetId() <<"] with Path[" << path <<"] probes DstNodeID[" << m_probeNode[j] << "] " );
			}
		}
	}
	m_hasBestPath = false;
	m_bestPathRtt = Seconds (666);

	m_id++;
	m_probeEvent = Simulator::Schedule (m_probeInterval, &Ipv4Probing::DoProbe, this);
}

void
Ipv4Probing::DoStop ()
{
	m_probeEvent.Cancel ();
}

/*
void
Ipv4Probing::BroadcastBestPathTo (Ipv4Address addr)
{
	Address to = InetSocketAddress (addr, 0);

	Ptr<Packet> packet = Create<Packet> (0);
	Ipv4Header newHeader;
	newHeader.SetSource (m_sourceAddress);
	newHeader.SetDestination (addr);
	newHeader.SetProtocol (0);
	newHeader.SetPayloadSize (packet->GetSize ());
	newHeader.SetEcn (Ipv4Header::ECN_ECT1);
	newHeader.SetTtl (255);
	packet->AddHeader (newHeader);

	// Probing tag
	Ipv4ProbingTag probingTag;
	probingTag.SetId (0);
	probingTag.SetPath (m_bestPath);
	probingTag.SetProbeAddress (m_probeAddress);
	probingTag.SetIsReply (1);
	probingTag.SetTime (m_bestPathRtt);
	probingTag.SetIsCE (m_bestPathECN);
	probingTag.SetIsBroadcast (1);
	packet->AddPacketTag (probingTag);

	m_socket->SendTo (packet, 0, to);
}
*/

// void
// Ipv4Probing::ForwardPathInfoTo (Ipv4Address addr, uint32_t path, Time oneWayRtt, bool isCE)
// {
//     Address to = InetSocketAddress (addr, 0);

//     Ptr<Packet> packet = Create<Packet> (0);
//     Ipv4Header newHeader;
//     newHeader.SetSource (m_sourceAddress);
//     newHeader.SetDestination (addr);
//     newHeader.SetProtocol (0);
//     newHeader.SetPayloadSize (packet->GetSize ());
//     newHeader.SetEcn (Ipv4Header::ECN_ECT1);
//     newHeader.SetTtl (255);
//     packet->AddHeader (newHeader);

//     // Probing tag
//     Ipv4ProbingTag probingTag;
//     probingTag.SetId (0);
//     probingTag.SetPath (path);
//     probingTag.SetProbeAddress (m_probeAddress);
//     probingTag.SetIsReply (1);
//     probingTag.SetTime (oneWayRtt);
//     probingTag.SetIsCE (isCE);
//     probingTag.SetIsBroadcast (1);
//     packet->AddPacketTag (probingTag);

//     m_socket->SendTo (packet, 0, to);

// }

}

