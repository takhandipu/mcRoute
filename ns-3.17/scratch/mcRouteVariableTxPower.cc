#include "ns3/aodv-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h" 
#include "ns3/v4ping-helper.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include <iostream>
#include <cmath>
#include <stdio.h>

using namespace ns3;

class Location
{
public:
  int x;
  int y;
};

class Topology
{
public:
  int time;
  int np;
  int ns;
  int nRow;
  int nCol;
  int dBm;
  Location *loc;
  //int **neighbors;
  Topology()
  {
    //FILE *ifp = fopen("10.txt", "r");
    //fscanf(ifp, "%d %d %d %d %d", &time, &np, &ns, &nRow, &nCol);
    time = 100;
    np = 5;
    ns = 10;
    nRow = 5000;
    nCol = 5000;
    dBm = 20;
    /*loc = new Location[2*np+ns];
    for(int i = 0; i < 2*np+ns; i++)
    {
      fscanf(ifp, "%d %d", &loc[i].x, &loc[i].y);
    }
    neighbors = new int*[2*np+ns];
    for(int i = 0; i < np; i++)
    {
      neighbors[i] = new int[];
      fscanf(ifp, "%d ");
    }*/
    //fclose(ifp);
    /*printf("%d %d %d %d %d\n", time, np, ns, nRow, nCol);
    for(int i = 0; i < 2*np+ns; i++)
    {
      printf("%d %d\n", loc[i].x, loc[i].y);
    }*/
  }
};
Topology t;
/**
 * \brief Test script.
 * 
 * This script creates 1-dimensional grid topology and then ping last node from the first one:
 * 
 * [10.0.0.1] <-- step --> [10.0.0.2] <-- step --> [10.0.0.3] <-- step --> [10.0.0.4]
 * 
 * ping 10.0.0.4
 */
class AodvExample 
{
public:
  AodvExample ();
  /// Configure script parameters, \return true on successful configuration
  bool Configure (int argc, char **argv);
  /// Run simulation
  void Run ();
  /// Report results
  void Report (std::ostream & os);

private:
  ///\name parameters
  //\{
  /// Number of nodes
  uint32_t np;
  uint32_t ns;
  /// Simulation time, seconds
  double totalTime;
  /// Print routes if true
  bool printRoutes;
  //\}

  ///\name network
  //\{
  NodeContainer puNodes;
  NodeContainer suNodes;
  NodeContainer suNodes1;
  NodeContainer suNodes2;
  NetDeviceContainer puDevices;
  NetDeviceContainer suDevices;
  NetDeviceContainer suDevices1;
  NetDeviceContainer suDevices2;
  Ipv4InterfaceContainer puInterfaces;
  Ipv4InterfaceContainer suInterfaces;
  Ipv4InterfaceContainer suInterfaces1;
  Ipv4InterfaceContainer suInterfaces2;
  //\}

private:
  void CreateNodes ();
  void CreateDevices ();
  void InstallInternetStack ();
  void InstallApplications ();
  void InstallFlowMonitor ();
};

int main (int argc, char **argv)
{
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (1024));
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("10Mbps"));
  // Enable AODV logs by default. Comment this if too noisy
  //LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL);
  int seed=12345;
  
  CommandLine cmd;
  cmd.AddValue ("seed", "Random Seed", seed);

  //cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  //cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("np", "Number of primary users.", t.np);
  cmd.AddValue ("ns", "Number of secondary users.", t.ns);
  //cmd.AddValue ("dBm", "Transmission Power Level.", t.dBm);
  //cmd.AddValue ("size", "Number of nodes.", size);
  //cmd.AddValue ("time", "Simulation time, s.", totalTime);
  //cmd.AddValue ("step", "Grid step, m", step);

  cmd.Parse (argc, argv);
  std::cout << (unsigned)seed << " ";
  SeedManager::SetSeed (seed);
  AodvExample test;
  if (!test.Configure (argc, argv))
    NS_FATAL_ERROR ("Configuration failed. Aborted.");

  test.Run ();
  test.Report (std::cout);
  return 0;
}

//-----------------------------------------------------------------------------
AodvExample::AodvExample () :
  np (t.np),
  ns (t.ns),
  totalTime (t.time),
  printRoutes (true)
{
}

bool
AodvExample::Configure (int argc, char **argv)
{
  // Enable AODV logs by default. Comment this if too noisy
  //LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL);
  /*int seed=12345;
  
  CommandLine cmd;
  cmd.AddValue ("seed", "Random Seed", seed);

  //cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("np", "Number of primary users.", t.np);
  //cmd.AddValue ("size", "Number of nodes.", size);
  //cmd.AddValue ("time", "Simulation time, s.", totalTime);
  //cmd.AddValue ("step", "Grid step, m", step);

  cmd.Parse (argc, argv);
  SeedManager::SetSeed (seed);*/
  return true;
}

void
AodvExample::Run ()
{
//  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (1)); // enable rts cts all the time.
  CreateNodes ();
  CreateDevices ();
  InstallInternetStack ();
  InstallApplications ();

  std::cout << totalTime << " ";
  
  InstallFlowMonitor ();
  
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  Simulator::Stop (Seconds (totalTime));
  Simulator::Run ();
  Simulator::Destroy ();
  
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

  uint64_t bytesTotal = 0;
  double lastRxTime=-1;
  double firstRxTime=-1;

  Time delaySums =Seconds(0.0);
  uint64_t rxPackets=0;
  uint64_t txPackets=0;
  
  uint64_t bytesTotalSU = 0;
  double lastRxTimeSU=-1;
  double firstRxTimeSU=-1;

  Time delaySumsSU =Seconds(0.0);
  uint64_t rxPacketsSU=0;
  uint64_t txPacketsSU=0;
  
  double lostPackets=0;
  double lostPacketsSU=0;
  
  uint64_t timesForwarded=0;
  uint64_t forwardedPacketCount=0;
  uint64_t maxForwarded=0;
  uint64_t minForwarded=0;
  bool first = true;
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
  {
	  
	  Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

      if (t.sourceAddress.isSU())
      {
	  //Find the time of first packet arrived to the sink node from all flows
	  if (firstRxTimeSU < 0)
		  firstRxTimeSU = i->second.timeFirstRxPacket.GetSeconds();
	  else
		  if (firstRxTimeSU > i->second.timeFirstRxPacket.GetSeconds() )
			  firstRxTimeSU = i->second.timeFirstRxPacket.GetSeconds();

	  //Find the time of last packet arrived to the sink node from all flows
	  if (lastRxTimeSU < i->second.timeLastRxPacket.GetSeconds() )
		  lastRxTimeSU = i->second.timeLastRxPacket.GetSeconds();

	  //Sum all received bytes of all flows
	  bytesTotalSU = bytesTotalSU + i->second.rxBytes;
	  delaySumsSU +=  i->second.delaySum;
	  rxPacketsSU += i->second.rxPackets;
	  txPacketsSU += i->second.txPackets;
	  lostPacketsSU += abs(i->second.txPackets - i->second.rxPackets);
      } else {
	  //Find the time of first packet arrived to the sink node from all flows
	  if (firstRxTime < 0)
		  firstRxTime = i->second.timeFirstRxPacket.GetSeconds();
	  else
		  if (firstRxTime > i->second.timeFirstRxPacket.GetSeconds() )
			  firstRxTime = i->second.timeFirstRxPacket.GetSeconds();

	  //Find the time of last packet arrived to the sink node from all flows
	  if (lastRxTime < i->second.timeLastRxPacket.GetSeconds() )
		  lastRxTime = i->second.timeLastRxPacket.GetSeconds();

	  //Sum all received bytes of all flows
	  bytesTotal = bytesTotal + i->second.rxBytes;
	  delaySums +=  i->second.delaySum;
	  rxPackets += i->second.rxPackets;
	  txPackets += i->second.txPackets;
	  lostPackets += abs(i->second.txPackets - i->second.rxPackets);
      }
      if(i->second.timesForwarded != 0)
      {
        forwardedPacketCount += 1;
        timesForwarded += i->second.timesForwarded;
        if(first) {
	  first = false;
	  maxForwarded = i->second.timesForwarded;
	  minForwarded = i->second.timesForwarded;
        } else {
          if(i->second.timesForwarded > maxForwarded) {
  	    maxForwarded = i->second.timesForwarded;
          }
          if(i->second.timesForwarded < minForwarded) {
  	    minForwarded = i->second.timesForwarded;
          }
        }
      }
      //std::cout<<i->second.timesForwarded<<std::endl;
  }

  /*std::cout << "Num clients = " << 2 * np + ns << " "
		  << "Avg throughput = "
		  << bytesTotal*8/(lastRxTime-firstRxTime)/1024
		  << " kbits/sec" << std::endl;

  std::cout << "Num clients = " << 2 * np + ns << " "
		  << "Avg Delay = "
		  << delaySums / rxPackets / 1000000
		  << " milliseconds" << std::endl;*/
  std::cout<<bytesTotal*8/(lastRxTime-firstRxTime)/1024<<" ";
  std::cout<<delaySums / rxPackets / 1000000000<<" ";
  //std::cout<<bytesTotal*8/(lastRxTime-firstRxTime)/1024/nNodes<<" ";
  std::cout<<lostPackets / (rxPackets+lostPackets)<<" ";
  //std::cout<<rxPackets<<" ";
  //std::cout<<txPackets<<" ";
  //std::cout<<std::endl;
  
  std::cout<<bytesTotalSU*8/(lastRxTimeSU-firstRxTimeSU)/1024<<" ";
  std::cout<<delaySumsSU / rxPacketsSU / 1000000000<<" ";
  //std::cout<<bytesTotal*8/(lastRxTime-firstRxTime)/1024/nNodes<<" ";
  std::cout<<lostPacketsSU / (rxPacketsSU+lostPacketsSU)<<" ";
  //std::cout<<rxPackets<<" ";
  //std::cout<<txPackets<<" ";
  //std::cout<<std::endl;
  std::cout<<(1.0 +(timesForwarded / forwardedPacketCount))<<" "<<(1+maxForwarded)<<" "<< (1+minForwarded);
  std::cout<<std::endl;
}

void
AodvExample::Report (std::ostream &)
{ 
}

void
AodvExample::InstallFlowMonitor ()
{
  /*FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();*/
}

void
AodvExample::CreateNodes ()
{
  std::cout << (unsigned)2*np << " ";
  puNodes.Create (2*np);
  // Name nodes
  for (uint32_t i = 0; i < 2*np; ++i)
    {
      std::ostringstream os;
      os << "pu-node-" << i;
      Names::Add (os.str (), puNodes.Get (i));
    }
  std::cout << (unsigned)(3*ns) << " ";
  suNodes.Create(ns);
  suNodes1.Create(ns);
  suNodes2.Create(ns);
  std::cout << (unsigned)t.dBm << " ";
  for(uint32_t i = 0; i < ns; i++)
  {
    std::ostringstream os;
    os << "su-node-" << i+2*np;
    Names::Add (os.str (), suNodes.Get (i));
  }
  for(uint32_t i = 0; i < ns; i++)
  {
    std::ostringstream os;
    os << "su-node-" << i+2*np+ns;
    Names::Add (os.str (), suNodes1.Get (i));
  }
  for(uint32_t i = 0; i < ns; i++)
  {
    std::ostringstream os;
    os << "su-node-" << i+2*np+2*ns;
    Names::Add (os.str (), suNodes2.Get (i));
  }
  // Create static grid
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (20),
                                 "DeltaY", DoubleValue (20),
                                 "GridWidth", UintegerValue (t.nRow),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (puNodes);
  mobility.Install(suNodes);
  mobility.Install(suNodes1);
  mobility.Install(suNodes2);
  
  /*for (uint32_t i = 0; i < 2*np; ++i)
    {
      Ptr<Node> node = puNodes.Get (i);
      Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
      Simulator::Schedule (Seconds (0.0), &MobilityModel::SetPosition, mob, Vector (t.loc[i].x, t.loc[i].y, 0));
    }
  
  for (uint32_t i = 0; i < ns; ++i)
    {
      Ptr<Node> node = suNodes.Get (i);
      Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
      Simulator::Schedule (Seconds (0.0), &MobilityModel::SetPosition, mob, Vector (t.loc[i+2*np].x, t.loc[i+2*np].y, 0));
    }*/
}

void
AodvExample::CreateDevices ()
{
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifiMac.SetType ("ns3::AdhocWifiMac");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.Set ("TxPowerStart", DoubleValue(t.dBm));
  wifiPhy.Set ("TxPowerEnd", DoubleValue(t.dBm));
  wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
  wifiPhy.Set ("TxGain", DoubleValue(5) );
  wifiPhy.Set ("RxGain", DoubleValue (5) );
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));
  puDevices = wifi.Install (wifiPhy, wifiMac, puNodes); 
  suDevices = wifi.Install (wifiPhy, wifiMac, suNodes);
  
  wifiPhy.Set ("TxPowerStart", DoubleValue(t.dBm));
  wifiPhy.Set ("TxPowerEnd", DoubleValue(t.dBm));
  wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
  wifiPhy.Set ("TxGain", DoubleValue(2.5) );
  wifiPhy.Set ("RxGain", DoubleValue (2.5) ); 
  suDevices1 = wifi.Install (wifiPhy, wifiMac, suNodes1);
  
  wifiPhy.Set ("TxPowerStart", DoubleValue(t.dBm));
  wifiPhy.Set ("TxPowerEnd", DoubleValue(t.dBm));
  wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
  wifiPhy.Set ("TxGain", DoubleValue(1) );
  wifiPhy.Set ("RxGain", DoubleValue (1) ); 
  suDevices2 = wifi.Install (wifiPhy, wifiMac, suNodes2);
  
  /*if (pcap)
    {
      wifiPhy.EnablePcapAll (std::string ("mcRoute"));
    }*/
}

void
AodvExample::InstallInternetStack ()
{
  AodvHelper aodv;
  // you can configure AODV attributes here using aodv.Set(name, value)
  InternetStackHelper stack;
  stack.SetRoutingHelper (aodv); // has effect on the next Install ()
  stack.Install (puNodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.0.0.0");
  puInterfaces = address.Assign (puDevices);
  
  stack.Install (suNodes);
  stack.Install (suNodes1);
  stack.Install (suNodes2);
  address.SetBase ("10.0.0.0", "255.0.0.0", "0.0.0.101");
  suInterfaces = address.Assign (suDevices);
  suInterfaces1 = address.Assign (suDevices1);
  suInterfaces2 = address.Assign (suDevices2);

  if (printRoutes)
    {
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("aodv.routes", std::ios::out);
      aodv.PrintRoutingTableAllAt (Seconds (totalTime-1), routingStream);
    }
}

void
AodvExample::InstallApplications ()
{
  OnOffHelper clientHelper ("ns3::TcpSocketFactory", Address ());
  clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  ApplicationContainer clientApps;
  uint16_t port = 50000;
  for(uint32_t i=0; i<np; ++i)
  {
    AddressValue remoteAddress (InetSocketAddress (puInterfaces.GetAddress(np+i), port));
    clientHelper.SetAttribute ("Remote", remoteAddress);
    clientApps.Add (clientHelper.Install (puNodes.Get(i)));
  }
  for(uint32_t i=0; i<ns/2; ++i)
  {
    AddressValue remoteAddress (InetSocketAddress (suInterfaces.GetAddress( (ns/2) + i), port));
    clientHelper.SetAttribute ("Remote", remoteAddress);
    clientApps.Add (clientHelper.Install (suNodes.Get(i)));
  }
  for(uint32_t i=0; i<ns/2; ++i)
  {
    AddressValue remoteAddress (InetSocketAddress (suInterfaces1.GetAddress( (ns/2) + i), port));
    clientHelper.SetAttribute ("Remote", remoteAddress);
    clientApps.Add (clientHelper.Install (suNodes1.Get(i)));
  }
  for(uint32_t i=0; i<ns/2; ++i)
  {
    AddressValue remoteAddress (InetSocketAddress (suInterfaces2.GetAddress( (ns/2) + i), port));
    clientHelper.SetAttribute ("Remote", remoteAddress);
    clientApps.Add (clientHelper.Install (suNodes2.Get(i)));
  }
  clientApps.Start (Seconds (0.0));
  clientApps.Stop (Seconds(totalTime) - Seconds(0.001));
  /*for(unsigned int i = 0; i < np; i++)
  {
    V4PingHelper ping (puInterfaces.GetAddress(np+i));
    ping.SetAttribute ("Verbose", BooleanValue (false));
    //ping.SetAttribute ("Interval", StringValue ("0.1s"));
    ApplicationContainer p = ping.Install(puNodes.Get(i));
    p.Start(Seconds(0));
    p.Stop(Seconds(totalTime) - Seconds(0.001));
  }
  for(unsigned int i = 0; i < ns/2; i++)
  {
    V4PingHelper ping (suInterfaces.GetAddress((ns/2)+i));
    ping.SetAttribute ("Verbose", BooleanValue (false));
    //ping.SetAttribute ("Interval", StringValue ("0.1s"));
    ApplicationContainer p = ping.Install(suNodes.Get(i));
    p.Start(Seconds(0));
    p.Stop(Seconds(totalTime) - Seconds(0.001));
  }*/
  /*for(unsigned int i = 0; i < 1; i++)
  {
    for(unsigned int j = 0; j < ns; j++)
    {
      V4PingHelper ping (suInterfaces.GetAddress(j));
      ping.SetAttribute ("Verbose", BooleanValue (true));
      ApplicationContainer p = ping.Install(puNodes.Get(i));
      p.Start(Seconds(0));
      p.Stop(Seconds(totalTime) - Seconds(0.001));
    }
  }*/
}

