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
  c.Create (4);
  NodeContainer n0n2 = NodeContainer (c.Get (0), c.Get (2));
  NodeContainer n1n2 = NodeContainer (c.Get (1), c.Get (2));
  NodeContainer n3n2 = NodeContainer (c.Get (3), c.Get (2)); 		//?

  InternetStackHelper internet;
  internet.Install (c);
  
  // -------------------- CREAZIONE CANALI -----------------------------
  NS_LOG_INFO ("Create channels.");
  PointToPointHelper p2p;
  // Canale server<-->router
  p2p.SetDeviceAttribute ("DataRate", StringValue ("500Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer d0d2 = p2p.Install (n0n2);
  // Canale utente1<-->router
  p2p.SetDeviceAttribute ("DataRate", StringValue ("7Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));
  NetDeviceContainer d1d2 = p2p.Install (n1n2);
  // Canale utente2<-->router
  p2p.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));
  NetDeviceContainer d3d2 = p2p.Install (n3n2); 					//?
  

  // ----------------- ASSEGNAZIONE INDIRIZZI --------------------------
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i0i2 = ipv4.Assign (d0d2);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i1i2 = ipv4.Assign (d1d2);

  ipv4.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i3i2 = ipv4.Assign (d3d2); 				//?
  

  // ------------------- TABELLE DI ROUTING ----------------------------
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  

  // ----------------- CREAZIONE APPLICAZIONI --------------------------
  NS_LOG_INFO ("Create Applications.");
  
  // Definizione porte
  uint16_t port_ftp = 20;         // FTP data
  uint16_t port_voip = 6000;      // VoIP data
  uint16_t port_voip_ctrl = 6001; // VoIP control 
  
  // Mittente FTP (server su nodo 0)
  OnOffHelper onoff_ftp("ns3::TcpSocketFactory", Address(InetSocketAddress (i3i2.GetAddress (0), port_ftp)));
  onoff_ftp.SetConstantRate (DataRate ("5Mbps"));
  ApplicationContainer ftp_server = onoff_ftp.Install (c.Get (0));
  ftp_server.Start(Seconds(0.0));
  ftp_server.Stop(Seconds(11.0)); 
  
  // Destinatario FTP (client su nodo 3)
  PacketSinkHelper sink_ftp ("ns3::TcpSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny (), port_ftp)));
  ApplicationContainer ftp_client = sink_ftp.Install (c.Get (3));
  ftp_client.Start (Seconds (0.0));
  ftp_client.Stop (Seconds (14.0));

  // Chiamante VoIP (client su nodo 1)
  OnOffHelper onoff ("ns3::UdpSocketFactory", Address(InetSocketAddress (i3i2.GetAddress (0), port_voip)));
  onoff.SetConstantRate (DataRate ("1Mbps"));
  onoff.SetAttribute ("Remote", AddressValue (InetSocketAddress (i3i2.GetAddress(0), port_voip)));
  ApplicationContainer app_voip = onoff.Install (c.Get (1));
  app_voip.Start (Seconds (4.0));
  app_voip.Stop (Seconds (12.0));
  // Chiamante VoIP (client su nodo 1) [connessione di controllo]
  OnOffHelper onoff_voip_tcp("ns3::TcpSocketFactory", Address(InetSocketAddress (i3i2.GetAddress (0), port_voip_ctrl)));
  onoff_voip_tcp.SetConstantRate (DataRate ("64kbps"));
  onoff_voip_tcp.SetAttribute ("Remote", AddressValue (InetSocketAddress (i3i2.GetAddress(0), port_voip_ctrl)));
  ApplicationContainer app_voip_ctrl = onoff_voip_tcp.Install (c.Get (1));
  app_voip_ctrl.Start (Seconds (3.0));
  app_voip_ctrl.Stop (Seconds (13.0));

  // Ricevente VoIP (client su nodo 3)
  PacketSinkHelper sink_voip ("ns3::UdpSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny (), port_voip)));
  app_voip = sink_voip.Install (c.Get (3));
  app_voip.Start (Seconds (0.0));
  app_voip.Stop (Seconds (14.0));
  // Ricevente VoIP (client su nodo 3) [connessione di controllo]
  PacketSinkHelper sink_voip_ctrl ("ns3::TcpSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny (), port_voip_ctrl)));
  app_voip_ctrl = sink_voip_ctrl.Install (c.Get (3));
  app_voip_ctrl.Start (Seconds (0.0));
  app_voip_ctrl.Stop (Seconds (14.0));

  AsciiTraceHelper ascii;
  p2p.EnableAsciiAll (ascii.CreateFileStream ("PS.tr"));
  p2p.EnablePcapAll ("PS");

  // Flow Monitor
  FlowMonitorHelper flowmonHelper;
  if (enableFlowMonitor){
      flowmonHelper.InstallAll();
  }

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (15.0));
  Simulator::Run ();
  NS_LOG_INFO ("Done.");
  
  // --------------- CONTEGGIO BYTE APPLICAZIONI -----------------------
  uint32_t totalRxBytesCounter_ftp = 0;
  uint32_t totalRxBytesCounter_voip_data = 0;
  uint32_t totalRxBytesCounter_voip_ctrl = 0;
  
  //Applicazione FTP
  Ptr <Application> app1 = ftp_client.Get (0);
  Ptr <PacketSink> pktSink1 = DynamicCast <PacketSink> (app1);
  totalRxBytesCounter_ftp += pktSink1->GetTotalRx ();
  cout << "Goodput FTP: " << (totalRxBytesCounter_ftp/Simulator::Now ().GetSeconds ())/1000 << " kB/s" << endl;
  
  //Applicazione VOIP DATA
  Ptr <Application> app2 = app_voip.Get (0);
  Ptr <PacketSink> pktSink2 = DynamicCast <PacketSink> (app2);
  totalRxBytesCounter_voip_data += pktSink2->GetTotalRx ();
  cout << "Goodput VOIP DATA: " << (totalRxBytesCounter_voip_data/Simulator::Now ().GetSeconds ())/1000 << " kB/s" << endl;
  
  //Applicazione VOIP CTRL
  Ptr <Application> app3 = app_voip_ctrl.Get (0);
  Ptr <PacketSink> pktSink3 = DynamicCast <PacketSink> (app3);
  totalRxBytesCounter_voip_ctrl += pktSink3->GetTotalRx ();
  cout << "Goodput VOIP CTRL: " << (totalRxBytesCounter_voip_ctrl/Simulator::Now ().GetSeconds ())/1000 << " kB/s" << endl;
  

  Simulator::Destroy ();
  return 0;
}
