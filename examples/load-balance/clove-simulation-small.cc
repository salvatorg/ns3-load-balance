#include <vector>
#include <map>
#include <utility>
#include <set>
#include <string>
#include <sstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-conga-routing-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-drb-routing-helper.h"
#include "ns3/ipv4-xpath-routing-helper.h"
#include "ns3/ipv4-tlb.h"
#include "ns3/ipv4-clove.h"
#include "ns3/ipv4-tlb-probing.h"
#include "ns3/ipv4-probing.h"
#include "ns3/link-monitor-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/tcp-resequence-buffer.h"
#include "ns3/ipv4-drill-routing-helper.h"
#include "ns3/ipv4-letflow-routing-helper.h"

#include "ns3/gnuplot.h"


// #include <array>
// The CDF in TrafficGenerator
extern "C"
{
#include "cdf.h"
}

#define TG_GOODPUT_RATIO	(1448.0 / (1500 + 14 + 4 + 8 + 12))
#define LINK_CAPACITY_BASE	1000000000				// 1Gbps

// #define BUFFER_SIZE 600							// 840KB
// #define BUFFER_SIZE 500							// 700KB
// #define BUFFER_SIZE 250							// 350KB
#define BUFFER_SIZE		100								// 140KB


// a marking threshold as low as 20 packets can be used for 10Gbps, we found that a more conservative marking threshold larger than 60 packets is required to avoid loss of throughput . DCTCP paper
// we use the marking thresholds of K = 20 packets for 1Gbps and K = 65 packets for 10Gbps ports
// #define RED_QUEUE_MARKING 65						// 10Gbps ,Clove-ECN -- 91KB
#define RED_QUEUE_MARKING	15						// 1Gbps ,Clove-ECN, Letflow-FTO50-150 -- 28KB
// #define RED_QUEUE_MARKING 71						// 10Gbps(?), Letflow-FTO50-150, 100KB

// The flow port range, each flow will be assigned a random port number within this range
#define PORT_START	10000
#define PORT_END	50000

// Adopted from the simulation from WANG PENG
// Acknowledged to https://williamcityu@bitbucket.org/williamcityu/2016-socc-simulation.git
#define PACKET_SIZE	1400

#define PRESTO_RATIO	10

using namespace std;
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CloveSimulationSmall");

enum RunMode {
	ECMP,
	Clove
};


uint32_t checkTimes;
double avgQueueSize;
uint32_t CloveFalsePositives=0;
uint32_t CloveFalseNegatives=0;
uint32_t CloveTruePositives=0;
uint32_t CloveAll=0;
uint64_t LetflowFalsePositives=0;
uint64_t LetflowPositives=0;
uint64_t LetflowEmptyPkts=0;

QueueDiscContainer queueDiscs;
QueueDiscContainer QueueDiscLeavesToServers;
QueueDiscContainer QueueDiscServersToLeaves;


vector< Ptr<QueueDisc> > myvector (8, ns3::Ptr< QueueDisc >()); 
vector< vector < Ptr<QueueDisc> > > SpineQueueDiscs (8 , myvector);


// vector < ns3::GnuplotDataset::Data::Data > queuediscDatasetIntf (8,ns3::GnuplotDataset::Data()); 
Gnuplot2dDataset queuediscDataset[8][8] ; 


std::stringstream filePlotQSizes;
std::stringstream filePlotQueue;
std::stringstream filePlotQueueAvg;


NodeContainer spines;
// spines.Create (SPINE_COUNT);
NodeContainer leaves;
// leaves.Create (LEAF_COUNT);
NodeContainer servers;
// servers.Create (SERVER_COUNT * LEAF_COUNT);


// salvatorg
string enum_to_string(RunMode type) {
   switch(type) {
	//   case TLB:
	//      return "TLB";
	//   case CONGA:
	//      return "CONGA";
	//   case CONGA_FLOW:
	//      return "CONGA_FLOW";
	//   case CONGA_ECMP:
	//      return "CONGA_ECMP";
	//   case PRESTO:
	//      return "PRESTO";
	//   case WEIGHTED_PRESTO:
	//      return "WEIGHTED_PRESTO";
	//   case DRB:
	//      return "DRB";
	//   case FlowBender:
	//      return "FlowBender";
	  case ECMP:
		 return "ECMP";
	  case Clove:
		 return "Clove";
	//   case DRILL:
	//      return "DRILL";
	//   case LetFlow:
	//      return "LetFlow";
	  default:
		 return "INVALID_RUN_MODE";
   }
}

// salvatorg
void GetNumECNs(void)
{
	NS_LOG_UNCOND (" ==== Leaves<->Spines Queues ("<<queueDiscs.GetN()<<") ====");
	uint32_t o=0;
	uint64_t tot=0;
	uint32_t i;
	// for (QueueDiscContainer::ConstIterator i = queueDiscs.Begin (); i != queueDiscs.End (); ++i)

	for (i=0; i<queueDiscs.GetN(); i++)
	{
		// RedQueueDisc::Stats stat	= StaticCast<RedQueueDisc> (*i)->GetStats();
		RedQueueDisc::Stats stat = StaticCast<RedQueueDisc> (queueDiscs.Get (i))->GetStats ();
		// RedQueueDisc::Stats statSL = StaticCast<RedQueueDisc> (queueDiscs.Get (i+1))->GetStats ();

		// TODO, make it modular
		// This is Leaf -> Spine Queue
		NS_LOG_UNCOND ("L"<< i/4 << "S" << " Num.ECNsMarked: " << stat.unforcedMarking + stat.forcedMarking << " Num.Drops: " << stat.forcedDrop);
		// NS_LOG_UNCOND ("S"<<  << "L" << i/4 << " Num.ECNsMarked: " << stat.unforcedMarking + stat.forcedMarking << " Num.Drops: " << stat.forcedDrop);

		o++;
		tot += stat.unforcedMarking + stat.forcedMarking;
	}
	NS_LOG_UNCOND ("Total: " << tot);
	tot=0;

	NS_LOG_UNCOND (" ==== Leaves->Hosts Queues ("<<QueueDiscLeavesToServers.GetN()<<") ====");
	// for (QueueDiscContainer::ConstIterator ii = QueueDiscLeavesToServers.Begin (); ii != QueueDiscLeavesToServers.End (); ++ii)
	// {
	// 	RedQueueDisc::Stats stat	= StaticCast<RedQueueDisc> (*ii)->GetStats();
	// for (QueueDiscContainer::ConstIterator i = queueDiscs.Begin (); i != queueDiscs.End (); ++i)
	for (i=0; i<QueueDiscLeavesToServers.GetN(); i++)
	{
		// RedQueueDisc::Stats stat	= StaticCast<RedQueueDisc> (*i)->GetStats();
		RedQueueDisc::Stats stat = StaticCast<RedQueueDisc> (QueueDiscLeavesToServers.Get (i))->GetStats ();
		NS_LOG_UNCOND ("Num.ECNsMarked: " << stat.unforcedMarking + stat.forcedMarking << " Num.Drops: " << stat.forcedDrop);
		tot += stat.unforcedMarking + stat.forcedMarking;
	}
	NS_LOG_UNCOND ("Total: " << tot);
	tot=0;

	NS_LOG_UNCOND (" ==== Hosts->Leaves Queues ("<<QueueDiscServersToLeaves.GetN()<<") ====");
	for (i=0; i<QueueDiscServersToLeaves.GetN(); i++)
	{
		RedQueueDisc::Stats stat = StaticCast<RedQueueDisc> (QueueDiscServersToLeaves.Get (i))->GetStats ();
		NS_LOG_UNCOND ("Num.ECNsMarked: " << stat.unforcedMarking + stat.forcedMarking << " Num.Drops: " << stat.forcedDrop);
		tot += stat.unforcedMarking + stat.forcedMarking;
	}
	NS_LOG_UNCOND ("Total: " << tot);

}


// salvatorg
std::string GetFormatedStr (uint spine_id, std::string str, std::string str2)
{
	std::stringstream ss;
	ss << "Queues_Track_Spine_" << spine_id << "_" << str << str2;
	return ss.str ();
}


// salvatorg
void DoGnuPlotPdf (uint spine_id, double end_time)
{
	GnuplotCollection gnuplots (GetFormatedStr (spine_id, "" , ".pdf").c_str ());

	std::ostringstream ecn_mark_line;
	ecn_mark_line << "set arrow from 0,"<< RED_QUEUE_MARKING*PACKET_SIZE << " to "<< end_time << "," << RED_QUEUE_MARKING*PACKET_SIZE << " nohead front";

	std::ostringstream y_range_max;
	y_range_max << "set yrange [0:+" << PACKET_SIZE*BUFFER_SIZE << "]";

	for (int i=0; i<8; i++)
	{
		std::ostringstream os;
		os << " Tx Queues Sizes -- Intf. " << i;
		Gnuplot queuediscGnuplot;
		
		queuediscGnuplot.SetTitle (os.str());
		os.clear();


		queuediscGnuplot.AppendExtra (y_range_max.str ());
		queuediscGnuplot.AppendExtra ("set grid y");
		queuediscGnuplot.AppendExtra (ecn_mark_line.str ());
		queuediscGnuplot.SetLegend ("Time (s)","Bytes");

		os << "Intf. " << i;
		queuediscDataset[spine_id][i].SetTitle (os.str ());
		queuediscDataset[spine_id][i].SetStyle (Gnuplot2dDataset::IMPULSES);
		queuediscGnuplot.AddDataset (queuediscDataset[spine_id][i]);
		os.clear();

		// std::ofstream queuediscGnuplotFile (GetFormatedStr (spine_id, ".0-7", ".plt").c_str ());

		// queuediscGnuplot.GenerateOutput (queuediscGnuplotFile);

		// queuediscGnuplotFile.close ();

		gnuplots.AddPlot (queuediscGnuplot);
	}

	std::ofstream queuediscGnuplotFile (GetFormatedStr (spine_id, "pdf", ".plt").c_str ());
	gnuplots.GenerateOutput (queuediscGnuplotFile);
	queuediscGnuplotFile.close ();
}



// salvatorg
void DoGnuPlot (uint spine_id, double end_time)
{
	Gnuplot queuediscGnuplot (GetFormatedStr (spine_id, ".0-7" , ".jpeg").c_str ());

	std::ostringstream ecn_mark_line;
	ecn_mark_line << "set arrow from 0,"<< RED_QUEUE_MARKING*PACKET_SIZE << " to "<< end_time << "," << RED_QUEUE_MARKING*PACKET_SIZE << " nohead front";

	std::ostringstream y_range_max;
	y_range_max << "set yrange [0:+" << PACKET_SIZE*BUFFER_SIZE << "]";

	queuediscGnuplot.SetTitle ("Tx Queues Sizes");
	queuediscGnuplot.SetTerminal ("jpeg");
	queuediscGnuplot.AppendExtra (y_range_max. str());
	queuediscGnuplot.AppendExtra ("set grid y");
	queuediscGnuplot.AppendExtra (ecn_mark_line.str ());

	queuediscGnuplot.SetLegend ("Time (s)","Bytes");

	for (int i=0; i<8; i++)
	{
		std::ostringstream os;
		os << "Intf. " << i;
		queuediscDataset[spine_id][i].SetTitle (os.str ());
		queuediscGnuplot.AddDataset (queuediscDataset[spine_id][i]);
		os.clear();
	}
	std::ofstream queuediscGnuplotFile (GetFormatedStr (spine_id, ".0-7", ".plt").c_str ());

	queuediscGnuplot.GenerateOutput (queuediscGnuplotFile);

	queuediscGnuplotFile.close ();
}

void CheckQueueDiscSize (Ptr<QueueDisc> queue, uint spine, uint intf)
{
	uint32_t qSize = queue->GetNBytes ();
	// if (qSize==0) NS_LOG_UNCOND("QueueDisc " << spine << " " << intf );
	// queuediscDataset[spine][intf].Add (Simulator::Now ().GetSeconds (), qSize);
	queuediscDataset[spine][intf].Add (Simulator::Now ().GetSeconds (), qSize);
	Simulator::Schedule (Seconds (0.00001), &CheckQueueDiscSize, queue, spine, intf); // 10us
}


// void CheckQueueSize (uint32_t )
// {
//     uint32_t qSize = queueDiscs.Get(uplink_idx)->GetNBytes();;
// 	// if (qSize==0) NS_LOG_UNCOND("QueueDisc " << spine << " " << intf );
//     // queuediscDataset[spine][intf].Add (Simulator::Now ().GetSeconds (), qSize);
// 	// queuediscDataset[spine][intf].Add (Simulator::Now ().GetSeconds (), qSize);
// 	NS_LOG_UNCOND("QueueDisc " << spine << " " << intf );
//     Simulator::Schedule (Seconds (0.00001), &CheckQueueDiscSize, queue, spine, intf); // 10us
// }

void CloveQueuesTrace ()
{
	 std::ostringstream oss;
	 std::string fPlotQueue = " ";



//   fPlotQueue << "CheckQueueSize: " << Simulator::Now ().GetSeconds () << "\t";
  for (QueueDiscContainer::ConstIterator i = queueDiscs.Begin (); i != queueDiscs.End (); ++i)
  {
	//(*i)->method ();  // some QueueDisc method
	// Modidified by salvatorg
	// Returns the number of bytes, QUEUE_MODE_BYTES
	uint qSize = StaticCast<RedQueueDisc> (*i)->GetQueueSize (); //  <QueueDisc> (*i)->GetNBytes()
	oss << qSize << " ";
	// std::string ss = std::to_string(qSize);
	//NS_LOG_UNCOND("Size ("<< << (*i) <<"): " << qSize);
	// fPlotQueue += oss.str();
	// fPlotQueue += " ";
	//oss.clear();
	// if (i == queueDiscs.End () )
	// 	fPlotQueue += qSize + std::endl;
	// else
	// 	fPlotQueue += qSize + "\t";
  }	
	NS_LOG_UNCOND("CheckQueueSize at time: " << Simulator::Now ().GetSeconds () << " " << oss.str());

}


// salvatorg
void ClovePathTrace (uint32_t flowId, uint32_t fromTor, uint32_t destTor, uint32_t newPath, uint32_t oldPath, Ipv4Address saddr, Ipv4Address daddr, Ptr<Node> nodeID)
{

	uint32_t dst_tor = ( ( newPath - (uint32_t)17 ) / (uint32_t)100 ) - (uint32_t)1;
	uint32_t spine = ( newPath - (uint32_t)17 ) % (uint32_t)100;	 

	if(fromTor>7 || destTor>7 || dst_tor!=destTor || spine>7)
	{
		NS_ABORT_MSG ("\n\n\n\n\n\nSomething is fucked up with the ToR numbers\n\n\n\n\n\n\n\n");
		// exit 0;
	}
	uint32_t uplink_idx = (fromTor*16)+(spine*2);
	uint32_t downlink_idx = (destTor*16)+(spine*2)+1;

	// Note: RedQueueDisc->GetQueueSize is 1 pkt behind QueueDisc->GetNBytes (? bug ?)
	// RedQueueDisc->GetQueueSize depricated in rel. 3.30
	uint32_t qSizeUplink = queueDiscs.Get(uplink_idx)->GetNBytes();
	// double qSizeUplink2 = queueDiscs.Get(uplink_idx)->GetCurrentSize ().GetValue (); // in rel. 3.30
	// uint32_t qSizeRedUplink = (StaticCast<RedQueueDisc> (queueDiscs.Get(uplink_idx)))->GetQueueSize ();
	uint32_t qSizeDownlink = queueDiscs.Get(downlink_idx)->GetNBytes();
	// double qSizeDownlink2 = queueDiscs.Get(downlink_idx)->GetCurrentSize ().GetValue (); // in rel. 3.30
	// uint32_t qSizeRedDownlink = (StaticCast<RedQueueDisc> (queueDiscs.Get(downlink_idx)))->GetQueueSize ();


	//  Avoid setup procedure
	if(oldPath != 0)
	{
		if( (qSizeUplink>RED_QUEUE_MARKING*PACKET_SIZE || qSizeDownlink>RED_QUEUE_MARKING*PACKET_SIZE) )
		{
			if( newPath != oldPath )
				CloveFalsePositives++;
			else
				CloveFalseNegatives++;
			// NS_LOG_INFO ("Flow: " << flowId << " (" << fromTor << " -> " << destTor << ") selects path: " << newPath << "(" << oldPath << ") at: " << Simulator::Now ().GetSeconds () << " Last seen: " << lastseen);
			// NS_LOG_INFO ("CheckQueueSize at time: " << Simulator::Now ().GetSeconds () << " " << qSizeUplink << " " << qSizeDownlink );
			// NS_LOG_INFO (" False Positives: " <<  CloveFalsePositives << " Positives: " << ClovePositives );
		}
		else
			CloveTruePositives++;

		// Count the number of flowlet decisions
		CloveAll++;
	}
	// Ptr<Ipv4L3Protocol> ipv4 = nodeID->GetObject<Ipv4L3Protocol> ();
	// Ptr<TrafficControlLayer> tc = leaves.Get(fromTor)->GetDevice(17+spine)->GetNode ()->GetObject<TrafficControlLayer> ();
	// Ptr<QueueDisc> q = tc->GetRootQueueDiscOnDevice (leaves.Get(fromTor)->GetDevice(17+spine));
	// NS_LOG_INFO ("Leaf "<< fromTor <<" Num.NetDevices: " <<leaves.Get(fromTor)->GetNDevices());
	// NS_LOG_INFO ("Q.Sizes: "<<qSizeUplink<<" , "<< q->GetNBytes() << " " << Simulator::Now ().GetSeconds ());

	Ptr<Ipv4L3Protocol> ipv4 = nodeID->GetObject<Ipv4L3Protocol> ();
	Ptr<TrafficControlLayer> tc = spines.Get(spine)->GetDevice(destTor+1)->GetNode ()->GetObject<TrafficControlLayer> ();
	Ptr<QueueDisc> q = tc->GetRootQueueDiscOnDevice (spines.Get(spine)->GetDevice(destTor+1));
	// NS_LOG_INFO ("Spine "<< spine <<" Num.NetDevices: " << spines.Get(spine)->GetNDevices());
	// NS_LOG_INFO ("Spine: "<< spine << " Q.Sizes: "<<qSizeDownlink<<" , "<< q->GetNBytes() << "\t" << Simulator::Now ().GetSeconds ());


	// NS_LOG_INFO ("Flow: " << flowId << " ToRs (" << fromTor << "->" << spine <<"->" << destTor << ") selects path: " << newPath << " SRC-IP: " << saddr << " DST-IP: " << daddr << "   " << "Node ID: " << nodeID->GetId () << " Num.NetDevices: " << nodeID->GetNDevices() );

	// NS_LOG_INFO (CloveAll << " : " << CloveFalsePositives << " " << CloveFalseNegatives << " " << CloveTruePositives << " " << CloveFalsePositives+CloveFalseNegatives+CloveTruePositives);
}



// RAND_MAX : This value is library-dependent, but is guaranteed to be at least 32767 on any standard library implementation.

// Port from Traffic Generator
// Acknowledged to https://github.com/HKUST-SING/TrafficGenerator/blob/master/src/common/common.c
double poission_gen_interval(double avg_rate)
{
	if (avg_rate > 0)
	   return -logf(1.0 - (double)rand() / RAND_MAX) / avg_rate;
	else
	   return 0;
}

/* generate poission process arrival interval */
double poission_gen_interval_ORIG(double avg_rate)
{
	if (avg_rate > 0)
		return -logf(1.0 - (double)(rand() % RAND_MAX) / RAND_MAX) / avg_rate;
	else
		return 0;
}

template<typename T>
T rand_range (T min, T max)
{
	return min + ((double)max - min) * rand () / RAND_MAX;
}

void install_applications (int fromLeafId, NodeContainer servers, double requestRate, struct cdf_table *cdfTable,
        long &flowCount, long &totalFlowSize, int SERVER_COUNT, int LEAF_COUNT, double START_TIME, double END_TIME, double FLOW_LAUNCH_END_TIME, uint32_t applicationPauseThresh, uint32_t applicationPauseTime)
{
    NS_LOG_INFO ("Install applications under POD: "<< fromLeafId << ", (requestRate):" << requestRate);
	uint32_t longFlowsNum=0;
	uint32_t shortFlowsNum=0;
	uint32_t totalFlowsNum=0;
	uint64_t totalBytes=0;
    for (int i = 0; i < SERVER_COUNT; i++)//SERVER_COUNT
    {
        int fromServerIndex = fromLeafId * SERVER_COUNT + i;
		double tmp;
        double startTime = START_TIME + poission_gen_interval (requestRate);
        while (startTime < FLOW_LAUNCH_END_TIME)
        {
			totalFlowsNum ++;
            flowCount ++;
            uint16_t port = rand_range (PORT_START, PORT_END);

            int destServerIndex = fromServerIndex;
	        while (destServerIndex >= fromLeafId * SERVER_COUNT && destServerIndex < fromLeafId * SERVER_COUNT + SERVER_COUNT)
            {
		        destServerIndex = rand_range (0, SERVER_COUNT * LEAF_COUNT);
            }

	        Ptr<Node> destServer = servers.Get (destServerIndex);
	        Ptr<Ipv4> ipv4 = destServer->GetObject<Ipv4> ();
	        Ipv4InterfaceAddress destInterface = ipv4->GetAddress (1,0);
	        Ipv4Address destAddress = destInterface.GetLocal ();

            BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (destAddress, port));
            uint32_t flowSize = gen_random_cdf (cdfTable);

			// salvatorg
			// Keep a counter on the long/short flows (long>=10MB)
			if(flowSize>=10000000)
				longFlowsNum++;
			if(flowSize<=100000)
				shortFlowsNum++;

			totalBytes += flowSize;
            totalFlowSize += flowSize;
 	        source.SetAttribute ("SendSize", UintegerValue (PACKET_SIZE));
            source.SetAttribute ("MaxBytes", UintegerValue(flowSize));//flowSize
            // source.SetAttribute ("DelayThresh", UintegerValue (applicationPauseThresh));
            // source.SetAttribute ("DelayTime", TimeValue (MicroSeconds (applicationPauseTime)));

            // Install apps
            ApplicationContainer sourceApp = source.Install (servers.Get (fromServerIndex));
            sourceApp.Start (Seconds (startTime));
            sourceApp.Stop (Seconds (END_TIME));

            // Install packet sinks
            PacketSinkHelper sink ("ns3::TcpSocketFactory",
                    InetSocketAddress (Ipv4Address::GetAny (), port));
            ApplicationContainer sinkApp = sink.Install (servers. Get (destServerIndex));
            sinkApp.Start (Seconds (START_TIME));
            sinkApp.Stop (Seconds (END_TIME));

            // NS_LOG_INFO ("\tFlow from server: " << fromServerIndex << " to server: "
            //         << destServerIndex << " on port: " << port << " with flow size: "
            //         << flowSize << " [start time: " << startTime <<"]");
            // if(flowCount==10)break;
			tmp = poission_gen_interval (requestRate);
            startTime += tmp;
			// NS_LOG_INFO (tmp);
        }
    }
	NS_LOG_INFO ("\tTotal Bytes to be sent\t: " << totalBytes*1e-9 << " GBytes " << " ( " << (totalBytes*8*1e-9) << " Gbits )");
	NS_LOG_INFO ("\tNumber of long flows\t: " << longFlowsNum);
	NS_LOG_INFO ("\tNumber of short flows\t: " << shortFlowsNum);
	NS_LOG_INFO ("\tNumber of flows from the pod\t: " << totalFlowsNum);
}

void install_single_application (int fromServer, int toServer, NodeContainer servers, uint32_t flowSize, double START_TIME, double END_TIME)
{
		NS_LOG_INFO ("Install applications:");


		uint16_t port = rand_range (PORT_START, PORT_END);

		Ptr<Node> destServer = servers.Get (toServer);
		Ptr<Ipv4> ipv4 = destServer->GetObject<Ipv4> ();
		Ipv4InterfaceAddress destInterface = ipv4->GetAddress (1,0);
		Ipv4Address destAddress = destInterface.GetLocal ();

		BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (destAddress, port));

		source.SetAttribute ("SendSize", UintegerValue (PACKET_SIZE));
		source.SetAttribute ("MaxBytes", UintegerValue(flowSize));
		// source.SetAttribute ("DelayThresh", UintegerValue (applicationPauseThresh));
		// source.SetAttribute ("DelayTime", TimeValue (MicroSeconds (applicationPauseTime)));

		// Install apps
		ApplicationContainer sourceApp = source.Install (servers.Get (fromServer));
		sourceApp.Start (Seconds (START_TIME));
		sourceApp.Stop (Seconds (END_TIME));

		// Install packet sinks
		PacketSinkHelper sink ("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), port));
		ApplicationContainer sinkApp = sink.Install (servers. Get (toServer));
		sinkApp.Start (Seconds (START_TIME));
		sinkApp.Stop (Seconds (END_TIME));

		NS_LOG_INFO ("\tFlow from server: " << fromServer << " to server: "
				<< toServer << " on port: " << port << " with flow size: "
				<< flowSize << " [start time: " << START_TIME <<"]");

}

int main (int argc, char *argv[])
{
#if 1
	LogComponentEnable ("CloveSimulationSmall", LOG_LEVEL_INFO);
#endif

	// Default Command line parameters
	std::string id 				= "0";
	std::string runModeStr		= "Clove";
	unsigned randomSeed			= 0;
	std::string cdfFileName		= "";
	double load					= 0.0;
	std::string transportProt	= "DcTcp";

	// The simulation starting and ending time
	double START_TIME	= 0.1;
	double END_TIME		= 1;

	double FLOW_LAUNCH_END_TIME	= 0.1;

	uint32_t linkLatency		= 15;	// micro seconds

	bool asymCapacity			= false;
	uint32_t asymCapacityPoss	= 20;	// 40 %



	int SERVER_COUNT	= 2;	// Number of servers in each pod
	int SPINE_COUNT		= 2;	// Number of Spines
	int LEAF_COUNT		= 2;	// Nember of Leaves
	int LINK_COUNT		= 1;	// Number of links between each leaf-spine node

	uint64_t spineLeafCapacity	= 10;	// Multiplied with the LINK_CAPACITY_BASE (bps)
	uint64_t leafServerCapacity	= 10;	// Multiplied with the LINK_CAPACITY_BASE (bps)

	uint32_t applicationPauseThresh	= 0;
	uint32_t applicationPauseTime	= 1000;

	// Clove parameters
	uint32_t cloveFlowletTimeout	= 240;	// micro seconds
	uint32_t cloveRunMode			= 1;	// 0. Clove-edge, 1. Clove-ECN, 2. Clove-INT(not impemented)
	uint32_t cloveHalfRTT			= 60;	// micro seconds
	bool cloveDisToUncongestedPath	= false;

	bool ProbingEnable			= false;
	uint32_t ProbingInterval	= 100;	// micro seconds

	CommandLine cmd;
	cmd.AddValue ("ID", "Running ID", id);
	cmd.AddValue ("StartTime", "Start time of the simulation", START_TIME);
	cmd.AddValue ("EndTime", "End time of the simulation", END_TIME);
	cmd.AddValue ("FlowLaunchEndTime", "End time of the flow launch period", FLOW_LAUNCH_END_TIME);
	cmd.AddValue ("runMode", "Running mode of this simulation: Conga, Conga-flow, Presto, Weighted-Presto, DRB, FlowBender, ECMP, Clove, DRILL, LetFlow", runModeStr);
	cmd.AddValue ("randomSeed", "Random seed, 0 for random generated", randomSeed);
	cmd.AddValue ("cdfFileName", "File name for flow distribution", cdfFileName);
	cmd.AddValue ("load", "Load of the network, 0.0 - 1.0", load);
	cmd.AddValue ("transportProt", "Transport protocol to use: Tcp, DcTcp", transportProt);
	cmd.AddValue ("linkLatency", "Link latency, should be in MicroSeconds", linkLatency);

	cmd.AddValue ("asymCapacity", "Whether the capacity is asym, which means some link will have only 1/10 the capacity of others", asymCapacity);
	cmd.AddValue ("asymCapacityPoss", "The possibility that a path will have only 1/10 capacity", asymCapacityPoss);

	cmd.AddValue ("serverCount", "The Server count", SERVER_COUNT);
	cmd.AddValue ("spineCount", "The Spine count", SPINE_COUNT);
	cmd.AddValue ("leafCount", "The Leaf count", LEAF_COUNT);
	cmd.AddValue ("linkCount", "The Link count", LINK_COUNT);

	cmd.AddValue ("spineLeafCapacity", "Spine <-> Leaf capacity in Gbps", spineLeafCapacity);
	cmd.AddValue ("leafServerCapacity", "Leaf <-> Server capacity in Gbps", leafServerCapacity);

	cmd.AddValue ("applicationPauseThresh", "How many packets can pass before we have delay, 0 for disable", applicationPauseThresh);
	cmd.AddValue ("applicationPauseTime", "The time for a delay, in MicroSeconds", applicationPauseTime);

	cmd.AddValue ("cloveFlowletTimeout", "Flowlet timeout for Clove", cloveFlowletTimeout);
	cmd.AddValue ("cloveRunMode", "Clove run mode, 0 for edge flowlet, 1 for ECN, 2 for INT (not yet implemented)", cloveRunMode);
	cmd.AddValue ("cloveHalfRTT", "Half RTT used in Clove ECN", cloveHalfRTT);
	cmd.AddValue ("cloveDisToUncongestedPath", "Whether Clove will distribute the weight to uncongested path (no ECN) or all paths", cloveDisToUncongestedPath);

    cmd.AddValue ("ProbingEnable", "ProbingEnable", ProbingEnable);
    cmd.AddValue ("ProbingInterval", "ProbingInterval", ProbingInterval);

	cmd.Parse (argc, argv);

	uint64_t SPINE_LEAF_CAPACITY = spineLeafCapacity * LINK_CAPACITY_BASE;
	uint64_t LEAF_SERVER_CAPACITY = leafServerCapacity * LINK_CAPACITY_BASE;
	Time LINK_LATENCY = MicroSeconds (linkLatency);

	RunMode runMode;
	if (runModeStr.compare ("ECMP") == 0)
	{
		runMode = ECMP;
	}
	else if (runModeStr.compare ("Clove") == 0)
	{
		runMode = Clove;
	}
	else
	{
		NS_LOG_ERROR ("The running mode should be Clove or ECMP");
		return 0;
	}

	if (load < 0.0 || load >= 1.0)
	{
		NS_LOG_ERROR ("The network load should within 0.0 and 1.0");
		return 0;
	}

	if (transportProt.compare ("DcTcp") == 0)
	{
		NS_LOG_INFO ("Enabling DcTcp");
		Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpDCTCP::GetTypeId ()));
		Config::SetDefault ("ns3::RedQueueDisc::Mode", StringValue ("QUEUE_MODE_BYTES")); // Determines unit for QueueLimit
		Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (PACKET_SIZE));
		Config::SetDefault ("ns3::RedQueueDisc::QueueLimit", UintegerValue (BUFFER_SIZE * PACKET_SIZE));
		//Config::SetDefault ("ns3::QueueDisc::Quota", UintegerValue (BUFFER_SIZE));
		Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (false));
		Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (RED_QUEUE_MARKING * PACKET_SIZE));
		Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (RED_QUEUE_MARKING * PACKET_SIZE));
	}

	if (runMode == Clove)
	{
		NS_LOG_INFO ("Enabling Clove");
		Config::SetDefault ("ns3::TcpSocketBase::Clove", BooleanValue (true));
		Config::SetDefault ("ns3::Ipv4Clove::FlowletTimeout", TimeValue (MicroSeconds (cloveFlowletTimeout)));
		Config::SetDefault ("ns3::Ipv4Clove::RunMode", UintegerValue (cloveRunMode));
		Config::SetDefault ("ns3::Ipv4Clove::HalfRTT", TimeValue (MicroSeconds (cloveHalfRTT)));
		Config::SetDefault ("ns3::Ipv4Clove::DisToUncongestedPath", BooleanValue (cloveDisToUncongestedPath));
	}

	if (ProbingEnable)
	{
		Config::SetDefault("ns3::Ipv4Probing::ProbingInterval", TimeValue (MicroSeconds (ProbingInterval)));
	}

	NS_LOG_INFO ("Config parameters");
	Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue(PACKET_SIZE));
	Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (0));
	Config::SetDefault ("ns3::TcpSocket::ConnTimeout", TimeValue (MilliSeconds (5)));
	Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (10));
	Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (MilliSeconds (5)));
	Config::SetDefault ("ns3::TcpSocketBase::ClockGranularity", TimeValue (MicroSeconds (100)));
	Config::SetDefault ("ns3::RttEstimator::InitialEstimation", TimeValue (MicroSeconds (80)));
	Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (160000000));
	Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (160000000));


	// NodeContainer spines;
	spines.Create (SPINE_COUNT);
	// NodeContainer leaves;
	leaves.Create (LEAF_COUNT);
	// NodeContainer servers;
	servers.Create (SERVER_COUNT * LEAF_COUNT);

	std::vector<Ptr<Ipv4Probing> > probings (SERVER_COUNT * LEAF_COUNT);


	NS_LOG_INFO ("Install Internet stacks");
	InternetStackHelper		internet;

	Ipv4StaticRoutingHelper	staticRoutingHelper;
	Ipv4GlobalRoutingHelper	globalRoutingHelper;
	Ipv4ListRoutingHelper	listRoutingHelper;
	Ipv4XPathRoutingHelper	xpathRoutingHelper;


	if (runMode == Clove)
	{
		// Enable the Clove stack in the InternetStackHelper for the servers
		internet.SetClove (true);
		// Aggregate IP/TCP/UDP functionality to existing Nodes and Clove functionality. (ns3::Ipv4StaticRouting[0], ns3::Ipv4GlobalRouting[-10])
		internet.Install (servers);
		// Disable the Clove stack for the InternetStackHelper
		internet.SetClove (false);

		// Install ns3::Ipv4GlobalRouting[0] and ns3::Ipv4XPathRoutingHelper for spines nad leaves
		// Note: the routing protocols are each consulted in decreasing order of priority to see whether a match is found
		listRoutingHelper.Add (xpathRoutingHelper, 1);	// Add to list the XPathRoutingProtocol with priority 1. 
		// listRoutingHelper.Add (staticRoutingHelper, 1);
		listRoutingHelper.Add (globalRoutingHelper, 0);	// Add to list the IPv4Routing with priority 0
		internet.SetRoutingHelper (listRoutingHelper);
		Config::SetDefault ("ns3::Ipv4GlobalRouting::PerflowEcmpRouting", BooleanValue(true)); // Set to the IPv4Routing to PerflowEcmpRouting

		internet.Install (spines);
		internet.Install (leaves);
	}
	else if (runMode == ECMP)
	{

		internet.SetRoutingHelper (globalRoutingHelper);
		Config::SetDefault ("ns3::Ipv4GlobalRouting::PerflowEcmpRouting", BooleanValue(true));

		internet.Install (servers);
		internet.Install (spines);
		internet.Install (leaves);
	}
	else
	{
		NS_LOG_ERROR ("Could not find the Internet stacks to use for the routing");
		return 0;
	}





	NS_LOG_INFO ("Install channels and assign addresses");
	PointToPointHelper p2p;
	Ipv4AddressHelper ipv4;
	TrafficControlHelper tc;

	// salvatorg. Moved above to the default configuration
	if (transportProt.compare ("DcTcp") == 0)
	{
		tc.SetRootQueueDisc ("ns3::RedQueueDisc");
	// 	// , "MinTh", DoubleValue (RED_QUEUE_MARKING * PACKET_SIZE),
	// 	// "MaxTh", DoubleValue (RED_QUEUE_MARKING * PACKET_SIZE));
	}

	NS_LOG_INFO ("Configuring servers (Servers:" << servers.GetN() << ")");
	// Setting servers
	p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (LEAF_SERVER_CAPACITY)));
	p2p.SetChannelAttribute ("Delay", TimeValue(LINK_LATENCY));
	p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (BUFFER_SIZE));
	// salvatorg. Lets make it default, as above for the TCP and DCTCP
	// if (transportProt.compare ("Tcp") == 0)
	// {
	//  	p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (BUFFER_SIZE));
	// }
	// salvatorg. Was commented out because dunno if it ovewrites the TrafficControlHelper 
	// which should set the DropTailQueue when initiliazed
	// else
	// {
	//     p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (10));
	// }

	ipv4.SetBase ("10.1.0.0", "255.255.255.0");

	std::vector<Ipv4Address> leafNetworks (LEAF_COUNT);

	std::vector<Ipv4Address> serverAddresses (SERVER_COUNT * LEAF_COUNT);

	std::map<std::pair<int, int>, uint32_t> leafToSpinePath;
	std::map<std::pair<int, int>, uint32_t> spineToLeafPath;

	for (int i = 0; i < LEAF_COUNT; i++)
	{
		Ipv4Address network = ipv4.NewNetwork ();
		leafNetworks[i] = network;

		for (int j = 0; j < SERVER_COUNT; j++)
		{
			int serverIndex = i * SERVER_COUNT + j;
			NodeContainer nodeContainer = NodeContainer (leaves.Get (i), servers.Get (serverIndex));
			NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);

			if (transportProt.compare ("DcTcp") == 0)
			{
				// NS_LOG_INFO ("Install RED Queue for leaf: " << i << " and server: " << j);
				QueueDiscContainer queueDisc =  tc.Install (netDeviceContainer);
				// salvatorg
				// Store the TxQueueDisc of every leaf to server 
				QueueDiscLeavesToServers.Add(queueDisc.Get(0));
				// Store the TxQueueDisc of every server to leaf 
				QueueDiscServersToLeaves.Add(queueDisc.Get(1));
			}

			// Assign IP addresses to both end points
			Ipv4InterfaceContainer interfaceContainer = ipv4.Assign (netDeviceContainer);

			NS_LOG_INFO ("Leaf - " << i << " (ID:" << (leaves.Get(i))->GetId() << ") is connected to Server - " << j << " (ID:" << (servers.Get(serverIndex))->GetId() << ") with address " << interfaceContainer.GetAddress(0) << " <-> " << interfaceContainer.GetAddress (1) << " with port " << netDeviceContainer.Get (0)->GetIfIndex () << " <-> " << netDeviceContainer.Get (1)->GetIfIndex ());

			// Store the each server address
			serverAddresses [serverIndex] = interfaceContainer.GetAddress (1);
			if (transportProt.compare ("Tcp") == 0)
			{
				tc.Uninstall (netDeviceContainer);
			}

			if (runMode == Clove)
			{
				for (int k = 0; k < SERVER_COUNT * LEAF_COUNT; k++)
				{
					Ptr<Ipv4Clove> clove = servers.Get (k)->GetObject<Ipv4Clove> ();
					clove->AddAddressWithTor (interfaceContainer.GetAddress (1), i);
				}
			}
		}
	}

	// for (int i=0; i<128; i++){
	// 		NS_LOG_INFO ("NodeContainer servers TypeId: " << servers.Get(i)->GetId());
	// 		NS_LOG_INFO ("NodeContainer servers GetApplication: " << servers.Get(i)->GetApplication(0)->GetTypeId());
	// }

	NS_LOG_INFO ("Configuring switches (Leaves:" << leaves.GetN() << ", Spines:" << spines.GetN() << ")");
	// Setting up switches
	p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (SPINE_LEAF_CAPACITY)));
	std::set<std::pair<uint32_t, uint32_t> > asymLink; // set< (A, B) > Leaf A -> Spine B is asymmetric


	for (int i = 0; i < LEAF_COUNT; i++)
	{
		for (int j = 0; j < SPINE_COUNT; j++)
		{
			for (int l = 0; l < LINK_COUNT; l++)
			{
				bool isAsymCapacity = false;

				if (asymCapacity && static_cast<uint32_t> (rand () % 100) < asymCapacityPoss)
				{
					isAsymCapacity = true;
				}

				// TODO
				uint64_t spineLeafCapacity = SPINE_LEAF_CAPACITY;

				if (isAsymCapacity)
				{
					spineLeafCapacity = SPINE_LEAF_CAPACITY / 5;
					asymLink.insert (std::make_pair (i, j));
					asymLink.insert (std::make_pair (j, i));
				}

				p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (spineLeafCapacity)));
				ipv4.NewNetwork ();

				NodeContainer nodeContainer = NodeContainer (leaves.Get (i), spines.Get (j));
				NS_LOG_INFO ("Leaf: " << i << " (ID:" << leaves.Get (i)->GetId () << "),"  << " Spine: " << j << "(ID:" << spines.Get (j)->GetId () << ")");

				NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);
				if (transportProt.compare ("DcTcp") == 0)
				{
					// salvatorg
					// create a list with all the leaf-spine queues
					QueueDiscContainer queueDisc = tc.Install (netDeviceContainer);
					SpineQueueDiscs[j][i] = queueDisc.Get(1);
					queueDiscs.Add(queueDisc);
				}
				Ipv4InterfaceContainer ipv4InterfaceContainer = ipv4.Assign (netDeviceContainer);
				// NS_LOG_INFO ("Leaf - " << i << " is connected to Spine - " << j << " with address "
				// 		<< ipv4InterfaceContainer.GetAddress(0) << " <-> " << ipv4InterfaceContainer.GetAddress (1)
				// 		<< " with port " << netDeviceContainer.Get (0)->GetIfIndex () << " <-> " << netDeviceContainer.Get (1)->GetIfIndex ()
				// 		<< " with data rate " << spineLeafCapacity);
				// salvatorg
				if (isAsymCapacity)
				{
					NS_LOG_INFO ("\tReducing Link Capacity of Leaf: " << i << " with port: " << netDeviceContainer.Get (0)->GetIfIndex ());
					NS_LOG_INFO ("\tReducing Link Capacity of Spine: " << j << " with port: " << netDeviceContainer.Get (1)->GetIfIndex ());
				}


				if (runMode == Clove)
				{
					std::pair<int, int> leafToSpine = std::make_pair<int, int> (i, j);
					leafToSpinePath[leafToSpine] = netDeviceContainer.Get (0)->GetIfIndex ();

					std::pair<int, int> spineToLeaf = std::make_pair<int, int> (j, i);
					spineToLeafPath[spineToLeaf] = netDeviceContainer.Get (1)->GetIfIndex ();
				}

				if (transportProt.compare ("Tcp") == 0)
				{
					tc.Uninstall (netDeviceContainer);
				}

			}
		}
	}

	// ATTEMPT to set up multicast address for the probes
    //  Ptr<Node> multicastRouter = leaves.Get(1);  // The node in question
	//  NS_LOG_UNCOND(" >>>>>>>>> LEAF:" << multicastRouter->GetId() << " " << multicastRouter->GetNDevices());
	//  // The first ifIndex is 0 for loopback, then the first p2p is numbered 1
    //  uint32_t inputIf = (multicastRouter->GetDevice (3))->GetIfIndex ();  // The input NetDevice
    //  std::vector< uint32_t > outputDevices;  // A container of output NetDevices
    //  outputDevices.push_back((multicastRouter->GetDevice (1))->GetIfIndex ()); 
   	//  outputDevices.push_back((multicastRouter->GetDevice (2))->GetIfIndex ());

	// staticRoutingHelper.GetStaticRouting (multicastRouter->GetObject<Ipv4> ())-> AddMulticastRoute (Ipv4Address("10.1.1.2"), Ipv4Address("225.1.2.4"), inputIf, outputDevices);
    // //  staticRoutingHelper.AddMulticastRoute (multicastRouter, Ipv4Address("10.1.1.2"), Ipv4Address("225.1.2.4"), inputIf, outputDevices);
	// inputIf = (multicastRouter->GetDevice (4))->GetIfIndex ();  // The input NetDevice
	// staticRoutingHelper.GetStaticRouting (multicastRouter->GetObject<Ipv4> ())-> AddMulticastRoute (Ipv4Address("10.1.1.2"), Ipv4Address("225.1.2.4"), inputIf, outputDevices);

    //  // 2) Set up a default multicast route on the sender n0 
    //  Ptr<Node> sender = servers.Get (0);
    //  uint32_t senderIf = (sender->GetDevice(1))->GetIfIndex ();
    //  staticRoutingHelper.GetStaticRouting (multicastRouter->GetObject<Ipv4> ())->SetDefaultMulticastRoute (senderIf);




	NS_LOG_INFO ("Installed RED Queues, Tot.Number : " << queueDiscs.GetN() << " queues");

	if (runMode == ECMP || runMode == Clove)
	{
		NS_LOG_INFO ("Populate global routing tables");
		Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	}

	if (runMode == Clove)
	{
		NS_LOG_INFO ("Configuring Clove available paths");
		for (int i = 0; i < LEAF_COUNT; i++)
		{
			for (int j = 0; j < SERVER_COUNT; j++)
			{
				int serverIndex = i * SERVER_COUNT + j;
				for (int k = 0; k < SPINE_COUNT; k++)
				{
					int path = 0;
					int pathBase = 1;
					path += leafToSpinePath[std::make_pair (i, k)] * pathBase;
					pathBase *= 100;
					for (int l = 0; l < LEAF_COUNT; l++)
					{
						if (i == l)
						{
							continue;
						}
						int newPath = spineToLeafPath[std::make_pair (k, l)] * pathBase + path;
						Ptr<Ipv4Clove> clove = servers.Get (serverIndex)->GetObject<Ipv4Clove> ();
						clove->AddAvailPath (l, newPath);
						// NS_LOG_INFO ("Configuring server: " << serverIndex << " to leaf: " << l << " with path: " << newPath);
					}
				}
			}
		}

		// Manualy set up of forwarding table
		// Ptr<Ipv4Clove> clove;
		// clove = servers.Get (0)->GetObject<Ipv4Clove> ();
		// clove->AddAvailPath (1, 203);
		// clove = servers.Get (1)->GetObject<Ipv4Clove> ();
		// clove->AddAvailPath (1, 203);
		// clove = servers.Get (3)->GetObject<Ipv4Clove> ();
		// clove->AddAvailPath (1, 202);



		if (ProbingEnable)
		{
		NS_LOG_INFO ("Configuring Probing");
			// Install tha handlers for the probes in each server, Recv()/Send() probe functions
			// We just want to be sure to install the Recv() function on all the servers
			for (int i = 0; i < SERVER_COUNT * LEAF_COUNT; i++)
			{
				Ptr<Ipv4Probing> probing = CreateObject<Ipv4Probing> ();
				probings[i] = probing;
				probing->SetNode (servers.Get (i));
				probing->SetSourceAddress (serverAddresses[i]);
				probing->Init ();
			}

			// The i th server under one leaf is used to probe each leaf by contacting the i th server under that leaf
			// The ipv4-probing will return all the path to a particular leaf under which the i th server lives
			// e.g. server 0 under leaf 0 probes server 0 under leaves 1-LEAF_COUNT
			for (int probingServer = 0; probingServer < SERVER_COUNT * LEAF_COUNT; probingServer=SERVER_COUNT+probingServer)
			{
				for (int serverBeingProbed = 0; serverBeingProbed < SERVER_COUNT * LEAF_COUNT ; serverBeingProbed++)
				{
						int LeafBeingProbed	= serverBeingProbed / SERVER_COUNT;
						int probingLeaf 	= probingServer / SERVER_COUNT;
						// int serverBeingProbed = SERVER_COUNT * serverIndexUnderLeaf;

						if (LeafBeingProbed == probingLeaf)
						{
							continue;
						}

						probings[probingServer]->SetProbeAddress (serverAddresses[serverBeingProbed], (servers.Get (serverBeingProbed))->GetId()); 
						
						NS_LOG_INFO ("Server ["<< serverAddresses[probingServer] <<"] (ID:" << (servers.Get (probingServer))->GetId() << ") is going to probe Server [" << serverAddresses[serverBeingProbed] << "] (ID:" << (servers.Get (serverBeingProbed))->GetId() << ")");
						// int leafIndex = i / SERVER_COUNT;
						// for (int j = leafIndex * SERVER_COUNT; j < leafIndex * SERVER_COUNT + SERVER_COUNT; j++)
						// {
						// 	if (i == j)
						// 	{
						// 		continue;
						// 	}
						// 	probing->AddBroadCastAddress (serverAddresses[j]);
						// 	//NS_LOG_INFO ("Server:" << i << " is going to broadcast to server: " << j);
						// }
				}
				probings[probingServer]->StartProbe ();
				probings[probingServer]->StopProbe (Seconds (END_TIME));
			}
		}
	}

	// The over-subscription ratio is the ratio between the upstream bandwidth and the downstream capacity
	double oversubRatio = static_cast<double>(SERVER_COUNT * LEAF_SERVER_CAPACITY) / (SPINE_LEAF_CAPACITY * SPINE_COUNT * LINK_COUNT);
	NS_LOG_INFO ("Over-subscription ratio (capacity server/capacity spine): " << oversubRatio<<":1 ");

	struct cdf_table* cdfTable = new cdf_table ();
	double requestRate	= 0;
	if(cdfFileName.compare ("") != 0)
	{
		NS_LOG_INFO ("Initialize CDF Table");
		// cdf_table* cdfTable = new cdf_table ();
		init_cdf (cdfTable);
		load_cdf (cdfTable, cdfFileName.c_str ());
		NS_LOG_INFO ("Average request size: " <<  avg_cdf(cdfTable)  << " bytes\n");
		NS_LOG_INFO ("CDF Table");
		// print_cdf (cdfTable);

		// uint32_t period_us = avg_cdf(cdfTable) * 8 / load / TG_GOODPUT_RATIO; /* average request arrival interval (in microseconds) */
		// uint32_t req_sleep_us = poission_gen_interval_ORIG(1.0/period_us); /* sleep interval based on poission process */

		NS_LOG_INFO ("Calculating request rate");
		requestRate = load * LEAF_SERVER_CAPACITY * SERVER_COUNT / oversubRatio / (8 * avg_cdf (cdfTable)) / SERVER_COUNT ;

		NS_LOG_INFO ("RequestRate (Server): " << requestRate << " per second from every server");
		NS_LOG_INFO ("RequestRate (POD): " << requestRate*SERVER_COUNT << " per second from every POD");

	}
	else
	{
		NS_LOG_ERROR ("Failed to find path to the CDF file");
		return 0;
	}

	NS_LOG_INFO ("Initialize random seed: " << randomSeed);
	if (randomSeed == 0)
		srand ((unsigned)time (NULL));
	else
		srand (randomSeed);


	NS_LOG_INFO ("Create applications");

	long flowCount = 0;
	long totalFlowSize = 0;

	// for (int fromLeafId = 0; fromLeafId < LEAF_COUNT; fromLeafId ++)//LEAF_COUNT
	// {
	//      install_applications(fromLeafId, servers, requestRate, cdfTable, flowCount, totalFlowSize, SERVER_COUNT, LEAF_COUNT, START_TIME, END_TIME, FLOW_LAUNCH_END_TIME, applicationPauseThresh, applicationPauseTime);
	// }

	// Manually add and launch applications
	install_single_application(0, 2, servers, 10000, START_TIME, END_TIME);
	install_single_application(1, 2, servers, 50000000, START_TIME, END_TIME);
	// install_single_application(3, 2, servers, 50000000, START_TIME, END_TIME);

	// install_single_application(0, 3, servers, 50000000, START_TIME+0.2, END_TIME);
	// install_single_application(1, 3, servers, 50000000, START_TIME+0.2, END_TIME);

	NS_LOG_INFO ("Total Number of flows\t: " << flowCount);
	NS_LOG_INFO ("Sum of flow sizes\t: " << totalFlowSize << " Bytes");
	NS_LOG_INFO ("Average flow size\t: " << static_cast<double> (totalFlowSize) / flowCount);

	NS_LOG_INFO ("Enabling flow monitor");
	Ptr<FlowMonitor> flowMonitor;
	FlowMonitorHelper flowHelper;
	flowMonitor = flowHelper.InstallAll();

	NS_LOG_INFO ("Enabling link monitor");
	Ptr<LinkMonitor> linkMonitor = Create<LinkMonitor> ();
	for (int i = 0; i < SPINE_COUNT; i++)
	{
	  std::stringstream name;
	  name << "Spine " << i;
	  Ptr<Ipv4LinkProbe> spineLinkProbe = Create<Ipv4LinkProbe> (spines.Get (i), linkMonitor);
	  spineLinkProbe->SetProbeName (name.str ());
	  spineLinkProbe->SetCheckTime (Seconds (0.01));
	  spineLinkProbe->SetDataRateAll (DataRate (SPINE_LEAF_CAPACITY));
	}
	for (int i = 0; i < LEAF_COUNT; i++)
	{
	  std::stringstream name;
	  name << "Leaf " << i;
	  Ptr<Ipv4LinkProbe> leafLinkProbe = Create<Ipv4LinkProbe> (leaves.Get (i), linkMonitor);
	  leafLinkProbe->SetProbeName (name.str ());
	  leafLinkProbe->SetCheckTime (Seconds (0.01));
	  leafLinkProbe->SetDataRateAll (DataRate (SPINE_LEAF_CAPACITY));
	}

	linkMonitor->Start (Seconds (START_TIME));
	linkMonitor->Stop (Seconds (END_TIME));

	flowMonitor->CheckForLostPackets ();

	std::stringstream flowMonitorFilename;
	std::stringstream linkMonitorFilename;

	flowMonitorFilename << id << "-" << LEAF_COUNT << "x" << SPINE_COUNT << "-L" << load*100 << "-"  << transportProt << "-" << enum_to_string(runMode)<< cloveRunMode << "-";
	linkMonitorFilename << id << "-" << LEAF_COUNT << "x" << SPINE_COUNT << "-L" << load*100 << "-"  << transportProt << "-" << enum_to_string(runMode)<< cloveRunMode << "-";


	// if (runMode == ECMP)
	// {
	// 	flowMonitorFilename << "ecmp-simulation-";
	// 	linkMonitorFilename << "ecmp-simulation-";
	// }
	if (runMode == Clove)
	{
		// flowMonitorFilename << "clove-" << cloveRunMode << "-FTMO" << cloveFlowletTimeout << "-HRTT" << cloveHalfRTT << "-" << cloveDisToUncongestedPath << "-";
		// linkMonitorFilename << "clove-" << cloveRunMode << "-FTMO" << cloveFlowletTimeout << "-HRTT" << cloveHalfRTT << "-" << cloveDisToUncongestedPath << "-";
		flowMonitorFilename << "FTMO" << cloveFlowletTimeout << "-HRTT" << cloveHalfRTT << "-UNCONGPATH" << cloveDisToUncongestedPath << "-";
		linkMonitorFilename << "FTMO" << cloveFlowletTimeout << "-HRTT" << cloveHalfRTT << "-UNCONGPATH" << cloveDisToUncongestedPath << "-";
	}


	flowMonitorFilename << "SEED" << randomSeed << "-";
	linkMonitorFilename << "SEED" << randomSeed << "-";


	if (asymCapacity)
	{
		flowMonitorFilename << "ASYM-";
		linkMonitorFilename << "ASYM-";
	}

	flowMonitorFilename << "B" << BUFFER_SIZE << ".xml";
	linkMonitorFilename << "B" << BUFFER_SIZE << "-link-utility.out";


	// salvatorg
	if (runMode == Clove)
	{
		NS_LOG_INFO ("Enabling Clove tracing");
		NS_LOG_INFO("FLOWLET TIMEOUT\t: "<< cloveFlowletTimeout << "us");
		NS_LOG_INFO ("RED_QUEUE_MARKING\t: " << RED_QUEUE_MARKING << " packets, " << RED_QUEUE_MARKING*PACKET_SIZE << "Bytes");
		NS_LOG_INFO ("WORKLOAD CDF\t: " << cdfFileName);
		Config::ConnectWithoutContext ("/NodeList/*/$ns3::Ipv4Clove/ClovePathTrace", MakeCallback (&ClovePathTrace));
		// Config::ConnectWithoutContext ("/NodeList/*/$ns3::Ipv4Clove/CloveQueuesTrace", MakeCallback (&CloveQueuesTrace));
	}

	NS_LOG_INFO ("Total Number of Netwrok Elements in simualtion\t:" << ns3::NodeList::GetNNodes() );
//   if (false)
//     {
//       filePlotQueue << "./" << flowMonitorFilename.str () << "_queuesUtil.plotme";
//       remove (filePlotQueue.str ().c_str ());
//     //   remove (filePlotQSizes.str ().c_str ());
//       //Ptr<QueueDisc> queue = queueDiscs.Get (0);
//       Simulator::ScheduleNow (&CheckQueueSize, queueDiscs);
//     }

	// NS_LOG_INFO ("Start Spine Queues Tracing System");

	// for (uint i=0; i<8; i++)
	// 	for (uint j=0; j<8; j++)
	//     	Simulator::ScheduleNow (&CheckQueueDiscSize, SpineQueueDiscs[i][j], i, j);
	AsciiTraceHelper ascii;
	p2p.EnableAsciiAll (ascii.CreateFileStream ("csma-multicast.tr"));

	NS_LOG_INFO ("Start simulation");
	Simulator::Stop (Seconds (END_TIME));
	Simulator::Run ();

	flowMonitor->SerializeToXmlFile(flowMonitorFilename.str (), true, true);
	linkMonitor->OutputToFile (linkMonitorFilename.str (), &LinkMonitor::DefaultFormat);

	Simulator::Destroy ();

	free_cdf (cdfTable);

	// for (int i=0; i<8; i++)
	// {
	// 	DoGnuPlot (i, END_TIME);
	// 	DoGnuPlotPdf (i, END_TIME);
	// }	



	// salvatorg
	if (runMode == Clove)
	{
		NS_LOG_INFO ("### All Flows ###");
		for (int k = 0; k < SERVER_COUNT * LEAF_COUNT ; k++)//SERVER_COUNT * LEAF_COUNT
		{
			NS_LOG_INFO ("[Server " << k << "]");
			// For each server which implements an Ipv4Clove stack
			// get number of stats.
			Ptr<Ipv4Clove> clove = servers.Get (k)->GetObject<Ipv4Clove> ();
			clove->GetStats();
 		}
		NS_LOG_INFO ("### Long Flows ###");
		for (int k = 0; k < SERVER_COUNT * LEAF_COUNT; k++)//SERVER_COUNT * LEAF_COUNT
		{
			NS_LOG_INFO ("[Server " << k << "]");
			// For each server which implements an Ipv4Clove stack
			// get number of stats.
			Ptr<Ipv4Clove> clove = servers.Get (k)->GetObject<Ipv4Clove> ();
			clove->GetStatsLongFlows();
		}
		NS_LOG_INFO ("### Short Flows ###");
		for (int k = 0; k < SERVER_COUNT * LEAF_COUNT; k++)//SERVER_COUNT * LEAF_COUNT
		{
			NS_LOG_INFO ("[Server " << k << "]");
			// For each server which implements an Ipv4Clove stack
			// get number of stats.
			Ptr<Ipv4Clove> clove = servers.Get (k)->GetObject<Ipv4Clove> ();
			clove->GetStatsLongFlows();
		}
		// Stats for the number of ECN bits that have been set by the queues 
		GetNumECNs();
	}




	NS_LOG_INFO (flowMonitorFilename.str ());
	NS_LOG_INFO (linkMonitorFilename.str ());
	NS_LOG_INFO ("Stop simulation");
}
