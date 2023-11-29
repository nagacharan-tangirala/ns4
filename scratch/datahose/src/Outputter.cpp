#include "Outputter.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Outputter");

void
Outputter::NotifySlPscchScheduling(UeMacPscchTxOutputStats* pscchStats,
                                   const SlPscchUeMacStatParameters pscchStatsParams)
{
    pscchStats->Save(pscchStatsParams);
}

void
Outputter::NotifySlPsschScheduling(UeMacPsschTxOutputStats* psschStats,
                                   const SlPsschUeMacStatParameters psschStatsParams)
{
    psschStats->Save(psschStatsParams);
}

void
Outputter::NotifySlPscchRx(UePhyPscchRxOutputStats* pscchStats,
                           const SlRxCtrlPacketTraceParams pscchStatsParams)
{
    pscchStats->Save(pscchStatsParams);
}

void
Outputter::NotifySlPsschRx(UePhyPsschRxOutputStats* psschStats,
                           const SlRxDataPacketTraceParams psschStatsParams)
{
    psschStats->Save(psschStatsParams);
}

void
Outputter::UePacketTraceDb(UeToUePktTxRxOutputStats* stats,
                           Ptr<Node> node,
                           const Address& localAddrs,
                           std::string txRx,
                           Ptr<const Packet> p,
                           const Address& srcAddrs,
                           const Address& dstAddrs,
                           const SeqTsSizeHeader& seqTsSizeHeader)
{
    uint32_t nodeId = node->GetId();
    uint64_t imsi = node->GetDevice(0)->GetObject<NrUeNetDevice>()->GetImsi();
    uint32_t seq = seqTsSizeHeader.GetSeq();
    uint32_t pktSize = p->GetSize() + seqTsSizeHeader.GetSerializedSize();

    stats->Save(txRx, localAddrs, nodeId, imsi, pktSize, srcAddrs, dstAddrs, seq);
}

void
Outputter::NotifySlRlcPduRx(UeRlcRxOutputStats* stats,
                            uint64_t imsi,
                            uint16_t rnti,
                            uint16_t txRnti,
                            uint8_t lcid,
                            uint32_t rxPduSize,
                            double delay)
{
    stats->Save(imsi, rnti, txRnti, lcid, rxPduSize, delay);
}

void
Outputter::SavePositionPerIP(V2xKpi* v2xKpi)
{
    for (auto it = NodeList::Begin(); it != NodeList::End(); ++it)
    {
        Ptr<Node> node = *it;
        int nDevs = node->GetNDevices();
        for (int j = 0; j < nDevs; j++)
        {
            Ptr<NrUeNetDevice> uedev = node->GetDevice(j)->GetObject<NrUeNetDevice>();
            if (uedev)
            {
                Ptr<Ipv4L3Protocol> ipv4Protocol = node->GetObject<Ipv4L3Protocol>();
                Ipv4InterfaceAddress addresses = ipv4Protocol->GetAddress(1, 0);
                std::ostringstream ueIpv4Addr;
                ueIpv4Addr.str("");
                ueIpv4Addr << addresses.GetLocal();
                Vector pos = node->GetObject<MobilityModel>()->GetPosition();
                v2xKpi->FillPosPerIpMap(ueIpv4Addr.str(), pos);
            }
        }
    }
}

void
Outputter::RecordMobility(bool FirstWrite, std::string fileName)
{
    std::ofstream outFile;
    if (FirstWrite)
    {
        outFile.open(fileName.c_str(), std::ios_base::out);
        FirstWrite = false;
    }
    else
    {
        outFile.open(fileName.c_str(), std::ios_base::app);
        outFile << std::endl;
        outFile << std::endl;
    }

    for (auto it = NodeList::Begin(); it != NodeList::End(); ++it)
    {
        Ptr<Node> node = *it;
        int nDevs = node->GetNDevices();
        for (int j = 0; j < nDevs; j++)
        {
            Ptr<NrUeNetDevice> uedev = node->GetDevice(j)->GetObject<NrUeNetDevice>();
            if (uedev)
            {
                Vector pos = node->GetObject<MobilityModel>()->GetPosition();
                outFile << Simulator::Now().GetSeconds() << " " << node->GetId() << " "
                        << uedev->GetImsi() << " " << pos.x << " " << pos.y << std::endl;
            }
        }
    }

    Simulator::Schedule(Seconds(0.1), &Outputter::RecordMobility, FirstWrite, fileName);
}
