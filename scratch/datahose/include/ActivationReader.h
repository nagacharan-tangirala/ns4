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

class ActivationReader
{
  private:
    std::shared_ptr<arrow::io::ReadableFile> m_activationFile;

    std::shared_ptr<arrow::Table> readActivationData();

  public:
    explicit ActivationReader(const std::string& filename);

    activation_map_t getActivationTimes();

    Time getTimeFromTimeColumn(const std::shared_ptr<arrow::ChunkedArray>& timeColumn,
                               int position) const;
};

#endif // NS3_ACTIVATION_READER_H
