/*
 * Core.h
 *
 * Created on: 2023-11-21
 * Author: charan
 */

#ifndef NS3_NETSETUP_H
#define NS3_NETSETUP_H

#include "NetworkParameters.h"
#include "Columns.h"
#include "ActivationReader.h"

#include <toml.hpp>

#include "ns3/constant-position-mobility-model.h"
#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/lte-sl-tft.h"
#include "ns3/node-container.h"
#include "ns3/nr-helper.h"
#include "ns3/nr-point-to-point-epc-helper.h"
#include "ns3/nr-sl-helper.h"
#include "ns3/nr-ue-net-device.h"
#include "ns3/on-off-helper.h"
#include "ns3/applications-module.h"
#include "ns3/isotropic-antenna-model.h"
#include "ns3/nr-module.h"

using namespace ns3;

typedef toml::basic_value<toml::discard_comments, std::map, std::vector> toml_value;

class NetSetup
{
  private:
    toml_value m_config;
    std::unique_ptr<NodeContainer> m_vehicleNodes;
    std::unique_ptr<NodeContainer> m_rsuNodes;
    std::unique_ptr<NodeContainer> m_controllerNodes;

    toml_value findTable(const std::string& tableName) const;

    void GetSlBitmapFromString (std::string slBitMapString, std::vector<std::bitset<1>> &slBitMapVector);

  public:
    NetSetup(toml_value config);

    OperationBandInfo updateNRHelperAndBuildOpBandInfo(const toml_value& networkSettings,
                                                       Ptr<NrHelper>& nrHelper) const;

    Ptr<NrHelper> configureAntenna(Ptr<NrHelper> nrHelper);

    Ptr<NrHelper> configureMacAndPhy(Ptr<NrHelper> nrHelper);

    Ptr<NrSlHelper> configureSideLink();

    LteRrcSap::SlBwpPoolConfigCommonNr configureResourcePool();

    LteRrcSap::SlFreqConfigCommonNr configureBandwidth(
        LteRrcSap::SlBwpPoolConfigCommonNr slResourcePool,
        std::set<uint8_t> bandwidth);

    LteRrcSap::SidelinkPreconfigNr configureSidelinkPreConfig();

    void setupTxApplications(NodeContainer nodes, Ipv4Address groupCastAddr, activation_map_t activationData);

    void setupRxApplications(NodeContainer nodes);
};
#endif // NS3_NETSETUP_H
