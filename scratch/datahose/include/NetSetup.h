/*
 * Core.h
 *
 * Created on: 2023-11-21
 * Author: charan
 */

#ifndef NS3_NETSETUP_H
#define NS3_NETSETUP_H

#include "ns3/node-container.h"
#include <toml.hpp>
#include <map>

using namespace ns3;

typedef toml::basic_value<toml::discard_comments, std::map, std::vector> toml_value;

class NetSetup
{
private:
  toml_value m_config;

public:
  NetSetup (toml_value config);

  void setupNetworkingParameters ();

  void setupVehicularNet (std::unique_ptr<NodeContainer> vehicleNodes);
};
#endif // NS3_NETSETUP_H
