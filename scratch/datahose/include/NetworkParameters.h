/*
 * NetSetup.h
 *
 * Created on: 2023-11-23
 * Author: charan
 */

#ifndef NETWORK_PARAMETERS
#define NETWORK_PARAMETERS

#include <string>

namespace netParameters
{
const std::string n_packetSize = "packet_size";
const std::string n_packetInterval = "packet_interval";
const std::string n_dataRate = "data_rate";

const std::string n_centerFrequency = "center_frequency";
const std::string n_bandwidth = "bandwidth";
const std::string n_txPower = "tx_power";
const std::string n_numerology = "numerology";

const std::string n_channelUpdatePeriod = "channel_update_period";
const std::string n_shadowing = "shadowing";

const std::string n_numRows = "num_rows";
const std::string n_numColumns = "num_columns";

const std::string n_sensingEnabled = "sensing_enabled";
const std::string n_t1 = "t1";
const std::string n_t2 = "t2";
const std::string n_activePoolId = "active_pool_id";
const std::string n_reservationPeriod = "reservation_period";
const std::string n_sidelinkProcesses = "sidelink_processes";
const std::string n_blindReTxEnabled = "blind_retx_enabled";
const std::string n_thresholdPsschRsrp = "threshold_pssch_rsrp";

const std::string n_fix_mcs = "fix_mcs_value";
const std::string n_initial_mcs = "initial_mcs";

const std::string n_tddPattern = "tdd_pattern";
const std::string n_slBitMap = "sl_bitmap";
const std::string n_slSensingWindow = "sl_sensing_window";
const std::string n_slSelectionWindow = "sl_selection_window";
const std::string n_slSubChannelSize = "sl_sub_channel_size";
const std::string n_slMaxNumPerReserve = "sl_max_num_per_reserve";
const std::string n_slProbResourceKeep = "sl_prob_resource_keep";
const std::string n_slMaxTxTransNumPssch = "max_tx_trans_num_pssch";

} // namespace netParameters

#endif // NETWORK_PARAMETERS
