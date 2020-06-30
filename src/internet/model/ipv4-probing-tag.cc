#include "ipv4-probing-tag.h"

namespace ns3 {

TypeId
Ipv4ProbingTag::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::Ipv4ProbingTag")
		.SetParent<Tag> ()
		.SetGroupName ("Probes")
		.AddConstructor<Ipv4ProbingTag> ();

	return tid;
}

Ipv4ProbingTag::Ipv4ProbingTag ()
{

}

uint16_t
Ipv4ProbingTag::GetId (void) const
{
	return m_id;
}

void
Ipv4ProbingTag::SetId (uint16_t id)
{
	m_id = id;
}

uint16_t
Ipv4ProbingTag::GetPath (void) const
{
	return m_path;
}

void
Ipv4ProbingTag::SetPath (uint16_t path)
{
	m_path = path;
}

Ipv4Address
Ipv4ProbingTag::GetProbeAddress (void) const
{
	return m_probeAddress;
}

void
Ipv4ProbingTag::SetSrcNodeID (uint16_t srcNodeID)
{
	m_srcNodeID = srcNodeID;
}

uint16_t
Ipv4ProbingTag::GetSrcNodeID (void) const
{
	return m_srcNodeID;
}

void
Ipv4ProbingTag::SetProbeAddress (Ipv4Address address)
{
	m_probeAddress = address;
}

uint8_t
Ipv4ProbingTag::GetIsReply (void) const
{
	return m_isReply;
}

void
Ipv4ProbingTag::SetIsReply (uint8_t isReply)
{
	m_isReply = isReply;
}

Time
Ipv4ProbingTag::GetTime (void) const
{
	return m_time;
}

void
Ipv4ProbingTag::SetTime (Time time)
{
	m_time = time;
}

uint8_t
Ipv4ProbingTag::GetIsCE (void) const
{
	return m_isCE;
}

void
Ipv4ProbingTag::SetIsCE (uint8_t ce)
{
	m_isCE = ce;
}

uint8_t
Ipv4ProbingTag::GetIsBroadcast (void) const
{
	return m_isBroadcast;
}

void
Ipv4ProbingTag::SetIsBroadcast (uint8_t isBroadcast)
{
	m_isBroadcast = isBroadcast;
}

TypeId
Ipv4ProbingTag::GetInstanceTypeId (void) const
{
	return GetTypeId ();
}

uint32_t
Ipv4ProbingTag::GetSerializedSize (void) const
{
	return sizeof (uint16_t)
		 + sizeof (uint16_t)
		 + sizeof (uint32_t)
		 + sizeof (uint16_t)
		 + sizeof (uint8_t)
		 + sizeof (double)
		 + sizeof (uint8_t)
		 + sizeof (uint8_t);
}

void
Ipv4ProbingTag::Serialize (TagBuffer i) const
{
	i.WriteU16 (m_id);
	i.WriteU16 (m_path);
	i.WriteU32 (m_probeAddress.Get ());
	i.WriteU16 (m_srcNodeID);
	i.WriteU8 (m_isReply);
	i.WriteDouble (m_time.GetSeconds ());
	i.WriteU8 (m_isCE);
	i.WriteU8 (m_isBroadcast);
}

void
Ipv4ProbingTag::Deserialize (TagBuffer i)
{
	m_id			= i.ReadU16 ();
	m_path			= i.ReadU16 ();
	m_probeAddress	= Ipv4Address (i.ReadU32 ());
	m_srcNodeID		= i.ReadU16 ();
	m_isReply		= i.ReadU8 ();
	m_time			= Time::FromDouble (i.ReadDouble (), Time::S);
	m_isCE			= i.ReadU8 ();
	m_isBroadcast	= i.ReadU8 ();
}

void
Ipv4ProbingTag::Print (std::ostream &os) const
{
	os << "id: " << m_id;
	os << "path: " << m_path;
	os << "probe address: " << m_probeAddress;
	os << "source node id: " << m_srcNodeID;
	os << "Is Reply: " << m_isReply;
	os << "Time: " << m_time;
	os << "Is CE: " << m_isCE;
}

}


