#include "Core.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Core");

Core::Core(std::string configFile)
{
    this->m_configFile = std::move(configFile);
}

void
Core::runSimulation()
{
    NS_LOG_INFO("Reading config file: " << this->m_configFile);
    this->readConfigFile();
    this->m_netSetup = std::make_unique<NetSetup>(this->m_config);
    NS_LOG_INFO("Setup logging");
    this->setupLogging();
    NS_LOG_INFO("Reading Simulation settings");
    this->readSimulationSettings();
    NS_LOG_INFO("Starting Simulation");
    this->run();
    NS_LOG_INFO("Simulation completed");
}

void
Core::readConfigFile()
{
    this->m_config = toml::parse(this->m_configFile);
}

toml_value
Core::findTable(const std::string& tableName) const
{
    return toml::find<toml_value>(this->m_config, tableName);
}

void
Core::readSimulationSettings()
{
    toml_value simSettings = this->findTable(CONST_COLUMNS::c_simSettings);
    this->m_stopTime = MilliSeconds(toml::find<uint64_t>(simSettings, CONST_COLUMNS::c_stopTime));
    this->m_streamTime =
        MilliSeconds(toml::find<uint64_t>(simSettings, CONST_COLUMNS::c_streamTime));
    NS_LOG_DEBUG("Stop time: " << this->m_stopTime.GetMilliSeconds());
    NS_LOG_DEBUG("Stream time: " << this->m_streamTime.GetMilliSeconds());
}

void
Core::createVehicleNodes()
{
    toml_value vehicleSettings = this->findTable(CONST_COLUMNS::c_vehicleSettings);
    std::string activationFile = toml::find<std::string>(vehicleSettings, CONST_COLUMNS::c_activationFile);

    NS_LOG_DEBUG("Reading Activation file: " << activationFile);
    ActivationReader activationReader(activationFile);
    this->m_vehicleActivationTimes = activationReader.getActivationTimes();

    this->m_numVehicles = this->m_vehicleActivationTimes.size();
    NS_LOG_DEBUG("Vehicle size: " << this->m_numVehicles);

    this->m_vehicleNodes.Create(this->m_numVehicles);
    for (uint32_t i = 0; i < this->m_vehicleNodes.GetN(); i++)
    {
        NS_LOG_DEBUG("Vehicle Node ID: " << this->m_vehicleNodes.Get(i)->GetId());
    }
    this->m_allNodes.Add(this->m_vehicleNodes);
    this->setupVehicleMobility(vehicleSettings);
}

void
Core::setupVehicleMobility(toml_value vehicleSettings)
{
    std::string traceFile = toml::find<std::string>(vehicleSettings, CONST_COLUMNS::c_traceFile);
    NS_LOG_DEBUG("Reading Vehicle trace file: " << traceFile);
    this->m_vehicleMobility = std::make_unique<TraceMobility>(traceFile);

    for (uint32_t i = 0; i < this->m_vehicleNodes.GetN(); ++i)
    {
        Ptr<Node> node = this->m_vehicleNodes.Get(i);
        MobilityHelper mobilityHelper;
        mobilityHelper.SetMobilityModel("ns3::WaypointMobilityModel");
        mobilityHelper.Install(node);
    }
    this->scheduleMobilityUpdate();
}

void
Core::scheduleMobilityUpdate()
{
    Time currentSimTime = std::max(Time(0), Simulator::Now());
    this->m_vehicleMobility->addWaypointsBetween(currentSimTime,
                                                 currentSimTime + this->m_streamTime,
                                                 this->m_vehicleNodes);

    NS_LOG_DEBUG("Scheduling mobility update at " << currentSimTime + this->m_streamTime);
    Simulator::Schedule(currentSimTime + this->m_streamTime, &Core::scheduleMobilityUpdate, this);
}

void
Core::createRsuNodes()
{
    toml_value rsuSettings = this->findTable(CONST_COLUMNS::c_rsuSettings);
    std::string activationFile = toml::find<std::string>(rsuSettings, CONST_COLUMNS::c_activationFile);

    NS_LOG_DEBUG("Reading Activation file: " << activationFile);
    ActivationReader activationReader(activationFile);
    this->m_rsuActivationTimes = activationReader.getActivationTimes();
    this->m_rsuIdMap = activationReader.getNodeIdMap();

    this->m_numRSUs = this->m_rsuActivationTimes.size();
    NS_LOG_DEBUG("RSU size: " << this->m_numRSUs);

    this->m_rsuNodes.Create(this->m_numRSUs);
    for (auto iter = this->m_rsuNodes.Begin(); iter != this->m_rsuNodes.End(); ++iter)
    {
        NS_LOG_DEBUG("RSU Node ID: " << (*iter)->GetId());
    }
    this->setupRSUPositions(rsuSettings);
    this->m_allNodes.Add(this->m_rsuNodes);
}

void
Core::setupRSUPositions(toml_value rsuSettings)
{
    std::string positionFile = toml::find<std::string>(rsuSettings, CONST_COLUMNS::c_positionFile);
    NS_LOG_DEBUG("Reading Position file: " << positionFile);
    PositionReader positionReader(positionFile);
    position_map_t positionData = positionReader.getPositionData();

    for (auto iter = this->m_rsuNodes.Begin(); iter != this->m_rsuNodes.End(); ++iter)
    {
        Ptr<Node> node = *iter;
        MobilityHelper mobilityHelper;
        mobilityHelper.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobilityHelper.Install(node);
        Ptr<ConstantPositionMobilityModel> mobility =
            node->GetObject<ConstantPositionMobilityModel>();

        auto position = positionData.find(node->GetId());
        if (position != positionData.end())
        {
            mobility->SetPosition(Vector(position->second.first, position->second.second, 0));
        }
        else
        {
            NS_ASSERT_MSG(false, "No position data found for RSU: " << node->GetId());
        }
    }
}

void
Core::setupLogging() const
{
    toml_value log_settings = this->findTable(CONST_COLUMNS::c_logSettings);
    std::vector<std::string> log_components =
        toml::find<std::vector<std::string>>(log_settings, c_logComponents);
    std::string logLevel = toml::find<std::string>(log_settings, c_logLevel);
    LogLevel nsLogLevel = Core::getLogLevel(logLevel);

    for (const std::string& log_component : log_components)
    {
        std::string current_env = getenv("NS_LOG");
        std::string new_env =
            current_env.append(":").append(log_component).append("=").append(logLevel);
        // setenv("NS_LOG", new_env.c_str(), true);
        LogComponentEnable(log_component.c_str(), nsLogLevel);
    }
}

LogLevel
Core::getLogLevel(std::string logLevel)
{
    std::transform(logLevel.begin(), logLevel.end(), logLevel.begin(), ::toupper);
    if (logLevel == "DEBUG")
    {
        return LOG_LEVEL_DEBUG;
    }
    else if (logLevel == "INFO")
    {
        return LOG_LEVEL_INFO;
    }
    else if (logLevel == "WARN")
    {
        return LOG_LEVEL_WARN;
    }
    else if (logLevel == "ERROR")
    {
        return LOG_LEVEL_ERROR;
    }
    else if (logLevel == "LOGIC")
    {
        return LOG_LEVEL_LOGIC;
    }
    else
    {
        return LOG_LEVEL_ALL;
    }
}

void
Core::run()
{
    NS_LOG_INFO("Create Vehicle nodes");
    this->createVehicleNodes();
    NS_LOG_INFO("Create RSU nodes");
    this->createRsuNodes();
    NS_LOG_INFO("Setting up NR module for all Nodes.");
    toml_value networkSettings = this->findTable(CONST_COLUMNS::c_netSettings);

    Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper>();
    Ptr<IdealBeamformingHelper> beamHelper = CreateObject<IdealBeamformingHelper>();
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();
    nrHelper->SetBeamformingHelper(beamHelper);
    NS_LOG_INFO("Setting up EPC...");
    nrHelper->SetEpcHelper(epcHelper);

    NS_LOG_DEBUG("Configuring operation band...");
    OperationBandInfo operationBandInfo =
        this->m_netSetup->updateNRHelperAndBuildOpBandInfo(networkSettings, nrHelper);
    BandwidthPartInfoPtrVector bwpInfoVector = CcBwpCreator::GetAllBwps({operationBandInfo});

    NS_LOG_INFO("Configuring Antenna...");
    nrHelper = this->m_netSetup->configureAntenna(nrHelper);

    NS_LOG_INFO("Configuring MAC and PHY...");
    nrHelper = this->m_netSetup->configureMacAndPhy(nrHelper);

    uint8_t bwpIdForGbrMcptt = 0;
    nrHelper->SetBwpManagerTypeId(TypeId::LookupByName("ns3::NrSlBwpManagerUe"));
    nrHelper->SetUeBwpManagerAlgorithmAttribute("GBR_MC_PUSH_TO_TALK",
                                                UintegerValue(bwpIdForGbrMcptt));
    std::set<uint8_t> bwpIds;
    bwpIds.insert(bwpIdForGbrMcptt);

    this->m_allDevices = nrHelper->InstallUeDevice(this->m_allNodes, bwpInfoVector);

    for (auto it = this->m_allDevices.Begin(); it != this->m_allDevices.End(); ++it)
    {
        DynamicCast<NrUeNetDevice>(*it)->UpdateConfig(); // required as per docs
    }

    NS_LOG_INFO("Configuring sidelink...");
    Ptr<NrSlHelper> slHelper = this->m_netSetup->configureSideLink();
    slHelper->SetEpcHelper(epcHelper);
    slHelper->PrepareUeForSidelink(this->m_allDevices, bwpIds);

    NS_LOG_INFO("Configuring sidelink resource pool...");
    LteRrcSap::SlBwpPoolConfigCommonNr slResourcePool = this->m_netSetup->configureResourcePool();

    NS_LOG_INFO("Configure sidelink bandwidth part...");
    LteRrcSap::SlFreqConfigCommonNr freqConfig =
        this->m_netSetup->configureBandwidth(slResourcePool, bwpIds);

    NS_LOG_INFO("Configure other sidelink settings...");
    LteRrcSap::SidelinkPreconfigNr sideLinkPreconfigNr =
        this->m_netSetup->configureSidelinkPreConfig();
    sideLinkPreconfigNr.slPreconfigFreqInfoList[0] = freqConfig;

    slHelper->InstallNrSlPreConfiguration(this->m_allDevices, sideLinkPreconfigNr);

    int64_t stream = 1;
    stream += nrHelper->AssignStreams(this->m_allDevices, stream);
    stream += slHelper->AssignStreams(this->m_allDevices, stream);

    NS_LOG_INFO("Configure Internet Stack...");

    InternetStackHelper internet;
    internet.Install(this->m_allNodes);

    NS_LOG_INFO("Separating devices into vehicle and RSU devices...");
    this->segregateNetDevices();

    uint32_t dstL2Id = 225;
    Ipv4InterfaceContainer ueIpIface;

    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.1.0.0", "255.255.0.0");
    NS_LOG_INFO("Assigning IP addresses to vehicles...");
    Ipv4InterfaceContainer vehContainer = ipv4.Assign (this->m_vehicleDevices);
    ipv4.SetBase ("10.2.0.0", "255.255.0.0");
    Ipv4InterfaceContainer rsuContainer = ipv4.Assign (this->m_rsuDevices);

    Ptr<LteSlTft> tft;

    NS_LOG_INFO("Reading output settings");
    toml_value outputSettings = this->findTable(CONST_COLUMNS::c_outputSettings);
    std::string outputPath = toml::find<std::string>(outputSettings, CONST_COLUMNS::c_outputPath);
    std::string outputName = toml::find<std::string>(outputSettings, CONST_COLUMNS::c_outputName);
    SQLiteOutput db(outputPath + outputName + ".db");
    Outputter outputter;

    NS_LOG_INFO("Reading V2R links");
    toml_value vehicleSettings = this->findTable(CONST_COLUMNS::c_vehicleSettings);
    std::string v2rLinkFile = toml::find<std::string>(vehicleSettings, CONST_COLUMNS::c_v2rLinkFile);
    auto linkReader = LinkReader(v2rLinkFile);
    auto linkMap = linkReader.getLinks();

    NS_LOG_INFO("Setup Tx applications");
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable>();
    startTimeSeconds->SetStream(1);
    startTimeSeconds->SetAttribute("Min", DoubleValue(0));
    startTimeSeconds->SetAttribute("Max", DoubleValue(0.05));
    std::string dataRateStr = toml::find<std::string>(networkSettings, netParameters::n_dataRate);
    int64_t packetSize = toml::find<int64_t>(networkSettings, netParameters::n_packetSize);

    ApplicationContainer vehicleApps;

    UeToUePktTxRxOutputStats pktStats;
    pktStats.SetDb(&db, "pktTxRx");

    for (uint32_t i = 0; i < this->m_vehicleDevices.GetN(); i++){
        Ptr<NetDevice> vehDevice = this->m_vehicleDevices.Get(i);
        Ptr<Node> vehNode = vehDevice->GetNode();
        Ptr<Ipv4> vehIpv4 = vehNode->GetObject<Ipv4>();
        Ipv4Address vehAddr = vehIpv4->GetAddress(1, 0).GetLocal();
        tft = Create<LteSlTft>(LteSlTft::Direction::TRANSMIT,
                               LteSlTft::CommType::UniCast,
                               vehAddr,
                               dstL2Id);
        slHelper->ActivateNrSlBearerForDevice(Time::From(0), vehDevice, tft);

        NS_LOG_DEBUG("Vehicle ID: " << vehNode->GetId() << " Vehicle Addr: " << vehAddr);

        Time lastStopTime = this->m_vehicleActivationTimes[vehNode->GetId()].second;
        ApplicationContainer vehApps;
        Ptr<Ipv4StaticRouting> vehicleRouting = ipv4RoutingHelper.GetStaticRouting(vehNode->GetObject<Ipv4>());

        auto link = linkMap.find(vehNode->GetId());
        if (link != linkMap.end())
        {
            for (uint32_t ii = 0; ii < link->second.size(); ++ii)
            {
                auto linkData = link->second[ii];
                Time startTime = linkData.first;
                Time stopTime = ii == link->second.size() - 1 ? lastStopTime : link->second[ii + 1].first;

                uint32_t rsuId = linkData.second;
                rsuId = rsuId - this->m_numVehicles;
                Ptr<NetDevice> rsuDevice = this->m_rsuDevices.Get(rsuId);
                Ptr<Node> rsuNode = rsuDevice->GetNode();
                Ptr<Ipv4> rsuIpv4 = rsuNode->GetObject<Ipv4>();
                Ipv4Address rsuAddr = rsuIpv4->GetAddress(1, 0).GetLocal();
                NS_LOG_DEBUG("RSU ID: " << rsuId << " RSU Addr: " << rsuAddr);

                // Create application
                Address rsuSocketAddress = InetSocketAddress(rsuAddr, 8000);
                OnOffHelper sidelinkClient("ns3::UdpSocketFactory", rsuSocketAddress);
                sidelinkClient.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));
                sidelinkClient.SetConstantRate(DataRate(dataRateStr), packetSize);
                vehApps.Add(sidelinkClient.Install(vehNode));

                // Set start and stop time
                double randomStart = startTimeSeconds->GetValue();
                vehApps.Get(ii)->SetStartTime(startTime + Seconds(randomStart));
                vehApps.Get(ii)->SetStopTime(stopTime);
                NS_LOG_DEBUG("Start time: " << startTime.GetMilliSeconds() << " Stop time: " << stopTime.GetMilliSeconds());

                vehicleRouting->AddHostRouteTo(rsuAddr, 1);
                vehApps.Get(ii)->TraceConnect("TxWithSeqTsSize",
                                             "tx",
                                             MakeBoundCallback(&Outputter::UePacketTraceDb, &pktStats, vehNode, vehAddr));
            }
        }
        vehicleApps.Add(vehApps);
    }

    NS_LOG_INFO("Setup Rx applications");
    ApplicationContainer rsuApps;
    for (uint32_t i = 0; i < this->m_rsuDevices.GetN(); i++)
    {
        Ptr<NetDevice> rsuDevice = this->m_rsuDevices.Get(i);
        Ptr<Node> rsuNode = rsuDevice->GetNode();
        Ptr<Ipv4> rsuIpv4 = rsuNode->GetObject<Ipv4>();
        rsuIpv4->SetForwarding(1, false);
        Ipv4Address rsuAddr = rsuIpv4->GetAddress(1, 0).GetLocal();

        tft = Create<LteSlTft>(LteSlTft::Direction::BIDIRECTIONAL,
                               LteSlTft::CommType::UniCast,
                               rsuAddr,
                               dstL2Id);
        slHelper->ActivateNrSlBearerForDevice(Time::From(0.01), rsuDevice, tft);

        InetSocketAddress remoteAddr = InetSocketAddress(rsuAddr, 8000);
        PacketSinkHelper sidelinkSink = PacketSinkHelper("ns3::UdpSocketFactory", remoteAddr);
        sidelinkSink.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));

        rsuApps.Add(sidelinkSink.Install(rsuNode));
        rsuApps.Get(i)->SetStartTime(Seconds(0.01));
        rsuApps.Get(i)->TraceConnect("RxWithSeqTsSize",
                                      "rx",
                                      MakeBoundCallback(&Outputter::UePacketTraceDb,
                                                        &pktStats,
                                                        rsuNode,
                                                        rsuAddr));
    }

    NS_LOG_DEBUG("All nodes size: " << this->m_allNodes.GetN());

    NS_LOG_DEBUG("Outputter: Connecting to SlPscchScheduling trace source");
    UeMacPscchTxOutputStats pscchStats;
    pscchStats.SetDb(&db, "pscchTxUeMac");
    Config::ConnectWithoutContext(
        "/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/"
        "ComponentCarrierMapUe/*/NrUeMac/SlPscchScheduling",
        MakeBoundCallback(&Outputter::NotifySlPscchScheduling, &pscchStats));

    NS_LOG_DEBUG("Outputter: Connecting to SlPsschScheduling trace source");
    UeMacPsschTxOutputStats psschStats;
    psschStats.SetDb(&db, "psschTxUeMac");
    Config::ConnectWithoutContext(
        "/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/"
        "ComponentCarrierMapUe/*/NrUeMac/SlPsschScheduling",
        MakeBoundCallback(&Outputter::NotifySlPsschScheduling, &psschStats));

    NS_LOG_DEBUG("Outputter: Connecting to RxPscchTraceUe trace source");
    UePhyPscchRxOutputStats pscchPhyStats;
    pscchPhyStats.SetDb(&db, "pscchRxUePhy");
    Config::ConnectWithoutContext(
        "/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUePhy/"
        "NrSpectrumPhyList/*/RxPscchTraceUe",
        MakeBoundCallback(&Outputter::NotifySlPscchRx, &pscchPhyStats));

    NS_LOG_DEBUG("Outputter: Connecting to RxPsschTraceUe trace source");
    UePhyPsschRxOutputStats psschPhyStats;
    psschPhyStats.SetDb(&db, "psschRxUePhy");
    Config::ConnectWithoutContext(
        "/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUePhy/"
        "NrSpectrumPhyList/*/RxPsschTraceUe",
        MakeBoundCallback(&Outputter::NotifySlPsschRx, &psschPhyStats));

    NS_LOG_DEBUG("Outputter: Connecting to RxRlcPduWithTxRnti trace source");
    UeRlcRxOutputStats ueRlcRxStats;
    ueRlcRxStats.SetDb(&db, "rlcRx");
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/"
                                  "ComponentCarrierMapUe/*/NrUeMac/RxRlcPduWithTxRnti",
                                  MakeBoundCallback(&Outputter::NotifySlRlcPduRx, &ueRlcRxStats));

    NS_LOG_DEBUG("Outputter: Connecting to V2xKpi trace source");
    V2xKpi v2xKpi;
    std::string v2xKpiPath = outputPath + outputName;
    v2xKpi.SetDbPath(v2xKpiPath);
    v2xKpi.SetTxAppDuration(this->m_stopTime.GetSeconds());
    Outputter::SavePositionPerIP(&v2xKpi);
    v2xKpi.SetRangeForV2xKpis(200);

    std::string mobilityFileName = outputPath + "mobility-ues.txt";
    NS_LOG_DEBUG("Outputter: Connecting to Mobility trace source");
    Outputter::RecordMobility(true, mobilityFileName);

    NS_LOG_INFO("Running the simulation...");

    Simulator::Stop(this->m_stopTime);
    Simulator::Run();

    pscchStats.EmptyCache();
    pktStats.EmptyCache();
    psschStats.EmptyCache();
    pscchPhyStats.EmptyCache();
    psschPhyStats.EmptyCache();
    ueRlcRxStats.EmptyCache();
    v2xKpi.WriteKpis();
    Simulator::Destroy();
}

void
Core::segregateNetDevices()
{
    for (auto it = this->m_allDevices.Begin(); it != this->m_allDevices.End(); ++it)
    {
        Ptr<NetDevice> device = *it;
        // If not vehicle node, then it is an RSU node
        if (device->GetNode()->GetId() < this->m_numVehicles)
        {
            m_vehicleDevices.Add(device);
        }
        else
        {
            m_rsuDevices.Add(device);
        }
    }
    NS_LOG_DEBUG("Vehicle devices size: " << m_vehicleDevices.GetN());
    NS_LOG_DEBUG("RSU devices size: " << m_rsuDevices.GetN());
}
