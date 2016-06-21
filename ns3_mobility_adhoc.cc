
/*	Autor: Jorge Guilherme Silva dos Santos
	Engenharia de Redes de Comunicação
	Universidade de Brasília

	Baseado em códigos de exemplo do NS3:
		manet-routing-compare.cc
		wifi-simple-adhoc.cc
*/



#include <fstream>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/applications-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace ns3;

uint32_t sentPackets = 0;
uint32_t rcvdPackets = 0;
// Variáveis que serão usados pelos argumentos de linha de comando
std::string CSVfileName = "e6.csv";
bool traceMobility = false;
int protocol = 2;
int nSinks = 4;
double txp = 7.5;
int nWifis = 15;
double TotalTime = 100.0;
std::string phyMode ("DsssRate11Mbps");
std::string tr_name ("e6");
double nodeSpeed = 20.0; //in m/s
int nodePause = 0; //in s
std::string protocolName = "AODV";		
int packetSize = 1000; // em bytes
int numPackets = 3100; // em numero de pacotes
Time interPacketInterval = Seconds (0.032);	// Intervalo entre pacotes
DelayJitterEstimation delayEst;
double acumulatedDelay = 0.0;


static void writeCounts () {
	acumulatedDelay = acumulatedDelay / rcvdPackets;
	// Escreve no arquivo CSV
	std::ofstream out (CSVfileName.c_str (), std::ios::app);		// Abre arquivo
	out << (Simulator::Now ()).GetSeconds () << " " <<
	sentPackets << " " <<
	rcvdPackets << " " <<
	nodeSpeed << " " <<
	nodePause << " " <<
	nSinks << " " <<
	protocolName << " " <<
	acumulatedDelay << std::endl;
	out.close ();																// Fecha arquivo
}

static void writeOnScreen() {
	std::cout << (Simulator::Now ()).GetSeconds () << " " <<
	sentPackets << " " <<
	rcvdPackets << " " <<
	nSinks << " " <<
	protocolName << std::endl;
}

// Aproveitado de: wifi-simples-adhoc.cc
// Basicamente, gera tráfego e contabilizao que acontece na recepção
// As modificações feitas no arquivo original estão comentadas em suas respectivas linhas
void ReceivePacket (Ptr<Socket> socket)
{
	rcvdPackets++;				// Linha adicionada
	Ptr<Packet> packet;
	while (packet = socket->Recv ())
	{
	    //NS_LOG_UNCOND ("Received one packet!");
	    delayEst.RecordRx(packet);
	    acumulatedDelay += delayEst.GetLastDelay().GetSeconds();
	}
	//DEBUG
	//writeCounts();
	//writeOnScreen();
	
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize, uint32_t pktCount, Time pktInterval )
	
{
	Ptr<Packet> packet;
	if (pktCount > 0)
	{	
	    packet = Create<Packet>(packetSize);
	    delayEst.PrepareTx (packet);
	    socket->Send (packet);
	    Simulator::Schedule (pktInterval, &GenerateTraffic, socket, pktSize,pktCount-1, pktInterval);
	    sentPackets++;		//Linha adicionada
	    //DEBUG
	    //std::cout << (Simulator::Now ()).GetSeconds () << "Enviado: sentPackets = " << sentPackets << std::endl;
	}
	else
	{
	    socket->Close ();
	}
}



int main (int argc, char *argv[])
{

	// Argumentos de linha de comando
	CommandLine cmd;
	cmd.AddValue ("CSVfileName", "Nome do arquivo CSV de saida [e6.csv]", CSVfileName);
	cmd.AddValue ("traceMobility", "Se habilita ou nao rastreio de mobilidade [false]", traceMobility);
	cmd.AddValue ("protocol", "1=OLSR;2=AODV;3=DSDV;4=DSR [2]", protocol);
	cmd.AddValue ("nWifis", "Numero de nos Wi-Fi na rede [15]", nWifis);
	cmd.AddValue ("nodeSpeed", "Velocidade dos nos em m/s [20.0]", nodeSpeed);
	cmd.AddValue ("nodePause", "Tempo de pausa dos nos em s [0]", nodePause);
	cmd.AddValue ("numPackets", "Numero de pacotes transmitidos por cada no [3100]", numPackets);
	cmd.AddValue ("packetSize", "Tamanho dos pacotes em bytes [1000]", packetSize);
	cmd.Parse (argc, argv);

	/* Trecho aproveitado de manet-routing-compare.cc */
	// Foram feitas modificações pontuais para adaptar ao estilo dessa simulação

	// Gera o início do arquivo CSV
	std::ofstream out (CSVfileName.c_str (), std::ios::app);		// Abre arquivo
	out << "SimulationSecond " <<
	"PacketsSent " <<
	"PacketsReceived " <<
	"NodeSpeed " <<
	"NodePause " <<
	"NumberOfSinks " <<
	"RoutingProtocol " <<
	std::endl;
	out.close ();																// Fecha arquivo

	//Set Non-unicastMode rate to unicast mode
	Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));

	// Cria container com os nós
	NodeContainer adhocNodes;
	adhocNodes.Create (nWifis);

	// setting up wifi phy and channel using helpers
	WifiHelper wifi;
	wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

	// Cria o canal Wi-Fi e define o modelo de perda de propagação de Friis
	YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
	YansWifiChannelHelper wifiChannel;
	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
	wifiPhy.SetChannel (wifiChannel.Create ());

	// Add a non-QoS upper mac, and disable rate control
	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
								"DataMode",StringValue (phyMode),
								"ControlMode",StringValue (phyMode));

	wifiPhy.Set ("TxPowerStart",DoubleValue (txp));
	wifiPhy.Set ("TxPowerEnd", DoubleValue (txp));

	wifiMac.SetType ("ns3::AdhocWifiMac");
	NetDeviceContainer adhocDevices = wifi.Install (wifiPhy, wifiMac, adhocNodes);

	// Mobilidade: BRUXARIA
	MobilityHelper mobilityAdhoc;
	int64_t streamIndex = 0; // used to get consistent mobility across scenarios

	ObjectFactory pos;
	pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
	pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=500.0]"));
	pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=500.0]"));

	Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
	streamIndex += taPositionAlloc->AssignStreams (streamIndex);

	std::stringstream ssSpeed;
	ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << nodeSpeed << "]";
	std::stringstream ssPause;
	ssPause << "ns3::ConstantRandomVariable[Constant=" << nodePause << "]";
	mobilityAdhoc.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
									"Speed", StringValue (ssSpeed.str ()),
									"Pause", StringValue (ssPause.str ()),
									"PositionAllocator", PointerValue (taPositionAlloc));
	mobilityAdhoc.SetPositionAllocator (taPositionAlloc);
	mobilityAdhoc.Install (adhocNodes);
	streamIndex += mobilityAdhoc.AssignStreams (adhocNodes, streamIndex);

	AodvHelper aodv;
	OlsrHelper olsr;
	DsdvHelper dsdv;
	DsrHelper dsr;
	DsrMainHelper dsrMain;
	Ipv4ListRoutingHelper list;
	InternetStackHelper internet;

	switch (protocol)
	{
	case 1:
		list.Add (olsr, 100);
		protocolName = "OLSR";
		break;
	case 2:
		list.Add (aodv, 100);
		protocolName = "AODV";
		break;
	case 3:
		list.Add (dsdv, 100);
		protocolName = "DSDV";
		break;
	case 4:
		protocolName = "DSR";
		break;
	default:
		NS_FATAL_ERROR ("No such protocol:" << protocol);
	}

	if (protocol < 4)
	{
		internet.SetRoutingHelper (list);
		internet.Install (adhocNodes);
	}
	else if (protocol == 4)
	{
		internet.Install (adhocNodes);
		dsrMain.Install (dsr, adhocNodes);
	}

	Ipv4AddressHelper addressAdhoc;
	addressAdhoc.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer adhocInterfaces;
	adhocInterfaces = addressAdhoc.Assign (adhocDevices);

	/* Fim do trecho aproveitado de manet-routing-compare.cc */

	// Início da criação das "aplicações"
	// que na verdade são apenas uma função geradora de tráfego

	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

	for (int i = 0; i < nSinks; i++)
	{
		/* Início do código aproveitado de wifi-simples-adhoc.cc */
		// Nós: de 0 a nSinks - 1 são os sinks enquanto os nós de nSinks até 2*nSinks - 1 são os geradores de tráfego (sources)

		//// Criação dos sinks
		Ptr<Socket> recvSink = Socket::CreateSocket (adhocNodes.Get (i), tid);
		InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
		recvSink->Bind (local);
		recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

		//// Criação dos sources (geradores de tráfego)
		Ptr<Socket> source = Socket::CreateSocket (adhocNodes.Get (i + nSinks), tid);
		InetSocketAddress remote = InetSocketAddress (adhocInterfaces.GetAddress(i,0), 80);
		//InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80);
		source->SetAllowBroadcast (true);
		source->Connect (remote);

			//// Agenda cada uma das fontes de tráfego para iniciar a transmissão
			Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
									Seconds (0.0), &GenerateTraffic, 
									source, packetSize, numPackets, interPacketInterval);

			/* Fim do código aproveitado de wifi-simples-adhoc.cc */
	}

	// Habilita geração de arquivos PCAP
	//// vou tentar usar isso para calcular o delay
	wifiPhy.EnablePcap ("e6", adhocDevices);

	// So para não deixar a simulacao ocorrer as cegas
	Simulator::Schedule(Seconds(20.0), &writeOnScreen);
	Simulator::Schedule(Seconds(40.0), &writeOnScreen);
	Simulator::Schedule(Seconds(60.0), &writeOnScreen);
	Simulator::Schedule(Seconds(80.0), &writeOnScreen);
	Simulator::Schedule(Seconds(100.0), &writeOnScreen);
	Simulator::Schedule(Seconds(120.0), &writeOnScreen);
	Simulator::Schedule(Seconds(120.0), &writeCounts);

	Simulator::Stop (Seconds (120.0));
	Simulator::Run ();
	Simulator::Destroy ();

	return 0;
}




