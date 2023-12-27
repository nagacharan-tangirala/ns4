#include "ActivationReader.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ActivationReader");

ActivationReader::ActivationReader(const std::string& filename)
{
    PARQUET_ASSIGN_OR_THROW(this->m_activationFile, arrow::io::ReadableFile::Open(filename));
}

activation_map_t
ActivationReader::getActivationTimes()
{
    activation_map_t activationTimes;
    auto table = this->readActivationData();

    auto onTimeCol = table->GetColumnByName(CONST_COLUMNS::c_onTimes);
    auto offTimeCol = table->GetColumnByName(CONST_COLUMNS::c_offTimes);

    auto ns3IdColumn = table->GetColumnByName(CONST_COLUMNS::c_ns3Id);
    auto ns3IdData = ns3IdColumn->View(std::make_shared<arrow::Int64Type>()).ValueOrDie();

    auto nodeIdColumn = table->GetColumnByName(CONST_COLUMNS::c_nodeId);
    auto nodeIdData = nodeIdColumn->View(std::make_shared<arrow::Int64Type>()).ValueOrDie();

    for (int rowIdx = 0; rowIdx < ns3IdData->length(); rowIdx++)
    {
        auto ns3IdVal = ns3IdData->GetScalar(rowIdx).ValueOrDie();
        uint64_t ns3_node_id = std::static_pointer_cast<arrow::Int64Scalar>(ns3IdVal)->value;

        auto nodeIdVal = nodeIdData->GetScalar(rowIdx).ValueOrDie();
        uint64_t node_id = std::static_pointer_cast<arrow::Int64Scalar>(nodeIdVal)->value;

        NS_LOG_DEBUG("Node ID: " << node_id << " NS3 Node ID: " << ns3_node_id);
        this->m_nodeIdMap.insert(std::make_pair(node_id, ns3_node_id));

        auto onTimeVal = onTimeCol->GetScalar(rowIdx).ValueOrDie();
        uint64_t onTime = std::static_pointer_cast<arrow::Int64Scalar>(onTimeVal)->value;
        Time onTimeMs = Time(MilliSeconds(onTime));

        auto offTimeVal = offTimeCol->GetScalar(rowIdx).ValueOrDie();
        uint64_t offTime = std::static_pointer_cast<arrow::Int64Scalar>(offTimeVal)->value;
        Time offTimeMs = Time(MilliSeconds(offTime));

        NS_LOG_DEBUG("Node ID: " << ns3_node_id << " On time: " << onTimeMs.GetMilliSeconds()
                                 << " Off time: " << offTimeMs.GetMilliSeconds());

        if (activationTimes.find(ns3_node_id) != activationTimes.end())
        {
            NS_LOG_UNCOND("Overwriting activation times for node " << ns3_node_id);
        }
        activationTimes.insert(
            std::make_pair(ns3_node_id, std::make_pair(onTimeMs, offTimeMs)));
    }
    return activationTimes;
}

std::shared_ptr<arrow::Table>
ActivationReader::readActivationData()

{
    std::unique_ptr<parquet::arrow::FileReader> reader;
    PARQUET_THROW_NOT_OK(
        parquet::arrow::OpenFile(this->m_activationFile, arrow::default_memory_pool(), &reader));
    std::shared_ptr<arrow::Table> table;
    PARQUET_THROW_NOT_OK(reader->ReadTable(&table));
    return table;
}