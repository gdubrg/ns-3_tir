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
using namespace std;

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
  NodeContainer c;
  c.Create (3);
  NodeContainer n0n1 = NodeContainer (c.Get (0), c.Get (1));
  NodeContainer n2n1 = NodeContainer (c.Get (2), c.Get (1));

  InternetStackHelper internet;
  internet.Install (c);
  

  // -------------------- CREAZIONE CANALI -----------------------------
  NS_LOG_INFO ("Create channels.");
  PointToPointHelper p2p;
  // Canale nodo1<-->router
  p2p.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer d0d1 = p2p.Install (n0n1);
  
  // Canale router<-->nodo2
  p2p.SetDeviceAttribute ("DataRate", StringValue ("7Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));
  
  
  double      minTh = 20;
  double      maxTh = 80;
  p2p.SetQueue ("ns3::RedQueue",
                 "MinTh", DoubleValue (minTh),
                 "MaxTh", DoubleValue (maxTh),
                 "LinkBandwidth", StringValue ("7Mbps"),
                 "LinkDelay", StringValue ("10ms"));
                 
                 
                 
  NetDeviceContainer d2d1 = p2p.Install (n2n1);
  Ptr<NetDevice> dev_ptr = d2d1.Get(0);
  // è l'interfaccia giusta? controllo a che nodo è associata
  Ptr<Node> nodo_associato = dev_ptr->GetNode();
  std::cout << "ID: " << nodo_associato->GetId() << std::endl;
  dev_ptr->SetAttribute("DataRate", StringValue ("64kbps"));
  
  // ----------------- ASSEGNAZIONE INDIRIZZI --------------------------
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i0i1 = ipv4.Assign (d0d1);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i2i1 = ipv4.Assign (d2d1);
  

  // ------------------- TABELLE DI ROUTING ----------------------------
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  

  // ----------------- CREAZIONE APPLICAZIONI --------------------------
  NS_LOG_INFO ("Create Applications.");
  
  // Definizione porte
  uint16_t port_ftp = 20;         // FTP data
  uint16_t port_ftp2 = 30;         // FTP data
  //uint32_t file_dim = 10000000; // 10Mb 
  
  //Mittente FTP (server su nodo 0)
  OnOffHelper onoff_ftp1("ns3::TcpSocketFactory", Address(InetSocketAddress (i2i1.GetAddress (0), port_ftp)));
  onoff_ftp1.SetConstantRate (DataRate ("5Mbps"));
  onoff_ftp1.SetAttribute("PacketSize", UintegerValue(512));
  onoff_ftp1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onoff_ftp1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  ApplicationContainer ftp_server1 = onoff_ftp1.Install (c.Get (0));
  ftp_server1.Start(Seconds(0.0));
  ftp_server1.Stop(Seconds(10.0)); 
  
  // Destinatario FTP (client su nodo 2)
  PacketSinkHelper sink_ftp1 ("ns3::TcpSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny (), port_ftp)));
  ApplicationContainer ftp_client1 = sink_ftp1.Install (c.Get (2));
  ftp_client1.Start (Seconds (0.0));
  ftp_client1.Stop (Seconds (12.0));

  // Mittente FTP (server su nodo 2)
  OnOffHelper onoff_ftp2("ns3::TcpSocketFactory", Address(InetSocketAddress (i0i1.GetAddress (0), port_ftp2)));
  onoff_ftp2.SetConstantRate (DataRate ("5Mbps"));
  onoff_ftp2.SetAttribute("PacketSize", UintegerValue(512));
  onoff_ftp2.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onoff_ftp2.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  ApplicationContainer ftp_server2 = onoff_ftp2.Install (c.Get (2));
  ftp_server2.Start(Seconds(3.0));
  ftp_server2.Stop(Seconds(10.0)); 

  // Destinatario FTP (client su nodo 0)
  PacketSinkHelper sink_ftp2 ("ns3::TcpSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny (), port_ftp2)));
  ApplicationContainer ftp_client2 = sink_ftp2.Install (c.Get (0));
  ftp_client2.Start (Seconds (0.0));
  ftp_client2.Stop (Seconds (12.0));

  AsciiTraceHelper ascii;
  p2p.EnableAsciiAll (ascii.CreateFileStream ("AQM_RED.tr"));
  p2p.EnablePcapAll ("AQM_RED");

  // Flow Monitor
  FlowMonitorHelper flowmonHelper;
  if (enableFlowMonitor){
      flowmonHelper.InstallAll();
  }

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (13.0));
  Simulator::Run ();
  NS_LOG_INFO ("Done.");
  
  
  // --------------- CONTEGGIO BYTE APPLICAZIONI -----------------------
  uint32_t totalRxBytesCounter_ftp1 = 0;
  uint32_t totalRxBytesCounter_ftp2 = 0;
  
  //Applicazione FTP 1
  Ptr <Application> app1 = ftp_client1.Get (0);
  Ptr <PacketSink> pktSink1 = DynamicCast <PacketSink> (app1);
  totalRxBytesCounter_ftp1 += pktSink1->GetTotalRx ();
  cout << "Goodput FTP1: " << (totalRxBytesCounter_ftp1/Simulator::Now ().GetSeconds ())/1000 << " kB/s" << endl;
  
  //Applicazione FTP 2
  Ptr <Application> app2 = ftp_client2.Get (0);
  Ptr <PacketSink> pktSink2 = DynamicCast <PacketSink> (app2);
  totalRxBytesCounter_ftp2 += pktSink2->GetTotalRx ();
  cout << "Goodput FTP2: " << (totalRxBytesCounter_ftp2/Simulator::Now ().GetSeconds ())/1000 << " kB/s" << endl;
  
  if (enableFlowMonitor){
      flowmonHelper.SerializeToXmlFile ("simple-global-routing.flowmon", false, false);
  }
  
  //Fine della simulazione
  Simulator::Destroy ();
  return 0;
}
