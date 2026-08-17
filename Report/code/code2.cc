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