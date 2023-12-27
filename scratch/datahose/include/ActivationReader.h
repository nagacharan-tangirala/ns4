/*
 * ActivationReader.h
 *
 * Created on: 2023-11-22
 * Author: charan
 */

#ifndef NS3_ACTIVATION_READER_H
#define NS3_ACTIVATION_READER_H

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

typedef std::map<uint32_t, std::pair<Time, Time>> activation_map_t;
typedef std::map<uint64_t, uint64_t> node_id_map_t; // <pavenet nodeId, ns3 nodeId>

class ActivationReader
{
  private:
    std::shared_ptr<arrow::io::ReadableFile> m_activationFile;
    node_id_map_t m_nodeIdMap;

    std::shared_ptr<arrow::Table> readActivationData();

  public:
    explicit ActivationReader(const std::string& filename);

    activation_map_t getActivationTimes();

    node_id_map_t getNodeIdMap() { return this->m_nodeIdMap; }
};

#endif // NS3_ACTIVATION_READER_H
