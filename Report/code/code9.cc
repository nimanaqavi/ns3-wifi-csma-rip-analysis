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