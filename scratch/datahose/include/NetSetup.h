/*
 * Core.h
 *
 * Created on: 2023-11-21
 * Author: charan
 */

#ifndef NS3_NETSETUP_H
#define NS3_NETSETUP_H

#include "ns3/node-container.h"
#include "ns3/nr-point-to-point-epc-helper.h"
#include "ns3/nr-helper.h"
#include <map>
#include <toml.hpp>

using namespace ns3;

typedef toml::basic_value<toml::discard_comments, std::map, std::vector> toml_value;

class NetSetup
{
  private:
    toml_value m_config;
    std::unique_ptr<NodeContainer> m_vehicleNodes;
    std::unique_ptr<NodeContainer> m_rsuNodes;
    std::unique_ptr<NodeContainer> m_controllerNodes;

  public:
    NetSetup(toml_value config);

    void readNetworkingParameters();

    std::unique_ptr<NodeContainer> setupVehicularNet(std::unique_ptr<NodeContainer> vehicleNodes);
};
#endif // NS3_NETSETUP_H
