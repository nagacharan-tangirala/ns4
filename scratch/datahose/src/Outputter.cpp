#include "Outputter.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Outputter");

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
    uint64_t imsi = nodeId;
    uint32_t seq = seqTsSizeHeader.GetSeq();
    uint32_t pktSize = p->GetSize() + seqTsSizeHeader.GetSerializedSize();
    std::string pktUid = std::to_string(p->GetUid());

    stats->Save(txRx, localAddrs, nodeId, imsi, pktSize, srcAddrs, dstAddrs, seq, pktUid);
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
            Vector pos = node->GetObject<MobilityModel>()->GetPosition();
            outFile << Simulator::Now().GetSeconds() << " " << node->GetId() << " "
                    << node->GetId() << " " << pos.x << " " << pos.y << std::endl;
        }
    }

    Simulator::Schedule(Seconds(0.1), &Outputter::RecordMobility, FirstWrite, fileName);
}
