/*
 * LinkReader.h
 *
 * Created on: 2023-12-26
 * Author: charan
 */

#ifndef NS3_LINKREADER_H
#define NS3_LINKREADER_H

#include "Columns.h"

#include "ns3/log.h"
#include "ns3/nstime.h"

#include <arrow/array.h>
#include <arrow/io/file.h>
#include <arrow/scalar.h>
#include <arrow/table.h>
#include <parquet/arrow/reader.h>
#include <parquet/file_reader.h>

using namespace ns3;

typedef std::map<uint32_t, std::vector<std::pair<Time, uint32_t>>> link_map_t; // <nodeId, <time, nodeId>>

class LinkReader
{
  private:
    std::shared_ptr<arrow::io::ReadableFile> m_linkFile;

    std::shared_ptr<arrow::Table> readLinkData();

  public:
    explicit LinkReader(const std::string& filename);

    link_map_t getLinks();
};

#endif // NS3_LINKREADER_H
