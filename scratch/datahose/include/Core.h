/*
 * Core.h
 *
 * Created on: 2023-11-21
 * Author: charan
 */

#ifndef NS3_CORE_H
#define NS3_CORE_H

#include "ActivationReader.h"
#include "Columns.h"
#include "NetSetup.h"
#include "Outputter.h"
#include "LinkReader.h"
#include "PositionReader.h"
#include "TraceMobility.h"

#include "ns3/applications-module.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/core-module.h"
#include "ns3/isotropic-antenna-model.h"
#include "ns3/lte-sl-tft.h"
#include "ns3/network-module.h"
#include "ns3/node-container.h"
#include "ns3/nr-helper.h"
#include "ns3/nr-module.h"
#include "ns3/nr-point-to-point-epc-helper.h"
#include "ns3/nr-sl-helper.h"
#include "ns3/nr-ue-net-device.h"
#include "ns3/on-off-helper.h"

#include <toml.hpp>

using namespace ns3;

typedef toml::basic_value<toml::discard_comments, std::map, std::vector> toml_value;

class Core
{
  private:
    std::string m_configFile;
    toml_value m_config;

    std::unique_ptr<NetSetup> m_netSetup;

    Time m_stopTime;
    Time m_streamTime;

    uint32_t m_numVehicles;
    uint32_t m_numRSUs;

    NodeContainer m_vehicleNodes;
    NodeContainer m_rsuNodes;
    NodeContainer m_allNodes;

    NetDeviceContainer m_vehicleDevices;
    NetDeviceContainer m_rsuDevices;
    NetDeviceContainer m_allDevices;

    node_id_map_t m_rsuIdMap;

    activation_map_t m_rsuActivationTimes;
    activation_map_t m_vehicleActivationTimes;

    std::unique_ptr<TraceMobility> m_vehicleMobility;

    toml_value findTable(const std::string& tableName) const;

    void scheduleMobilityUpdate();

    void createVehicleNodes();

    void createRsuNodes();

    void setupRSUPositions(toml_value rsuSettings);

    static LogLevel getLogLevel(std::string logLevel);

    void readSimulationSettings();

    void readConfigFile();

    void setupLogging() const;

    void setupVehicleMobility(toml_value vehicleSettings);

    void run();

  public:
    Core(std::string configFile);

    void runSimulation();

    void segregateNetDevices();
};
#endif // NS3_CORE_H
