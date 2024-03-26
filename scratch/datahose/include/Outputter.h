/*
 * Outputter.h
 *
 * Created on: 2023-11-28
 * Author: charan
 */

#ifndef NS3_OUTPUTTER_H
#define NS3_OUTPUTTER_H

#include "Columns.h"
#include "ue-to-ue-pkt-txrx-output-stats.h"

#include "ns3/config.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/mobility-model.h"
#include "ns3/seq-ts-size-header.h"
#include "ns3/sqlite-output.h"

#include <toml.hpp>

using namespace ns3;

class Outputter
{
  private:
    UeToUePktTxRxOutputStats m_pktStats;

  public:
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
     * \brief Record mobility of the vehicle UEs every second
     * \param FirstWrite If this flag is true, write from scratch, otherwise, append to the file
     * \param fileName Name of the file in which to write the positions
     */
    static void RecordMobility(bool FirstWrite, std::string fileName);
};
#endif // NS3_OUTPUTTER_H
