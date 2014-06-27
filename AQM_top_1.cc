#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimpleGlobalRoutingExample");

int 
main (int argc, char *argv[])
{
  // Users may find it convenient to turn on explicit debugging
  // for selected modules; the below lines suggest how to do this
#if 0 
  LogComponentEnable ("SimpleGlobalRoutingExample", LOG_LEVEL_INFO);
#endif

  //DefaultValue::Bind ("DropTailQueue::m_maxPackets", 30);

  // Allow the user to override any of the defaults and the above
  // DefaultValue::Bind ()s at run-time, via command-line arguments
  CommandLine cmd;
  bool enableFlowMonitor = false;
  cmd.AddValue ("EnableMonitor", "Enable Flow Monitor", enableFlowMonitor);
  cmd.Parse (argc, argv);
  

  // --------------------- CREAZIONE NODI ------------------------------
  NS_LOG_INFO ("Create nodes.");
  
  NodeContainer nodes;
  nodes.Create (2);
  //NodeContainer n0n1 = NodeContainer (c.Get (0), c.Get (1));

  InternetStackHelper internet;
  internet.Install (nodes);
  


  // -------------------- CREAZIONE CANALI -----------------------------
  NS_LOG_INFO ("Create channels.");
  
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer d0d1 = p2p.Install (nodes);
  //NetDeviceContainer d0d1 = p2p.Install (n0n1);
  

  // ----------------- ASSEGNAZIONE INDIRIZZI --------------------------
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i0i1 = ipv4.Assign (d0d1);
  

  // ------------------- TABELLE DI ROUTING ----------------------------
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  




uint16_t port = 9;  // well-known echo port number
uint32_t maxBytes = 0;

  BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (i0i1.GetAddress (1), port));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  ApplicationContainer sourceApps = source.Install (nodes.Get (0));
  sourceApps.Start (Seconds (0.0));
  sourceApps.Stop (Seconds (10.0));

//
// Create a PacketSinkApplication and install it on node 1
//
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (nodes.Get (1));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (10.0));
  
  
  
  
  
  /*
  // ----------------- CREAZIONE APPLICAZIONI --------------------------
  NS_LOG_INFO ("Create Applications.");
  
  // Definizione porte
  //uint16_t port_ftp1 = 20;         // FTP1 data
  //uint16_t port_ftp2 = 30;         // FTP2 data
  //uint32_t maxBytes = 10000000; //inutile?
  
  // Server FTP 1 [ pacchetti grandi] (server su nodo 0)
  OnOffHelper onoff_ftp1("ns3::TcpSocketFactory", Address(InetSocketAddress (i0i1.GetAddress (0), port_ftp1)));
  onoff_ftp1.SetConstantRate (DataRate ("5Mbps"));
  ApplicationContainer ftp_server1 = onoff_ftp1.Install (nodes.Get (0));
  ftp_server1.Start(Seconds(0.0));
  ftp_server1.Stop(Seconds(11.0)); 
  
  // Server FTP 2 [ pacchetti piccoli] (server su nodo 0)
  //OnOffHelper onoff_ftp2("ns3::TcpSocketFactory", Address(InetSocketAddress (i0i1.GetAddress (0), port_ftp2)));
  //onoff_ftp2.SetConstantRate (DataRate ("5Mbps"));
  //ApplicationContainer ftp_server2 = onoff_ftp2.Install (c.Get (0));
  //ftp_server2.Start(Seconds(0.0));
  //ftp_server2.Stop(Seconds(11.0)); 
  
  // Client FTP1 (client su nodo 1)
  PacketSinkHelper sink_ftp1 ("ns3::TcpSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny (), port_ftp1)));
  ApplicationContainer ftp_client1 = sink_ftp1.Install (nodes.Get (1));
  ftp_client1.Start (Seconds (0.0));
  ftp_client1.Stop (Seconds (15.0));

  // Client FTP2 (client su nodo 1)
  //PacketSinkHelper sink_ftp2 ("ns3::TcpSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny (), port_ftp2)));
  //ApplicationContainer ftp_client2 = sink_ftp2.Install (c.Get (1));
  //ftp_client2.Start (Seconds (0.0));
  //ftp_client2.Stop (Seconds (15.0));

*/


  AsciiTraceHelper ascii;
  p2p.EnableAsciiAll (ascii.CreateFileStream ("AQM_top1.tr"));
  p2p.EnablePcapAll ("AQM_top1");

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (11.0));
  Simulator::Run ();
  NS_LOG_INFO ("Done.");

  Simulator::Destroy ();
  return 0;
}
