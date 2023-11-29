#include "NetSetup.h"

#include "ns3/antenna-module.h"
#include "ns3/boolean.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("NetSetup");

NetSetup::NetSetup(toml_value config)
{
    this->m_config = config;
}

toml_value
NetSetup::findTable(const std::string& tableName) const
{
    return toml::find<toml_value>(this->m_config, tableName);
}

OperationBandInfo
NetSetup::updateNRHelperAndBuildOpBandInfo(const toml_value& networkSettings,
                                           Ptr<NrHelper>& nrHelper) const
{
    CcBwpCreator ccBwpCreator;
    const uint8_t numCcPerBand = 1;

    uint16_t bandwidth = toml::find<uint16_t>(networkSettings, netParameters::n_bandwidth);
    double centralFrequency =
        toml::find<uint64_t>(networkSettings, netParameters::n_centerFrequency);
    CcBwpCreator::SimpleOperationBandConf bandConf(centralFrequency,
                                                   bandwidth,
                                                   numCcPerBand,
                                                   BandwidthPartInfo::V2V_Highway);

    OperationBandInfo operationBandInfo = ccBwpCreator.CreateOperationBandContiguousCc(bandConf);

    int32_t channelUpdatePeriod =
        toml::find<int32_t>(networkSettings, netParameters::n_channelUpdatePeriod);
    Config::SetDefault("ns3::ThreeGppChannelModel::UpdatePeriod",
                       TimeValue(MilliSeconds(channelUpdatePeriod)));
    nrHelper->SetChannelConditionModelAttribute("UpdatePeriod",
                                                TimeValue(MilliSeconds(channelUpdatePeriod)));

    bool shadowing = toml::find<bool>(networkSettings, netParameters::n_shadowing);
    nrHelper->SetPathlossAttribute("ShadowingEnabled", BooleanValue(shadowing));

    nrHelper->InitializeOperationBand(&operationBandInfo);
    return operationBandInfo;
}

Ptr<NrHelper>
NetSetup::configureAntenna(Ptr<NrHelper> nrHelper)
{
    toml_value antennaSettings = this->findTable(CONST_COLUMNS::c_antennaSettings);
    uint32_t numRows = toml::find<uint32_t>(antennaSettings, netParameters::n_numRows);
    uint32_t numColumns = toml::find<uint32_t>(antennaSettings, netParameters::n_numColumns);

    nrHelper->SetUeAntennaAttribute("NumRows", UintegerValue(numRows));
    nrHelper->SetUeAntennaAttribute("NumColumns", UintegerValue(numColumns));
    nrHelper->SetUeAntennaAttribute("AntennaElement",
                                    PointerValue(CreateObject<IsotropicAntennaModel>()));
    NS_LOG_DEBUG("Antenna setup: " << numRows << "x" << numColumns);
    return nrHelper;
}

Ptr<NrHelper>
NetSetup::configureMacAndPhy(Ptr<NrHelper> nrHelper)
{
    toml_value macPhySettings = this->findTable(CONST_COLUMNS::c_macPhySettings);
    bool sensingEnabled = toml::find<bool>(macPhySettings, netParameters::n_sensingEnabled);
    int32_t t1 = toml::find<int32_t>(macPhySettings, netParameters::n_t1);
    int32_t t2 = toml::find<int32_t>(macPhySettings, netParameters::n_t2);
    int32_t reservationPeriod =
        toml::find<int32_t>(macPhySettings, netParameters::n_reservationPeriod);
    int32_t sidelinkProcesses =
        toml::find<int32_t>(macPhySettings, netParameters::n_sidelinkProcesses);
    int32_t thresholdPsschRsrp =
        toml::find<int32_t>(macPhySettings, netParameters::n_thresholdPsschRsrp);
    bool blindReTxEnabled = toml::find<bool>(macPhySettings, netParameters::n_blindReTxEnabled);
    int32_t activePoolId = toml::find<int32_t>(macPhySettings, netParameters::n_activePoolId);

    nrHelper->SetUeMacAttribute("EnableSensing", BooleanValue(sensingEnabled));
    nrHelper->SetUeMacAttribute("T1", UintegerValue(t1));
    nrHelper->SetUeMacAttribute("T2", UintegerValue(t2));
    nrHelper->SetUeMacAttribute("ActivePoolId", UintegerValue(activePoolId));
    nrHelper->SetUeMacAttribute("ReservationPeriod", TimeValue(MilliSeconds(reservationPeriod)));
    nrHelper->SetUeMacAttribute("NumSidelinkProcess", UintegerValue(sidelinkProcesses));
    nrHelper->SetUeMacAttribute("SlThresPsschRsrp", IntegerValue(thresholdPsschRsrp));
    nrHelper->SetUeMacAttribute("EnableBlindReTx", BooleanValue(blindReTxEnabled));

    double txPower = toml::find<double>(macPhySettings, netParameters::n_txPower);
    nrHelper->SetUePhyAttribute("TxPower", DoubleValue(txPower));

    NS_LOG_DEBUG("MAC and PHY setup: "
                 << "sensingEnabled: " << sensingEnabled << ", t1: " << t1 << ", t2: " << t2
                 << ", reservationPeriod: " << reservationPeriod << ", sidelinkProcesses: "
                 << sidelinkProcesses << ", thresholdPsschRsrp: " << thresholdPsschRsrp
                 << ", blindReTxEnabled: " << blindReTxEnabled << ", activePoolId: " << activePoolId
                 << ", txPower: " << txPower << "dBm");

    return nrHelper;
}

Ptr<NrSlHelper>
NetSetup::configureSideLink()
{
    toml_value slSettings = this->findTable(CONST_COLUMNS::c_sidelinkSettings);
    Ptr<NrSlHelper> slHelper = CreateObject<NrSlHelper>();

    // Configure the error model and AMC
    std::string errorModel = "ns3::NrEesmIrT1";
    slHelper->SetSlErrorModel(errorModel);
    slHelper->SetUeSlAmcAttribute("AmcModel", EnumValue(NrAmc::ErrorModel));
    slHelper->SetNrSlSchedulerTypeId(NrSlUeMacSchedulerSimple::GetTypeId());

    bool fix_mcs = toml::find<bool>(slSettings, netParameters::n_fix_mcs);
    int32_t initial_mcs = toml::find<int32_t>(slSettings, netParameters::n_initial_mcs);

    slHelper->SetUeSlSchedulerAttribute("FixNrSlMcs", BooleanValue(fix_mcs));
    slHelper->SetUeSlSchedulerAttribute("InitialNrSlMcs", UintegerValue(initial_mcs));

    return slHelper;
}

LteRrcSap::SlBwpPoolConfigCommonNr
NetSetup::configureResourcePool()
{
    toml_value slSettings = this->findTable(CONST_COLUMNS::c_sidelinkSettings);
    Ptr<NrSlCommPreconfigResourcePoolFactory> poolFactory =
        Create<NrSlCommPreconfigResourcePoolFactory>();

    std::string sl_bitmap = toml::find<std::string>(slSettings, netParameters::n_slBitMap);
    std::vector<std::bitset<1>> slBitMapVector;
    this->GetSlBitmapFromString(sl_bitmap, slBitMapVector);
    poolFactory->SetSlTimeResources(slBitMapVector);

    uint16_t slSensingWindow = toml::find<uint16_t>(slSettings, netParameters::n_slSensingWindow);
    uint16_t slSelectionWindow =
        toml::find<uint16_t>(slSettings, netParameters::n_slSelectionWindow);
    uint16_t slSubChannelSize = toml::find<uint16_t>(slSettings, netParameters::n_slSubChannelSize);
    uint16_t slMaxNumPerReserve =
        toml::find<uint16_t>(slSettings, netParameters::n_slMaxNumPerReserve);

    poolFactory->SetSlSensingWindow(slSensingWindow);
    poolFactory->SetSlSelectionWindow(slSelectionWindow);
    poolFactory->SetSlSubchannelSize(slSubChannelSize);
    poolFactory->SetSlMaxNumPerReserve(slMaxNumPerReserve);

    poolFactory->SetSlFreqResourcePscch(10);

    LteRrcSap::SlResourcePoolNr resourcePool = poolFactory->CreatePool();
    LteRrcSap::SlResourcePoolConfigNr poolConfig;
    poolConfig.haveSlResourcePoolConfigNr = true;
    uint16_t poolId = 0; // TODO: Read from config

    LteRrcSap::SlResourcePoolIdNr poolIdNr;
    poolIdNr.id = poolId;
    poolConfig.slResourcePoolId = poolIdNr;
    poolConfig.slResourcePool = resourcePool;

    LteRrcSap::SlBwpPoolConfigCommonNr slBwpPoolConfigCommonNr;
    // Array for pools, we insert the pool in the array as per its poolId
    slBwpPoolConfigCommonNr.slTxPoolSelectedNormal[poolIdNr.id] = poolConfig;

    return slBwpPoolConfigCommonNr;
}

void
NetSetup::GetSlBitmapFromString(std::string slBitMapString,
                                std::vector<std::bitset<1>>& slBitMapVector)
{
    static std::unordered_map<std::string, uint8_t> lookupTable = {
        {"0", 0},
        {"1", 1},
    };

    std::stringstream ss(slBitMapString);
    std::string token;
    std::vector<std::string> extracted;

    while (std::getline(ss, token, '|'))
    {
        extracted.push_back(token);
    }

    for (const auto& v : extracted)
    {
        if (lookupTable.find(v) == lookupTable.end())
        {
            NS_FATAL_ERROR("Bit type " << v << " not valid. Valid values are: 0 and 1");
        }
        slBitMapVector.push_back(lookupTable[v] & 0x01);
    }
}

LteRrcSap::SlFreqConfigCommonNr
NetSetup::configureBandwidth(LteRrcSap::SlBwpPoolConfigCommonNr slResourcePool,
                             std::set<uint8_t> bwpIds)
{
    toml_value slSettings = this->findTable(CONST_COLUMNS::c_sidelinkSettings);

    double numerology = toml::find<int32_t>(slSettings, netParameters::n_numerology);
    uint16_t bandwidth = toml::find<uint16_t>(slSettings, netParameters::n_bandwidth);

    // Configure the BWP IE
    LteRrcSap::Bwp bwp;
    bwp.numerology = numerology;
    bwp.symbolsPerSlots = 14;
    bwp.rbPerRbg = 1;
    bwp.bandwidth = bandwidth;

    // Configure the SlBwpGeneric IE
    LteRrcSap::SlBwpGeneric slBwpGeneric;
    slBwpGeneric.bwp = bwp;
    slBwpGeneric.slLengthSymbols = LteRrcSap::GetSlLengthSymbolsEnum(14);
    slBwpGeneric.slStartSymbol = LteRrcSap::GetSlStartSymbolEnum(0);

    // Configure the SlBwpConfigCommonNr IE
    LteRrcSap::SlBwpConfigCommonNr slBwpConfigCommonNr;
    slBwpConfigCommonNr.haveSlBwpGeneric = true;
    slBwpConfigCommonNr.slBwpGeneric = slBwpGeneric;
    slBwpConfigCommonNr.haveSlBwpPoolConfigCommonNr = true;
    slBwpConfigCommonNr.slBwpPoolConfigCommonNr = slResourcePool;

    // Configure the SlFreqConfigCommonNr IE, which hold the array to store
    // the configuration of all Sidelink BWP (s).
    LteRrcSap::SlFreqConfigCommonNr slFreConfigCommonNr;
    // Array for BWPs. Here we will iterate over the BWPs, which
    // we want to use for SL.
    for (const auto& it : bwpIds)
    {
        // it is the BWP id
        slFreConfigCommonNr.slBwpList[it] = slBwpConfigCommonNr;
    }
    return slFreConfigCommonNr;
}

LteRrcSap::SidelinkPreconfigNr
NetSetup::configureSidelinkPreConfig()
{
    toml_value slSettings = this->findTable(CONST_COLUMNS::c_sidelinkSettings);

    std::string tddPattern = toml::find<std::string>(slSettings, netParameters::n_tddPattern);
    LteRrcSap::TddUlDlConfigCommon tddUlDlConfigCommon;
    tddUlDlConfigCommon.tddPattern = tddPattern;

    // Configure the SlPreconfigGeneralNr IE
    LteRrcSap::SlPreconfigGeneralNr slPreconfigGeneralNr;
    slPreconfigGeneralNr.slTddConfig = tddUlDlConfigCommon;

    // Configure the SlUeSelectedConfig IE
    LteRrcSap::SlUeSelectedConfig slUeSelectedPreConfig;
    double slProbResourceKeep = toml::find<double>(slSettings, netParameters::n_slProbResourceKeep);
    NS_ABORT_MSG_UNLESS(slProbResourceKeep <= 1.0,
                        "slProbResourceKeep value must be between 0 and 1");
    slUeSelectedPreConfig.slProbResourceKeep = slProbResourceKeep;

    // Configure the SlPsschTxParameters IE
    LteRrcSap::SlPsschTxParameters psschParams;
    uint16_t slMaxTxTransNumPssch =
        toml::find<uint16_t>(slSettings, netParameters::n_slMaxTxTransNumPssch);
    psschParams.slMaxTxTransNumPssch = static_cast<uint8_t>(slMaxTxTransNumPssch);

    // Configure the SlPsschTxConfigList IE
    LteRrcSap::SlPsschTxConfigList pscchTxConfigList;
    pscchTxConfigList.slPsschTxParameters[0] = psschParams;
    slUeSelectedPreConfig.slPsschTxConfigList = pscchTxConfigList;

    LteRrcSap::SidelinkPreconfigNr slPreConfigNr;
    slPreConfigNr.slPreconfigGeneral = slPreconfigGeneralNr;
    slPreConfigNr.slUeSelectedPreConfig = slUeSelectedPreConfig;

    return slPreConfigNr;
}

ApplicationContainer
NetSetup::setupTxUdpApplications(NodeContainer nodes,
                                 Ipv4Address groupCastAddr,
                                 activation_map_t activationData)
{
    NS_LOG_DEBUG("Setting up TX applications");
    Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable>();
    startTimeSeconds->SetStream(1);
    startTimeSeconds->SetAttribute("Min", DoubleValue(0));
    startTimeSeconds->SetAttribute("Max", DoubleValue(0.01));

    // Set Application in the UEs
    Address remoteAddress = InetSocketAddress(groupCastAddr, 8000);
    UdpClientHelper sidelinkClient(remoteAddress, 8000);

    toml_value networkSettings = this->findTable(CONST_COLUMNS::c_netSettings);
    std::string dataRateStr = toml::find<std::string>(networkSettings, netParameters::n_dataRate);
    int64_t packetInterval = toml::find<int64_t>(networkSettings, netParameters::n_packetInterval);
    int64_t packetSize = toml::find<int64_t>(networkSettings, netParameters::n_packetSize);

    sidelinkClient.SetAttribute("Interval", TimeValue(MilliSeconds(packetInterval)));
    sidelinkClient.SetAttribute("PacketSize", UintegerValue(packetSize));

    ApplicationContainer clientApps;
    for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
        clientApps.Add(sidelinkClient.Install(nodes.Get(i)));
        try
        {
            std::pair<Time, Time> activationTimes = activationData.at(i);
            double randomStart = startTimeSeconds->GetValue();
            clientApps.Get(i)->SetStartTime(activationTimes.first + Seconds(randomStart));
            clientApps.Get(i)->SetStopTime(activationTimes.second);
        }
        catch (const std::out_of_range& oor)
        {
            std::cerr << "Cannot find data for node " << i << " in activationData" << std::endl;
        }
    }
    return clientApps;
}

ApplicationContainer
NetSetup::setupTxApplications(NodeContainer nodes,
                              Ipv4Address groupCastAddr,
                              activation_map_t activationData)
{
    NS_LOG_DEBUG("Setting up TX applications");
    Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable>();
    startTimeSeconds->SetStream(1);
    startTimeSeconds->SetAttribute("Min", DoubleValue(0));
    startTimeSeconds->SetAttribute("Max", DoubleValue(0.1));

    // Set Application in the UEs
    Address remoteAddress = InetSocketAddress(groupCastAddr, 8000);
    OnOffHelper sidelinkClient("ns3::UdpSocketFactory", remoteAddress);
    sidelinkClient.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));

    toml_value networkSettings = this->findTable(CONST_COLUMNS::c_netSettings);
    std::string dataRateStr = toml::find<std::string>(networkSettings, netParameters::n_dataRate);
    int64_t packetSize = toml::find<int64_t>(networkSettings, netParameters::n_packetSize);
    sidelinkClient.SetConstantRate(DataRate(dataRateStr), packetSize);

    ApplicationContainer clientApps;
    for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
        clientApps.Add(sidelinkClient.Install(nodes.Get(i)));
        try
        {
            std::pair<Time, Time> activationTimes = activationData.at(i);
            double randomStart = startTimeSeconds->GetValue();
            clientApps.Get(i)->SetStartTime(activationTimes.first + Seconds(randomStart));
            clientApps.Get(i)->SetStopTime(activationTimes.second);
        }
        catch (const std::out_of_range& oor)
        {
            std::cerr << "Cannot find data for node " << i << " in activationData" << std::endl;
        }
    }
    return clientApps;
}

ApplicationContainer
NetSetup::setupRxApplications(NodeContainer nodes)
{
    NS_LOG_DEBUG("Setting up RX applications");
    ApplicationContainer serverApps;
    Address address = InetSocketAddress(Ipv4Address::GetAny(), 8000);
    PacketSinkHelper sidelinkSink("ns3::UdpSocketFactory", address);
    sidelinkSink.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));
    for (uint32_t i = 0; i < nodes.GetN(); i++)
    {
        NS_LOG_DEBUG("Installing RX application on node " << nodes.Get(i)->GetId());
        serverApps.Add(sidelinkSink.Install(nodes.Get(i)));
        serverApps.Start(Seconds(0.0));
    }
    return serverApps;
}

ApplicationContainer
NetSetup::setupRxUdpApplications(NodeContainer nodes)
{
    NS_LOG_DEBUG("Setting up RX applications");
    ApplicationContainer serverApps;
    UdpServerHelper sidelinkSink(8000);
    for (uint32_t i = 0; i < nodes.GetN(); i++)
    {
        serverApps.Add(sidelinkSink.Install(nodes.Get(i)));
        serverApps.Start(Seconds(0.0));
    }
    return serverApps;
}
