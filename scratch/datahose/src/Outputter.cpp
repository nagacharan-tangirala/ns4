#include "Outputter.h"

NS_LOG_COMPONENT_DEFINE("Outputter");

Outputter::Outputter(std::string outputPath, std::string outputName, Time simDuration)
{
    this->m_outputPath = outputPath;
    this->m_outputName = outputName;
    this->m_duration = simDuration;
}

void
Outputter::configureDatabase(ApplicationContainer clientApps, ApplicationContainer serverApps)
{
    std::string dbPath = m_outputPath +  m_outputName + ".db";
    NS_LOG_DEBUG("Outputter: Database path: " << dbPath);
    SQLiteOutput db(dbPath);

    NS_LOG_DEBUG("Outputter: Connecting to SlPscchScheduling trace source");
    UeMacPscchTxOutputStats pscchStats;
    pscchStats.SetDb(&db, "pscchTxUeMac");
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/"
                                  "ComponentCarrierMapUe/*/NrUeMac/SlPscchScheduling",
                                  MakeBoundCallback(&NotifySlPscchScheduling, &this->m_pscchStats));

    NS_LOG_DEBUG("Outputter: Connecting to SlPsschScheduling trace source");
    this->m_psschStats.SetDb(&db, "psschTxUeMac");
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/"
                                  "ComponentCarrierMapUe/*/NrUeMac/SlPsschScheduling",
                                  MakeBoundCallback(&NotifySlPsschScheduling, &this->m_psschStats));

    NS_LOG_DEBUG("Outputter: Connecting to RxPscchTraceUe trace source");
    this->m_pscchPhyStats.SetDb(&db, "pscchRxUePhy");
    Config::ConnectWithoutContext(
        "/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUePhy/"
        "NrSpectrumPhyList/*/RxPscchTraceUe",
        MakeBoundCallback(&NotifySlPscchRx, &this->m_pscchPhyStats));

    NS_LOG_DEBUG("Outputter: Connecting to RxPsschTraceUe trace source");
    this->m_psschPhyStats.SetDb(&db, "psschRxUePhy");
    Config::ConnectWithoutContext(
        "/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUePhy/"
        "NrSpectrumPhyList/*/RxPsschTraceUe",
        MakeBoundCallback(&NotifySlPsschRx, &this->m_psschPhyStats));

    NS_LOG_DEBUG("Outputter: Connecting to RxRlcPduWithTxRnti trace source");
    this->m_ueRlcRxStats.SetDb(&db, "rlcRx");
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/"
                                  "ComponentCarrierMapUe/*/NrUeMac/RxRlcPduWithTxRnti",
                                  MakeBoundCallback(&NotifySlRlcPduRx, &this->m_ueRlcRxStats));

    this->m_pktStats.SetDb(&db, "pktTxRx");

    for (uint32_t ac = 0; ac < clientApps.GetN(); ac++)
    {
        Ipv4Address localAddrs =
            clientApps.Get(ac)->GetNode()->GetObject<Ipv4L3Protocol>()->GetAddress(1, 0).GetLocal();
        NS_LOG_DEBUG("Setting Tx trace for address: " << localAddrs);
        clientApps.Get(ac)->TraceConnect("TxWithSeqTsSize",
                                         "tx",
                                         MakeBoundCallback(&UePacketTraceDb,
                                                           &this->m_pktStats,
                                                           clientApps.Get(ac)->GetNode(),
                                                           localAddrs));
    }

    // Set Rx traces
    for (uint32_t ac = 0; ac < serverApps.GetN(); ac++)
    {
        Ipv4Address localAddrs =
            serverApps.Get(ac)->GetNode()->GetObject<Ipv4L3Protocol>()->GetAddress(1, 0).GetLocal();
        NS_LOG_DEBUG("Setting Rx trace for address: " << localAddrs);
        serverApps.Get(ac)->TraceConnect("RxWithSeqTsSize",
                                         "rx",
                                         MakeBoundCallback(&UePacketTraceDb,
                                                           &this->m_pktStats,
                                                           serverApps.Get(ac)->GetNode(),
                                                           localAddrs));
    }

    NS_LOG_DEBUG("Outputter: Connecting to V2xKpi trace source");
    std::string v2xkpiPath = this->m_outputPath + this->m_outputName + "_v2xkpi";
    this->m_v2xKpi.SetDbPath(v2xkpiPath);
    this->m_v2xKpi.SetTxAppDuration(this->m_duration.GetSeconds());
    SavePositionPerIP(&this->m_v2xKpi);
    this->m_v2xKpi.SetRangeForV2xKpis(200);

    std::string mobilityFileName = this->m_outputPath + "mobility-ues.txt";
    NS_LOG_DEBUG("Outputter: Connecting to Mobility trace source");
    RecordMobility(true, mobilityFileName);
}

void
    Outputter::dumpCache()
{
    this->m_pscchStats.EmptyCache();
    this->m_psschStats.EmptyCache();
    this->m_pscchPhyStats.EmptyCache();
    this->m_psschPhyStats.EmptyCache();
    this->m_ueRlcRxStats.EmptyCache();
    this->m_v2xKpi.WriteKpis();
    this->m_pktStats.EmptyCache();
}

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
