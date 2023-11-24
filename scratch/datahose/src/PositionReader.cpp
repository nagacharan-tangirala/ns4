#include "PositionReader.h"

#include "ArrowUtils.h"
#include "Columns.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("PositionReader");

PositionReader::PositionReader(const std::string& filename)
{
    this->m_positionFile = arrow::io::ReadableFile::Open(filename).ValueOrDie();
}

std::shared_ptr<arrow::Table>
PositionReader::readPositionData()
{
    std::unique_ptr<parquet::arrow::FileReader> reader;
    PARQUET_THROW_NOT_OK(
        parquet::arrow::OpenFile(this->m_positionFile, arrow::default_memory_pool(), &reader));
    std::shared_ptr<arrow::Table> table;
    PARQUET_THROW_NOT_OK(reader->ReadTable(&table));
    return table;
}

position_map_t
PositionReader::getPositionData()
{
    auto table = this->readPositionData();

    auto xColumn = arrowUtils::getDoubleChunkedArray(table, CONST_COLUMNS::c_coordX);
    auto yColumn = arrowUtils::getDoubleChunkedArray(table, CONST_COLUMNS::c_coordY);
    auto nodeIdColumn = arrowUtils::getInt64ChunkedArray(table, CONST_COLUMNS::c_ns3Id);

    position_map_t positionData;
    for (int i = 0; i < table->num_rows(); i++)
    {
        int64_t nodeId = arrowUtils::getInt64Value(nodeIdColumn, i);
        double x = arrowUtils::getDoubleValue(xColumn, i);
        double y = arrowUtils::getDoubleValue(yColumn, i);
        NS_LOG_DEBUG("Node ID: " << nodeId << " X: " << x << " Y: " << y);
        positionData.insert(std::make_pair(nodeId, std::make_pair(x, y)));
    }
    return positionData;
}