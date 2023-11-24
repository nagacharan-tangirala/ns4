/*
 * TraceReader.h
 *
 * Created on: 2023-11-22
 * Author: charan
 */

#ifndef NS3_TRACE_READER_H
#define NS3_TRACE_READER_H

#include "Columns.h"

#include "ns3/log.h"

#include <arrow/io/file.h>
#include <arrow/scalar.h>
#include <arrow/table.h>
#include <parquet/arrow/reader.h>
#include <parquet/file_reader.h>

using namespace ns3;

class TraceReader
{
  private:
    std::shared_ptr<arrow::io::ReadableFile> m_traceFile;
    int64_t m_rowGroupIdx;

    [[nodiscard]] static int64_t findIndexOfTimeStep(const std::shared_ptr<arrow::Table>& table,
                                                     int64_t timeStep);

    [[nodiscard]] static int64_t getLastTimeStep(const std::shared_ptr<arrow::Table>& table);

    [[nodiscard]] static int64_t getFirstTimeStep(const std::shared_ptr<arrow::Table>& table);

    [[nodiscard]] static std::shared_ptr<arrow::Table> pruneTableUntil(
        const std::shared_ptr<arrow::Table>& table,
        int64_t timeStep);

    [[nodiscard]] static std::shared_ptr<arrow::Table> startTableFrom(
        const std::shared_ptr<arrow::Table>& table,
        int64_t timeStep);

    [[nodiscard]] static std::shared_ptr<arrow::Table> getCombinedTable(
        std::shared_ptr<arrow::Table>& combinedTable,
        const std::shared_ptr<arrow::Table>& table);

  public:
    explicit TraceReader(const std::string& filename);

    std::shared_ptr<arrow::Table> streamDataBetween(int64_t startTime, int64_t endTime);
};

#endif // NS3_TRACE_READER_H
