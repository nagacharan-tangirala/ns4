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

    auto nodeIdColumn = table->GetColumnByName(CONST_COLUMNS::c_ns3Id);
    auto nodeIdData = nodeIdColumn->View(std::make_shared<arrow::Int64Type>()).ValueOrDie();

    for (int rowIdx = 0; rowIdx < nodeIdData->length(); rowIdx++)
    {
        auto nodeIdVal = nodeIdData->GetScalar(rowIdx).ValueOrDie();
        uint64_t nodeId = std::static_pointer_cast<arrow::Int64Scalar>(nodeIdVal)->value;

        auto onTimeVal = onTimeCol->GetScalar(rowIdx).ValueOrDie();
        uint64_t onTime = std::static_pointer_cast<arrow::Int64Scalar>(onTimeVal)->value;
        Time onTimeMs = Time(MilliSeconds(onTime));

        auto offTimeVal = offTimeCol->GetScalar(rowIdx).ValueOrDie();
        uint64_t offTime = std::static_pointer_cast<arrow::Int64Scalar>(offTimeVal)->value;
        Time offTimeMs = Time(MilliSeconds(offTime));

        NS_LOG_DEBUG("Node ID: " << nodeId << " On time: " << onTimeMs.GetMilliSeconds()
                                 << " Off time: " << offTimeMs.GetMilliSeconds());

        if (activationTimes.find(nodeId) != activationTimes.end())
        {
            NS_LOG_UNCOND("Overwriting activation times for node " << nodeId);
        }
        activationTimes.insert(
            std::make_pair(nodeId, std::make_pair(onTime, offTime)));
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