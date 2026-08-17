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