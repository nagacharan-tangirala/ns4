/*
 * PositionReader.h
 *
 * Created on: 2023-11-22
 * Author: charan
 */

#ifndef NS3_POSITION_READER_H
#define NS3_POSITION_READER_H

#include "ns3/log.h"

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>

using namespace ns3;

typedef std::map<uint32_t, std::pair<double, double>> position_map_t;

class PositionReader
{
  private:
    std::shared_ptr<arrow::io::ReadableFile> m_positionFile;

    std::shared_ptr<arrow::Table> readPositionData();

  public:
    explicit PositionReader(const std::string& filename);

    position_map_t getPositionData();
};

#endif // NS3_POSITION_READER_H
