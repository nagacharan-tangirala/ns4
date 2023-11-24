#include "NetSetup.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("NetSetup");

NetSetup::NetSetup (toml_value config)
{
  this->m_config = config;
}

void
NetSetup::setupNetworkingParameters ()
{
}

void
NetSetup::setupVehicularNet (std::unique_ptr<NodeContainer> vehicleNodes)
{
  NS_LOG_DEBUG ("Setting up vehicular network");
}