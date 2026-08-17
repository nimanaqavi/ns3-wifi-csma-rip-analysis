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