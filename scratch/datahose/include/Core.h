/*
 * Core.h
 *
 * Created on: 2024-01-19
 * Author: charan
 */

#ifndef NS3_CORE_H
#define NS3_CORE_H

#include "ActivationReader.h"
#include "Columns.h"
#include "Outputter.h"
#include "LinkReader.h"
#include "PositionReader.h"
#include "TraceMobility.h"

#include "ns3/constant-position-mobility-model.h"
#include "ns3/csma-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/string.h"
#include "ns3/udp-echo-helper.h"
#include "ns3/on-off-helper.h"
#include <toml.hpp>
#include <utility>

using namespace ns3;

typedef toml::basic_value<toml::discard_comments, std::map, std::vector> toml_value;

class Core
{
  private:
    std::string m_configFile;
    toml_value m_config;

    Time m_stopTime;
    Time m_streamTime;
    Time m_stepSize;

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
