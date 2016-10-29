// Jorge Guilherme Silva dos Santos - 12/0014611
// Universidade de Brasília
// -------------------------------------------------------//
// CÓDIGO PARTE 1 (E e F)
// -------------------------------------------------------//
// Bibliotecas básicas
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"	//DumbbellHelper
#include "ns3/applications-module.h"	// UdpEchoServer
#include "ns3/stats-module.h"			//GnuPlot

using namespace ns3;

/*	   Topologia:

n2 -----+			  +----- n4
(udp)	|			  |		(null)
		|	          |
		n0 ---------- n1 
		|			  |
		|			  |
n3 -----+			  +----- n5
(tcp)						 (sink)


*/

int
main (int argc, char *argv[])
{
	// Variável para o número de nodes para cada lado da topologia
	uint32_t    nLeftLeaf  = 2;
	uint32_t    nRightLeaf = 2;

	// Variáveis que serão alteradas por linha de comando
	std::string rate_p2pM = "1.2Mbps";
	std::string delay_p2pM = "10ms";
	std::string rate_p2pL = "2Mbps";
	std::string delay_p2pL = "2ms";
	std::string tcp_version = "TcpTahoe";
	double tcp_start = 1.0;
	double tcp_stop = 4.0;
	double udp_start = 0.3;
	double udp_stop = 5.0;
	uint32_t maxpackets = 1;
	double interval = 1.0;
	uint32_t packetsize = 1024;

	// Parâmetros de linha de comando
    CommandLine cmd;
    cmd.AddValue ("rate_p2pM", "Taxa do enlace intermediario [1.2Mbps]", rate_p2pM);
    cmd.AddValue ("delay_p2pM", "Atraso do enlace intermediario [10ms]", delay_p2pM);
    cmd.AddValue ("rate_p2pL", "Taxa do enlace das folhas [2Mbps]", rate_p2pL);
    cmd.AddValue ("delay_p2pL", "Atraso do enlace das folhas [2ms]", delay_p2pL);
    cmd.AddValue ("tcp_start", "Tempo de inicio da aplicacao TCP [1.0]", tcp_start);
    cmd.AddValue ("tcp_stop", "Tempo de inicio da aplicacao TCP [1.0]", tcp_stop);
    cmd.AddValue ("udp_start", "Tempo de inicio da aplicacao TCP [1.0]", udp_start);
    cmd.AddValue ("udp_stop", "Tempo de inicio da aplicacao TCP [1.0]", udp_stop);
    cmd.AddValue ("maxpackets", "Máximo de pacotes UDP enviados [1]", maxpackets);
    cmd.AddValue ("interval", "Intervalo entre pacotes UDP sucessivos [1.0]", interval);
    cmd.AddValue ("packetsize", "Tamanho dos pacotes UDP [1024]", packetsize);
    cmd.Parse (argc,argv);

	// Definição dos enlaces
	//// Enlace central (gargalo) ou enlace do Meio
	PointToPointHelper p2pM;
	p2pM.SetDeviceAttribute  ("DataRate", StringValue (rate_p2pM));
	p2pM.SetChannelAttribute ("Delay", StringValue (delay_p2pM));

	//// Enlace das extremidades (folhas ou "leafs")
	PointToPointHelper p2pL;
	p2pL.SetDeviceAttribute ("DataRate", StringValue (rate_p2pL));
	p2pL.SetChannelAttribute   ("Delay", StringValue (delay_p2pL));

	// Efetivamente cria a topologia Dumbbell utilizando os enlaces que
	// foram definidos anteriormente
    PointToPointDumbbellHelper dumbbell (nLeftLeaf, p2pL,
                                         nRightLeaf, p2pL,
                                         p2pM);


    // Pilha de protocolo TCP/IP
    // (aka pilha de protocolos da Internet)
    InternetStackHelper stack;
    dumbbell.InstallStack (stack);

    // Endereçamento IP
    //// (utilizei faixas diferentes para deixar as coisas mais legais)
    dumbbell.AssignIpv4Addresses (Ipv4AddressHelper ("172.16.6.0", "255.255.255.0"),
                                  Ipv4AddressHelper ("192.168.1.0", "255.255.255.0"),
                                  Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"));

    // Algumas definições padrão
    uint16_t port = 50000;		// Porta do TCP Sink
    uint16_t udp_port = 9;
    GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::"+tcp_version));

	/* PARTE DE CONFIGURAÇÃO E INSTALAÇÃO DAS APLICAÇÕES */

    // TCP Sink
	Ptr <Node> NodeSink = dumbbell.GetRight(1);
	Ptr <Ipv4> ipv4 = NodeSink->GetObject <Ipv4> ();
	Ipv4Address sinkIp = ipv4->GetAddress (1,0).GetLocal ();
	Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
	PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", sinkAddress);
	ApplicationContainer sink_app = packetSinkHelper.Install (NodeSink);
    sink_app.Start ( Seconds (tcp_start) );
    sink_app.Stop ( Seconds (tcp_stop) ); 

	// Aolicação FTP
	//// Criação da Aplicação 
	BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
	AddressValue sinkAddressValue (InetSocketAddress (sinkIp, port));
	ftp.SetAttribute ("Remote", sinkAddressValue);

	//// Instala a aplicação no node
	ApplicationContainer sourceApp = ftp.Install (dumbbell.GetLeft(1));

	//// Define os tempos de início e fim do FTP
	sourceApp.Start (Seconds (tcp_start));
	sourceApp.Stop (Seconds (tcp_stop));

    // UDP Echo Server
    Ptr <Node> NodeUDPServer = dumbbell.GetRight(0);
    Ptr <Ipv4> ipv4Udp = NodeUDPServer->GetObject <Ipv4> ();
    Ipv4Address echoIp = ipv4Udp->GetAddress (1,0).GetLocal ();

	// Aplicação UDP
	//// De cara, sabemos que vamos usar aplicação UDP nessa primeira parte
	//// Então, habilita os logs
	LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
	LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

	//// Criação do servidor UDP e instalação do mesmo no nó N1
	//// Novamente, NÃO é um servidor de echo
	UdpServerHelper udpServer (udp_port);
	ApplicationContainer serverApps = udpServer.Install (NodeUDPServer);

	//// Define os tempos de início e fim da aplicação servidor UDP
	serverApps.Start (Seconds (udp_start));
	serverApps.Stop (Seconds (udp_stop));

	//// Criação do cliente UDP para utilização com servidor 
	UdpEchoClientHelper echoClient ( echoIp, udp_port );

	//// Configura o número máximo de pacotes enviados pelo cliente
	echoClient.SetAttribute ("MaxPackets", UintegerValue (maxpackets));
	//// Intervalo de tempo entre pacotes no cliente
	echoClient.SetAttribute ("Interval", TimeValue (Seconds (interval)));
	//// Tamanho máximo do pacote no cliente
	echoClient.SetAttribute ("PacketSize", UintegerValue (packetsize));

	//// Instala o cliente no nó n0.
	////// Importante: Esse statement deve vir após a criação e definição do echoClient
	ApplicationContainer clientApps = echoClient.Install (dumbbell.GetLeft(0));

	// Define os tempos de início e fim da aplicação cliente
	clientApps.Start (Seconds (udp_start));
	clientApps.Stop (Seconds (udp_stop));


	// Roteamento global automático
	////  Como se o N2 fosse um roteador executando OSPF
	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	// Habilita geração de arquivos PCAP
	p2pL.EnablePcapAll ("e6p1e/e6p1e");
	p2pM.EnablePcapAll("e6p1e/e6p1e");

	// Carrega a simulação após todas as definições
  	Simulator::Run ();
  	// Encerra a simulação no final
  	Simulator::Destroy ();
  	return 0;
}


