#include "Core.h"

#include <utility>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Core");

Core::Core(std::string configFile)
{
    this->m_configFile = std::move(configFile);
    this->m_groupCastAddr = Ipv4Address("225.0.0.0");
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

    this->m_vehicleActivationTimes = getActivationData(vehicleSettings);
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
    this->m_rsuActivationTimes = getActivationData(rsuSettings);
    this->m_numRSUs = this->m_rsuActivationTimes.size();
    NS_LOG_DEBUG("RSU size: " << this->m_numRSUs);

    this->m_rsuNodes.Create(this->m_numRSUs);
    for (auto iter = this->m_rsuNodes.Begin(); iter != this->m_rsuNodes.End(); ++iter)
    {
        NS_LOG_DEBUG("RSU Node ID: " << (*iter)->GetId());
    }
    this->m_allNodes.Add(this->m_rsuNodes);
    this->setupRSUPositions(rsuSettings);
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
Core::createControllerNodes()
{
    toml_value controllerSettings = this->findTable(CONST_COLUMNS::c_controllerSettings);
    this->m_rsuActivationTimes = getActivationData(controllerSettings);
    this->m_numControllers = this->m_rsuActivationTimes.size();

    this->m_controllerNodes.Create(this->m_numControllers);
    this->setupControllerPositions(controllerSettings);
    this->m_allNodes.Add(this->m_controllerNodes);
}

void
Core::setupControllerPositions(toml_value controllerSettings)
{
    std::string positionFile =
        toml::find<std::string>(controllerSettings, CONST_COLUMNS::c_positionFile);
    NS_LOG_DEBUG("Reading Position file: " << positionFile);
    PositionReader positionReader(positionFile);
    position_map_t positionData = positionReader.getPositionData();

    for (auto iter = this->m_controllerNodes.Begin(); iter != this->m_controllerNodes.End(); iter++)
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
            NS_ASSERT_MSG(false, "No position data found for controller: " << node->GetId());
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

activation_map_t
Core::getActivationData(const toml_value& nodeSettings)
{
    std::string activationFile =
        toml::find<std::string>(nodeSettings, CONST_COLUMNS::c_activationFile);
    NS_LOG_DEBUG("Reading Activation file: " << activationFile);
    ActivationReader activationReader(activationFile);
    activation_map_t activationTimes = activationReader.getActivationTimes();
    return activationTimes;
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
    NS_LOG_INFO("Create Controller nodes");
    this->createControllerNodes();
    NS_LOG_INFO("Setting up NR module for all Nodes.");
    toml_value networkSettings = this->findTable(CONST_COLUMNS::c_netSettings);

    Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper>();
    // Beam forming can be enabled later
    // Ptr<IdealBeamformingHelper> beamHelper = CreateObject<IdealBeamformingHelper>();
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();
    // nrHelper->SetBeamformingHelper(beamHelper);
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
    this->m_allDevices.Add(this->m_controllerDevices);
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

    NS_LOG_INFO("Separating devices into vehicle and RSU devices...");
    this->segregateNetDevices();

    int64_t stream = 1;
    stream += nrHelper->AssignStreams(this->m_allDevices, stream);
    stream += slHelper->AssignStreams(this->m_allDevices, stream);

    NS_LOG_INFO("Configure Internet Stack...");

    InternetStackHelper internet;
    internet.Install(this->m_allNodes);
    stream += internet.AssignStreams(this->m_allNodes, stream);

    uint32_t dstL2Id = 255;
    Ptr<LteSlTft> tft;
    Ipv4InterfaceContainer ueIpIface;
    NS_LOG_INFO("Assigning IP addresses...");
    ueIpIface = epcHelper->AssignUeIpv4Address(this->m_allDevices);

    // set the default gateway for the UE
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    for (uint32_t u = 0; u < this->m_allNodes.GetN(); ++u)
    {
        Ptr<Node> ueNode = this->m_allNodes.Get(u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    tft = Create<LteSlTft>(LteSlTft::Direction::TRANSMIT,
                           LteSlTft::CommType::GroupCast,
                           this->m_groupCastAddr,
                           dstL2Id);
    // Set Sidelink bearers
    slHelper->ActivateNrSlBearer(Time::From(0), this->m_vehicleDevices, tft);

    tft = Create<LteSlTft>(LteSlTft::Direction::BIDIRECTIONAL,
                           LteSlTft::CommType::Uincast,
                           this->m_groupCastAddr,
                           dstL2Id);
    // Set Sidelink bearers
    slHelper->ActivateNrSlBearer(Time::From(0), this->m_rsuDevices, tft);

    tft = Create<LteSlTft>(LteSlTft::Direction::RECEIVE,
                           LteSlTft::CommType::GroupCast,
                           this->m_groupCastAddr,
                           dstL2Id);
    // Set Sidelink bearers
    slHelper->ActivateNrSlBearer(Time::From(0), this->m_controllerDevices, tft);

    // Configure applications for vehicle nodes
    NS_LOG_DEBUG("Setup Tx and Rx applications");
    ApplicationContainer vehicleApps =
        this->m_netSetup->setupTxApplications(this->m_vehicleNodes,
                                              this->m_groupCastAddr,
                                              this->m_vehicleActivationTimes);
    ApplicationContainer rsuApps = this->m_netSetup->setupRxUdpApplications(this->m_rsuNodes);
    ApplicationContainer contApps = this->m_netSetup->setupRxApplications(this->m_controllerNodes);
    rsuApps.Add(contApps);

    NS_LOG_DEBUG("All nodes size: " << this->m_allNodes.GetN());

    toml_value outputSettings = this->findTable(CONST_COLUMNS::c_outputSettings);
    std::string outputPath = toml::find<std::string>(outputSettings, CONST_COLUMNS::c_outputPath);
    std::string outputName = toml::find<std::string>(outputSettings, CONST_COLUMNS::c_outputName);
    SQLiteOutput db(outputPath + outputName + ".db");
    Outputter outputter;

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

    UeToUePktTxRxOutputStats pktStats;
    pktStats.SetDb(&db, "pktTxRx");

    for (uint32_t ac = 0; ac < vehicleApps.GetN(); ac++)
    {
        Ipv4Address localAddrs = vehicleApps.Get(ac)
                                     ->GetNode()
                                     ->GetObject<Ipv4L3Protocol>()
                                     ->GetAddress(1, 0)
                                     .GetLocal();
        NS_LOG_DEBUG("Setting Tx trace for address: " << localAddrs);
        vehicleApps.Get(ac)->TraceConnect("TxWithSeqTsSize",
                                          "tx",
                                          MakeBoundCallback(&Outputter::UePacketTraceDb,
                                                            &pktStats,
                                                            vehicleApps.Get(ac)->GetNode(),
                                                            localAddrs));
    }

    // Set Rx traces
    for (uint32_t ac = 0; ac < rsuApps.GetN(); ac++)
    {
        Ipv4Address localAddrs =
            rsuApps.Get(ac)->GetNode()->GetObject<Ipv4L3Protocol>()->GetAddress(1, 0).GetLocal();
        NS_LOG_DEBUG("Setting Rx trace for address: " << localAddrs);
        rsuApps.Get(ac)->TraceConnect("RxWithSeqTsSize",
                                      "rx",
                                      MakeBoundCallback(&Outputter::UePacketTraceDb,
                                                        &pktStats,
                                                        rsuApps.Get(ac)->GetNode(),
                                                        localAddrs));
    }

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

    NS_LOG_DEBUG("Running the simulation...");

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
        // Last node is the controller node
        if (device->GetNode()->GetId() == this->m_allNodes.GetN() - 1)
        {
            m_controllerDevices.Add(device);
            continue;
        }

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
    NS_LOG_DEBUG("Controller devices size: " << m_controllerDevices.GetN());
}
