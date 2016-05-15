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
 */

/* Author: Jorge Guilherme Silva dos Santos
	Universidade de Brasilia - Brazil
*/

// Basic NS3 libraries

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <string>
#include <fstream>

using namespace ns3;		// Como sempre

// Log definition
NS_LOG_COMPONENT_DEFINE ("Experimento4");

int
main (int argc, char *argv[])
{
	//Variáveis que receberão os valores da linha de comando
	std::string saidaAscii;
	std::string saidaPcap;

	// Criação dos argumentos de linha de comanda para especificação dos arquivos
	// de saída
	CommandLine cmd;
	cmd.AddValue("ascii", "Nome do arquivo de saída ASCII: exemplo.tr", saidaAscii);
	cmd.AddValue("pcap", "Nome do arquivo de saída PCAP (sem extensão)", saidaPcap);
	cmd.Parse(argc, argv);

	Time::SetResolution (Time::NS);
	// Tipo de log definido para a aplicação
	LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
	LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

	// Define e cria quatro nós
	NodeContainer nodes;
	nodes.Create (4);

	// Configurar a pilha de protocolo IP nos nós
	InternetStackHelper stack;
	stack.Install (nodes);

	/* Aqui, NA = n0, NB = n1, NC = n3, ND = n2
	Para corresponder ao solicitado no roteiro

		n0 ===== n1 <- pTP01
		||		 ||
		||		 ||
		n3 ===== n2 <- pTP23
	*/

	// Separa os nós em diferentes containers para facilitar depois a criação dos Devices
	// Essa separação segue a lógica que irá aparecer para a criação dos enlaces ponto a ponto
	NodeContainer n0n1 = NodeContainer (nodes.Get(0), nodes.Get(1));
	NodeContainer n1n2 = NodeContainer (nodes.Get(1), nodes.Get(2));
	NodeContainer n2n3 = NodeContainer (nodes.Get(2), nodes.Get(3));
	NodeContainer n3n0 = NodeContainer (nodes.Get(3), nodes.Get(0));

	// Cria os enlaces ponto a ponto
	PointToPointHelper pTP01;
	PointToPointHelper pTP12;
	PointToPointHelper pTP23;
	PointToPointHelper pTP30;

	// Configura a taxa da conexão dos enlaces.
	pTP01.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
	pTP12.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
	pTP23.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
	pTP30.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));

	// Configura o atraso dos enlaces.
	pTP01.SetChannelAttribute ("Delay", StringValue ("1ms"));
	pTP12.SetChannelAttribute ("Delay", StringValue ("1ms"));
	pTP23.SetChannelAttribute ("Delay", StringValue ("1ms"));
	pTP30.SetChannelAttribute ("Delay", StringValue ("1ms"));

	// Cria os dispositivos e associando-os ao enlace
	NetDeviceContainer d0d1;
	NetDeviceContainer d1d2;
	NetDeviceContainer d2d3;
	NetDeviceContainer d3d0;
	d0d1 = pTP01.Install (n0n1);
	d1d2 = pTP12.Install (n1n2);
	d2d3 = pTP23.Install (n2n3);
	d3d0 = pTP30.Install (n3n0);

	// Configura os endereços IP e máscaras de rede
	// Cada enlace ponto a ponto com uma rede
	Ipv4AddressHelper ipv4;
	ipv4.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer i0i1 = ipv4.Assign (d0d1);

	ipv4.SetBase ("10.2.2.0", "255.255.255.0");
	Ipv4InterfaceContainer i1i2 = ipv4.Assign (d1d2[]);

	ipv4.SetBase ("10.3.3.0", "255.255.255.0");
	Ipv4InterfaceContainer i2i3 = ipv4.Assign (d2d3);

	ipv4.SetBase ("10.4.4.0", "255.255.255.0");
	Ipv4InterfaceContainer i3i0 = ipv4.Assign (d3d0);

	// Cria o servidor da aplicação de Echo na porta 9
	UdpEchoServerHelper echoServer (9);

	// Configura o segundo nó como servidor da aplicação de Echo
	ApplicationContainer serverApps = echoServer.Install (n1n2.Get (1));

	// Define o tempo de início e final da aplicação no servidor
	serverApps.Start (Seconds (1.0));
	serverApps.Stop (Seconds (10.0));

	// Configura o endereço IP do servidor e a porta no cliente da aplicação de Echo
	Ipv4Address serverAddress = i1i2.GetAddress (1);
	Ipv4Address serverAddress2 = i2i3.GetAddress (0);

	UdpEchoClientHelper echoClient1 (serverAddress, 9);
	UdpEchoClientHelper echoClient2 (serverAddress2, 9);

	// Roteamento estático
	Ptr<Ipv4> ipv40 = nodes.Get(0)->GetObject<Ipv4> ();
	Ptr<Ipv4> ipv41 = nodes.Get(1)->GetObject<Ipv4> ();
	Ptr<Ipv4> ipv42 = nodes.Get(2)->GetObject<Ipv4> ();
	Ptr<Ipv4> ipv43 = nodes.Get(3)->GetObject<Ipv4> ();

	Ipv4StaticRoutingHelper ipv4RoutingHelper;

	//Primeira rota, indo por cima: n0 -> n1 -> n2
	Ptr<Ipv4StaticRouting> staticRouting012 = ipv4RoutingHelper.GetStaticRouting (ipv40);
	staticRouting012->AddHostRouteTo (serverAddress, i0i1.GetAddress(1), 1);

	//Segunda rota, indo por baixo: n0 -> n3 -> n2
	Ptr<Ipv4StaticRouting> staticRouting032 = ipv4RoutingHelper.GetStaticRouting (ipv40);
	staticRouting032->AddHostRouteTo (serverAddress2, i3i0.GetAddress(0), 2);

	// Volta da primeira rota, indo por cima:  n0 <- n1 <- n2
	Ptr<Ipv4StaticRouting> staticRouting210 = ipv4RoutingHelper.GetStaticRouting (ipv42);
	staticRouting210->AddHostRouteTo (i0i1.GetAddress(0), i1i2.GetAddress(0), 1);

	// Volta da segunda rota, indo por baixo: n0 <- n3 <- n2
	Ptr<Ipv4StaticRouting> staticRouting230 = ipv4RoutingHelper.GetStaticRouting (ipv42);
	staticRouting230->AddHostRouteTo (i3i0.GetAddress(1), i2i3.GetAddress(1), 2);

	// Configura o número máximo de pacotes enviados pelo cliente
	echoClient1.SetAttribute ("MaxPackets", UintegerValue (1));
	echoClient2.SetAttribute ("MaxPackets", UintegerValue (1));

	// Configura o intervalo de tempo entre pacotes no cliente
	echoClient1.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
	echoClient2.SetAttribute ("Interval", TimeValue (Seconds (1.0)));

	// Configura o tamanho máximo do pacote no cliente
	echoClient1.SetAttribute ("PacketSize", UintegerValue (1024));
	echoClient2.SetAttribute ("PacketSize", UintegerValue (1024));

	// Configura o cliente da aplicação no primeiro nó
	ApplicationContainer clientApps1 = echoClient1.Install (n0n1.Get(0));
	ApplicationContainer clientApps2 = echoClient2.Install (n0n1.Get(0));


	// Define o tempo de início e final da aplicação no cliente
	clientApps1.Start (Seconds (2.0));
	clientApps1.Stop (Seconds (10.0));
	clientApps2.Start (Seconds (2.0));
	clientApps2.Stop (Seconds (10.0));

	// Rastreamento ASCII
	AsciiTraceHelper ascii;
	pTP01.EnableAsciiAll (ascii.CreateFileStream (saidaAscii+".tr"));
	pTP01.EnablePcapAll (saidaAscii);
	pTP12.EnableAsciiAll (ascii.CreateFileStream (saidaAscii+".tr"));
	pTP12.EnablePcapAll (saidaAscii);
	pTP23.EnableAsciiAll (ascii.CreateFileStream (saidaAscii+".tr"));
	pTP23.EnablePcapAll (saidaAscii);
	pTP30.EnableAsciiAll (ascii.CreateFileStream (saidaAscii+".tr"));
	pTP30.EnablePcapAll (saidaAscii);

	// Carrega a simulação após todas as definições
	Simulator::Run ();

	// Encerra a simulação no final
	Simulator::Destroy ();
	return 0;
}
