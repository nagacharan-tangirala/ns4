/*
 * Core.h
 *
 * Created on: 2023-11-21
 * Author: charan
 */

#ifndef NS3_CORE_H
#define NS3_CORE_H

#include "NetSetup.h"
#include "ActivationReader.h"
#include "PositionReader.h"
#include "Columns.h"
#include "TraceMobility.h"
#include "toml.hpp"

#include "ns3/network-module.h"
#include "ns3/constant-position-mobility-model.h"

using namespace ns3;

class Core
{
private:
  std::string m_configFile;
  toml_value m_config;
  std::unique_ptr<NetSetup> m_netSetup;

  Time m_stopTime;
  Time m_streamTime;

  std::unique_ptr<NodeContainer> m_vehicleNodes;
  std::unique_ptr<NodeContainer> m_rsuNodes;
  std::unique_ptr<NodeContainer> m_controllerNodes;

  std::unique_ptr<TraceMobility> m_vehicleMobility;

  toml_value findTable (const std::string &tableName) const;

  void scheduleMobilityUpdate ();

  void createVehicleNodes ();

  void createRsuNodes ();

  void createControllerNodes ();

  void setupRSUPositions (toml_value rsuSettings);

  void setupControllerPositions (toml_value controllerSettings);

  static LogLevel getLogLevel (std::string logLevel);

  void readSimulationSettings ();

  void readConfigFile ();

  void setupLogging () const;

  void setupVehicleMobility (toml_value vehicleSettings);

  static activation_map_t getActivationData (const toml_value &nodeSettings);

public:
  Core (std::string configFile);

  bool build ();

  Time getStopTime ();
};
#endif // NS3_CORE_H
