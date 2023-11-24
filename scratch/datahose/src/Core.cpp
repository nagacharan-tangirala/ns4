#include "Core.h"

#include <utility>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Core");

Core::Core(std::string configFile)
{
    this->m_configFile = std::move(configFile);
}

Time
Core::getStopTime()
{
    return this->m_stopTime;
}

bool
Core::build()
{
    NS_LOG_INFO("Reading config file: " << this->m_configFile);
    this->readConfigFile();
    NS_LOG_INFO("Setup NetSetup");
    this->m_netSetup = std::make_unique<NetSetup>(this->m_config);
    NS_LOG_INFO("Setup logging");
    this->setupLogging();
    NS_LOG_INFO("Reading Simulation settings");
    this->readSimulationSettings();
    NS_LOG_INFO("Create Vehicle nodes");
    this->createVehicleNodes();
    NS_LOG_INFO("Create RSU nodes");
    this->createRsuNodes();
    NS_LOG_INFO("Create Controller nodes");
    this->createControllerNodes();
    NS_LOG_INFO("Setup networking parameters");
    return true;
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

    activation_map_t activationTimes = getActivationData(vehicleSettings);
    const auto vehicleSize = activationTimes.size();
    NS_LOG_DEBUG("Vehicle size: " << vehicleSize);

    this->m_vehicleNodes = std::make_unique<NodeContainer>();
    this->m_vehicleNodes->Create(vehicleSize);
    this->setupVehicleMobility(vehicleSettings);

    this->m_vehicleNodes = this->m_netSetup->setupVehicularNet(std::move(this->m_vehicleNodes));
}

void
Core::setupVehicleMobility(toml_value vehicleSettings)
{
    std::string traceFile = toml::find<std::string>(vehicleSettings, CONST_COLUMNS::c_traceFile);
    NS_LOG_DEBUG("Reading Vehicle trace file: " << traceFile);
    this->m_vehicleMobility = std::make_unique<TraceMobility>(traceFile);

    for (auto iter = this->m_vehicleNodes->Begin(); iter != this->m_vehicleNodes->End(); iter++)
    {
        const Ptr<Node>& node = *iter;
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
    this->m_vehicleNodes =
        this->m_vehicleMobility->addWaypointsBetween(currentSimTime,
                                                     currentSimTime + this->m_streamTime,
                                                     std::move(this->m_vehicleNodes));
    NS_LOG_DEBUG("Scheduling mobility update at " << currentSimTime + this->m_streamTime);
    Simulator::Schedule(currentSimTime + this->m_streamTime, &Core::scheduleMobilityUpdate, this);
}

void
Core::createRsuNodes()
{
    toml_value rsuSettings = this->findTable(CONST_COLUMNS::c_rsuSettings);
    activation_map_t activationTimes = getActivationData(rsuSettings);
    const auto rsuSize = activationTimes.size();

    this->m_rsuNodes = std::make_unique<NodeContainer>();
    this->m_rsuNodes->Create(rsuSize);
    this->setupRSUPositions(rsuSettings);
}

void
Core::setupRSUPositions(toml_value rsuSettings)
{
    std::string positionFile = toml::find<std::string>(rsuSettings, CONST_COLUMNS::c_positionFile);
    NS_LOG_DEBUG("Reading Position file: " << positionFile);
    PositionReader positionReader(positionFile);
    position_map_t positionData = positionReader.getPositionData();

    for (auto iter = this->m_rsuNodes->Begin(); iter != this->m_rsuNodes->End(); iter++)
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
    activation_map_t activationTimes = getActivationData(controllerSettings);
    const auto controllerSize = activationTimes.size();

    this->m_controllerNodes = std::make_unique<NodeContainer>();
    this->m_controllerNodes->Create(controllerSize);
    this->setupControllerPositions(controllerSettings);
}

void
Core::setupControllerPositions(toml_value controllerSettings)
{
    std::string positionFile =
        toml::find<std::string>(controllerSettings, CONST_COLUMNS::c_positionFile);
    NS_LOG_DEBUG("Reading Position file: " << positionFile);
    PositionReader positionReader(positionFile);
    position_map_t positionData = positionReader.getPositionData();

    for (auto iter = this->m_controllerNodes->Begin(); iter != this->m_controllerNodes->End();
         iter++)
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
        setenv("NS_LOG", new_env.c_str(), true);
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