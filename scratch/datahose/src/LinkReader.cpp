#include "LinkReader.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LinkReader");

LinkReader::LinkReader (const std::string &filename)
{
    PARQUET_ASSIGN_OR_THROW(this->m_linkFile, arrow::io::ReadableFile::Open(filename));
}

link_map_t
    LinkReader::getLinks () {
    link_map_t links;
    auto table = this->readLinkData();

    auto timeCol = table->GetColumnByName(CONST_COLUMNS::c_timeStep);
    auto nodeIdCol = table->GetColumnByName(CONST_COLUMNS::c_nodeId);
    auto targetIdCol = table->GetColumnByName(CONST_COLUMNS::c_targetId);

    auto timeData = timeCol->View(std::make_shared<arrow::Int64Type>()).ValueOrDie();

    for (int rowIdx = 0; rowIdx < timeData->length(); rowIdx++)
    {
        auto timeStepVal = timeData->GetScalar(rowIdx).ValueOrDie();
        uint64_t timeStep = std::static_pointer_cast<arrow::Int64Scalar>(timeStepVal)->value;
        Time timeMs = Time(MilliSeconds(timeStep));

        auto targetIdVal = targetIdCol->GetScalar(rowIdx).ValueOrDie();
        uint64_t targetId = std::static_pointer_cast<arrow::Int64Scalar>(targetIdVal)->value;

        auto nodeIdVal = nodeIdCol->GetScalar(rowIdx).ValueOrDie();
        uint64_t nodeId = std::static_pointer_cast<arrow::Int64Scalar>(nodeIdVal)->value;

        NS_LOG_DEBUG("Node ID: " << nodeId << " Target ID: " << targetId << " Time: " << timeMs.GetMilliSeconds());

        if (links.find(nodeId) == links.end())
        {
            links.insert(std::make_pair(nodeId, std::vector<std::pair<Time, uint32_t>>()));
        }
        links[nodeId].emplace_back(std::make_pair(timeMs, targetId));
    }
    return links;
}

std::shared_ptr<arrow::Table>
    LinkReader::readLinkData () {
    std::unique_ptr<parquet::arrow::FileReader> reader;
    PARQUET_THROW_NOT_OK(
        parquet::arrow::OpenFile(this->m_linkFile, arrow::default_memory_pool(), &reader));
    std::shared_ptr<arrow::Table> table;
    PARQUET_THROW_NOT_OK(reader->ReadTable(&table));
    return table;
}
