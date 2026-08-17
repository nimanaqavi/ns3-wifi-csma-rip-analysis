#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-apps-module.h"

#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiRipProject");

int main(int argc, char* argv[])
{
    uint32_t nNodes = 5;                 
    std::string dataRate = "500kbps";    
    std::string scenario = "baseline";   
    double simTime = 80.0;               
    uint32_t packetSize = 1024;          
    double gridDelta = 120.0;            
    bool verbose = false;                

    CommandLine cmd;
    cmd.AddValue("nNodes", "Number of nodes in the topology", nNodes);
    cmd.AddValue("dataRate", "Per-flow UDP data rate", dataRate);
    cmd.AddValue("scenario", "Scenario label: baseline | heavy | scale", scenario);
    cmd.AddValue("simTime", "Total simulation time in seconds", simTime);
    cmd.AddValue("packetSize", "UDP packet payload size in bytes", packetSize);
    cmd.AddValue("gridDelta", "Grid spacing between nodes in meters", gridDelta);
    cmd.AddValue("verbose", "Enable verbose logging", verbose);
    cmd.Parse(argc, argv);

    if (nNodes < 2) return 1;
    if (verbose) LogComponentEnable("WifiRipProject", LOG_LEVEL_INFO);

    NodeContainer nodes;
    nodes.Create(nNodes);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode", StringValue("DsssRate11Mbps"),
                                 "ControlMode", StringValue("DsssRate1Mbps"));

    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());

    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);

    uint32_t gridWidth = static_cast<uint32_t>(std::ceil(std::sqrt(static_cast<double>(nNodes))));
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue(0.0),
                                   "MinY", DoubleValue(0.0),
                                   "DeltaX", DoubleValue(gridDelta),
                                   "DeltaY", DoubleValue(gridDelta),
                                   "GridWidth", UintegerValue(gridWidth),
                                   "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    RipHelper rip;
    Ipv4ListRoutingHelper listRouting;
    listRouting.Add(rip, 10);

    InternetStackHelper internet;
    internet.SetRoutingHelper(listRouting);
    internet.Install(nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    uint16_t port = 9;
    ApplicationContainer serverApps;
    ApplicationContainer clientApps;

    for (uint32_t i = 1; i < nNodes; ++i)
    {
        PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
        serverApps.Add(sink.Install(nodes.Get(i)));

        OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(interfaces.GetAddress(i), port));
        onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        onoff.SetAttribute("DataRate", DataRateValue(DataRate(dataRate)));
        onoff.SetAttribute("PacketSize", UintegerValue(packetSize));

        clientApps.Add(onoff.Install(nodes.Get(0)));
    }

    serverApps.Start(Seconds(1.0));
    clientApps.Start(Seconds(35.0)); 
    serverApps.Stop(Seconds(simTime));
    clientApps.Stop(Seconds(simTime));

    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    std::string sourceAddr = "10.1.1.1"; 

    std::cout << "\n--- Simulation Results (" << scenario << ", " << nNodes << " nodes, " << dataRate << ") ---\n";

    for (const auto& entry : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(entry.first);

        if (t.sourceAddress == Ipv4Address(sourceAddr.c_str()) && t.destinationPort == port)
        {
            const FlowMonitor::FlowStats& fs = entry.second;
            double pdr = (fs.txPackets > 0) ? (static_cast<double>(fs.rxPackets) / fs.txPackets) * 100.0 : 0.0;
            double duration = fs.timeLastRxPacket.GetSeconds() - fs.timeFirstTxPacket.GetSeconds();
            double throughput = (duration > 0) ? (fs.rxBytes * 8.0) / duration : 0.0;
            double avgDelay = (fs.rxPackets > 0) ? fs.delaySum.GetSeconds() / fs.rxPackets : 0.0;

            std::cout << "Flow " << entry.first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
            std::cout << "  Tx Packets: " << fs.txPackets << "\n";
            std::cout << "  Rx Packets: " << fs.rxPackets << "\n";
            std::cout << "  PDR:        " << pdr << " %\n";
            std::cout << "  Throughput: " << throughput / 1000.0 << " Kbps\n";
            std::cout << "  Avg Delay:  " << avgDelay * 1000.0 << " ms\n\n";
        }
    }

    Simulator::Destroy();
    return 0;
}