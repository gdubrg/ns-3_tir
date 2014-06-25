/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

// TO-ASK LIST
// 1 - è sufficiente la topologia della rete?
// 2 - vanno bene i protocolli ?
// 3 - [il codice ha senso ?]
// 4 - grafici con wireshark ? istante di partenza?
// 5- 
//

//
// Network topology
//
//  n0
//     \ 5 Mb/s, 2ms
//      \          1.5Mb/s, 10ms
//       n2 -------------------------n3
//      /
//     / 5 Mb/s, 2ms
//   n1
//
// - all links are point-to-point links with indicated one-way BW/delay
// - CBR/UDP flows from n0 to n3, and from n3 to n1
// - FTP/TCP flow from n0 to n3, starting at time 1.2 to time 1.35 sec.
// - UDP packet size of 210 bytes, with per-packet interval 0.00375 sec.
//   (i.e., DataRate of 448,000 bps)
// - DropTail queues 
// - Tracing of queues and packet receptions to file "simple-global-routing.tr"

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

  // Set up some default values for the simulation.  Use the 
  //Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (210));
  //Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("448kb/s"));

  //DefaultValue::Bind ("DropTailQueue::m_maxPackets", 30);

  // Allow the user to override any of the defaults and the above
  // DefaultValue::Bind ()s at run-time, via command-line arguments
  CommandLine cmd;
  bool enableFlowMonitor = false;
  cmd.AddValue ("EnableMonitor", "Enable Flow Monitor", enableFlowMonitor);
  cmd.Parse (argc, argv);

  // Here, we will explicitly create four nodes.  In more sophisticated
  // topologies, we could configure a node factory.
  NS_LOG_INFO ("Create nodes.");
  NodeContainer c;
  c.Create (4);
  NodeContainer n0n2 = NodeContainer (c.Get (0), c.Get (2));
  NodeContainer n1n2 = NodeContainer (c.Get (1), c.Get (2));
  NodeContainer n3n2 = NodeContainer (c.Get (3), c.Get (2));

  InternetStackHelper internet;
  internet.Install (c);

  // Creazione canali
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
  p2p.SetDeviceAttribute ("DataRate", StringValue ("512kbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));
  NetDeviceContainer d3d2 = p2p.Install (n3n2);

  // Assegnazione indirizzi IP
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i0i2 = ipv4.Assign (d0d2);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i1i2 = ipv4.Assign (d1d2);

  ipv4.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i3i2 = ipv4.Assign (d3d2);

  // Create router nodes, initialize routing database and set up the routing
  // tables in the nodes.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Create the OnOff application to send UDP datagrams of size
  // 210 bytes at a rate of 448 Kb/s
  NS_LOG_INFO ("Create Applications.");
  
  // Mittente FTP (server su nodo 0)
  uint16_t port_ftp = 20;   // FTP data
  uint32_t maxBytes = 10000000;
  BulkSendHelper source ("ns3::TcpSocketFactory", Address(InetSocketAddress (i3i2.GetAddress (0), port_ftp)));
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  //source.SetConstantRate (DataRate ("300Mbps"));
  ApplicationContainer app_ftp = source.Install (c.Get (0));
  app_ftp.Start (Seconds (1.0));
  app_ftp.Stop (Seconds (10.0));

  // Destinatario FTP (client su nodo 3)
  PacketSinkHelper sink_ftp ("ns3::TcpSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny (), port_ftp)));
  app_ftp = sink_ftp.Install (c.Get (3));
  app_ftp.Start (Seconds (1.0));
  app_ftp.Stop (Seconds (11.0));

  // Chiamante VoIP (client su nodo 1)
  uint16_t port_voip = 5000;   // VoIP data
  OnOffHelper onoff ("ns3::UdpSocketFactory", Address(InetSocketAddress (i3i2.GetAddress (0), port_voip)));
<<<<<<< HEAD
=======
  //OnOffHelper onoff ("ns3::UdpSocketFactory");
>>>>>>> dfb0c8fcd5fa8f120b4b79f81bf08ca478cb0da7
  onoff.SetConstantRate (DataRate ("512kb/s"));
  onoff.SetAttribute ("Remote", AddressValue (InetSocketAddress (i3i2.GetAddress(0), port_voip)));
  ApplicationContainer app_voip = onoff.Install (c.Get (1));
  app_voip.Start (Seconds (4.0));
  app_voip.Stop (Seconds (12.0));
  //Connessione TCP di controllo Udp
  uint16_t port_voip_tcp = 5001;   
  OnOffHelper onoff_voip_tcp("ns3::TcpSocketFactory", Address(InetSocketAddress (i3i2.GetAddress (0), port_voip_tcp)));
  onoff_voip_tcp.SetConstantRate (DataRate ("64kb/s"));
  onoff_voip_tcp.SetAttribute ("Remote", AddressValue (InetSocketAddress (i3i2.GetAddress(0), port_voip_tcp)));
  ApplicationContainer app_voip_tcp = onoff_voip_tcp.Install (c.Get (1));
  //Inizia in contemporanea alla connessione Udp
  app_voip_tcp.Start (Seconds (4.0));
  app_voip_tcp.Stop (Seconds (12.0));

  // Ricevente VoIP (client su nodo 3)
  PacketSinkHelper sink_voip ("ns3::UdpSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny (), port_voip)));
  app_voip = sink_voip.Install (c.Get (3));
  app_voip.Start (Seconds (1.0));
  app_voip.Stop (Seconds (15.0));

  AsciiTraceHelper ascii;
  p2p.EnableAsciiAll (ascii.CreateFileStream ("iRouting.tr"));
  p2p.EnablePcapAll ("iRouting");

  // Flow Monitor
  FlowMonitorHelper flowmonHelper;
  if (enableFlowMonitor)
    {
      flowmonHelper.InstallAll ();
    }

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (20.0));
  Simulator::Run ();
  NS_LOG_INFO ("Done.");

  if (enableFlowMonitor)
    {
      flowmonHelper.SerializeToXmlFile ("simple-global-routing.flowmon", false, false);
    }

  Simulator::Destroy ();
  return 0;
}
