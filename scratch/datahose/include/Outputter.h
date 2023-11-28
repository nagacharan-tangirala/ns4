/*
 * Outputter.h
 *
 * Created on: 2023-11-28
 * Author: charan
 */

#ifndef NS3_OUTPUTTER_H
#define NS3_OUTPUTTER_H

#include "Columns.h"
#include "ue-mac-pscch-tx-output-stats.h"
#include "ue-mac-pssch-tx-output-stats.h"
#include "ue-phy-pscch-rx-output-stats.h"
#include "ue-phy-pssch-rx-output-stats.h"
#include "ue-rlc-rx-output-stats.h"
#include "ue-to-ue-pkt-txrx-output-stats.h"
#include "v2x-kpi.h"

#include "ns3/config.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/mobility-model.h"
#include "ns3/nr-ue-mac.h"
#include "ns3/nr-ue-net-device.h"
#include "ns3/seq-ts-size-header.h"
#include "ns3/sqlite-output.h"

#include <toml.hpp>

using namespace ns3;

class Outputter
{
  private:
    std::string m_outputPath;
    std::string m_outputName;
    Time m_duration;
    UeMacPscchTxOutputStats m_pscchStats;
    UeMacPsschTxOutputStats m_psschStats;

    UePhyPsschRxOutputStats m_psschPhyStats;
    UePhyPscchRxOutputStats m_pscchPhyStats;

    UeRlcRxOutputStats m_ueRlcRxStats;
    UeToUePktTxRxOutputStats m_pktStats;
    V2xKpi m_v2xKpi;

  public:
    Outputter(std::string outputPath, std::string outputName, Time duration);

    void dumpCache();

    void configureDatabase(ApplicationContainer clientApps, ApplicationContainer serverApps);

    /**
     * \brief Method to listen the trace SlPscchScheduling of NrUeMac, which gets
     *        triggered upon the transmission of SCI format 1-A from UE MAC.
     *
     * \param pscchStats Pointer to the UeMacPscchTxOutputStats class,
     *        which is responsible to write the trace source parameters to a database.
     * \param pscchStatsParams Parameters of the trace source.
     */
    static void NotifySlPscchScheduling(UeMacPscchTxOutputStats* pscchStats,
                                        const SlPscchUeMacStatParameters pscchStatsParams);

    /**
     * \brief Method to listen the trace SlPsschScheduling of NrUeMac, which gets
     *        triggered upon the transmission of SCI format 2-A and data from UE MAC.
     *
     * \param psschStats Pointer to the UeMacPsschTxOutputStats class,
     *        which is responsible to write the trace source parameters to a database.
     * \param psschStatsParams Parameters of the trace source.
     */
    static void NotifySlPsschScheduling(UeMacPsschTxOutputStats* psschStats,
                                        const SlPsschUeMacStatParameters psschStatsParams);

    /**
     * \brief Method to listen the trace RxPscchTraceUe of NrSpectrumPhy, which gets
     *        triggered upon the reception of SCI format 1-A.
     *
     * \param pscchStats Pointer to the UePhyPscchRxOutputStats class,
     *        which is responsible to write the trace source parameters to a database.
     * \param pscchStatsParams Parameters of the trace source.
     */
    static void NotifySlPscchRx(UePhyPscchRxOutputStats* pscchStats,
                                const SlRxCtrlPacketTraceParams pscchStatsParams);

    /**
     * \brief Method to listen the trace RxPsschTraceUe of NrSpectrumPhy, which gets
     *        triggered upon the reception of SCI format 2-A and data.
     *
     * \param psschStats Pointer to the UePhyPsschRxOutputStats class,
     *        which is responsible to write the trace source parameters to a database.
     * \param psschStatsParams Parameters of the trace source.
     */
    static void NotifySlPsschRx(UePhyPsschRxOutputStats* psschStats,
                                const SlRxDataPacketTraceParams psschStatsParams);

    /**
     * \brief Method to listen the application level traces of type TxWithAddresses
     *        and RxWithAddresses.
     * \param stats Pointer to the UeToUePktTxRxOutputStats class,
     *        which is responsible to write the trace source parameters to a database.
     * \param node The pointer to the TX or RX node
     * \param localAddrs The local IPV4 address of the node
     * \param txRx The string indicating the type of node, i.e., TX or RX
     * \param p The packet
     * \param srcAddrs The source address from the trace
     * \param dstAddrs The destination address from the trace
     * \param seqTsSizeHeader The SeqTsSizeHeader
     */
    static void UePacketTraceDb(UeToUePktTxRxOutputStats* stats,
                                Ptr<Node> node,
                                const Address& localAddrs,
                                std::string txRx,
                                Ptr<const Packet> p,
                                const Address& srcAddrs,
                                const Address& dstAddrs,
                                const SeqTsSizeHeader& seqTsSizeHeader);

    /**
     * \brief Trace sink for RxRlcPduWithTxRnti trace of NrUeMac
     * \param stats Pointer to UeRlcRxOutputStats API responsible to write the
     *        information communicated by this trace into a database.
     * \param imsi The IMSI of the UE
     * \param rnti The RNTI of the UE
     * \param txRnti The RNTI of the TX UE
     * \param lcid The logical channel id
     * \param rxPduSize The received PDU size
     * \param delay The end-to-end, i.e., from TX RLC entity to RX
     *        RLC entity, delay in Seconds.
     */
    static void NotifySlRlcPduRx(UeRlcRxOutputStats* stats,
                                 uint64_t imsi,
                                 uint16_t rnti,
                                 uint16_t txRnti,
                                 uint8_t lcid,
                                 uint32_t rxPduSize,
                                 double delay);

    /**
     * \brief Save position of the UE as per its IP address
     * \param v2xKpi pointer to the V2xKpi API storing the IP of an UE and its position.
     */
    static void SavePositionPerIP(V2xKpi* v2xKpi);

    /**
     * \brief Record mobility of the vehicle UEs every second
     * \param FirstWrite If this flag is true, write from scratch, otherwise, append to the file
     * \param fileName Name of the file in which to write the positions
     */
    static void RecordMobility(bool FirstWrite, std::string fileName);
};
#endif // NS3_OUTPUTTER_H
