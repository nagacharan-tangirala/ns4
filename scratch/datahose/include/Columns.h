//
// Created by charan on 21.11.23.
//

#ifndef CONST_COLUMNS
#define CONST_COLUMNS

#include <map>
#include <string>
#include <toml.hpp>

typedef toml::basic_value<toml::discard_comments, std::map, std::vector> toml_value;

const std::string c_simSettings = "simulation_settings";
const std::string c_logSettings = "log_settings";
const std::string c_netSettings = "network_settings";
const std::string c_antennaSettings = "antenna_settings";
const std::string c_macPhySettings = "mac_phy_settings";
const std::string c_sidelinkSettings = "sidelink_settings";
const std::string c_outputSettings = "output_settings";

const std::string c_vehicleSettings = "vehicle";
const std::string c_rsuSettings = "rsu";
const std::string c_controllerSettings = "controller";

const std::string c_activationFile = "activations";
const std::string c_traceFile = "traces";
const std::string c_positionFile = "positions";
const std::string c_v2rLinkFile = "v2r_links";

const std::string c_nodeId = "node_id";
const std::string c_ns3Id = "ns3_id";
const std::string c_timeStep = "time_step";
const std::string c_coordX = "x";
const std::string c_coordY = "y";

const std::string c_onTimes = "on_times";
const std::string c_offTimes = "off_times";

const std::string c_targetId = "target_id";

const std::string c_stopTime = "sim_duration";
const std::string c_streamTime = "sim_streaming_step";
const std::string c_stepSize = "sim_step_size";

const std::string c_logComponents = "log_components";
const std::string c_logLevel = "log_level";

const std::string c_outputPath = "output_path";
const std::string c_outputName = "output_name";

#endif // CONST_COLUMNS
