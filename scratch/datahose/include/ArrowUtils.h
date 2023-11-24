/*
 * ArrowUtils.h
 *
 * Created on: 2023-11-2e
 * Author: charan
 */

#ifndef ARROW_UTILS_H
#define ARROW_UTILS_H

#include "ns3/log.h"

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>

namespace arrowUtils
{
static std::shared_ptr<arrow::ChunkedArray>
getDoubleChunkedArray(const std::shared_ptr<arrow::Table>& table, const std::string& columnName)
{
    auto column = table->GetColumnByName(columnName);
    return column->View(std::make_shared<arrow::DoubleType>()).ValueOrDie();
}

static std::shared_ptr<arrow::ChunkedArray>
getInt64ChunkedArray(const std::shared_ptr<arrow::Table>& table, const std::string& columnName)
{
    auto column = table->GetColumnByName(columnName);
    return column->View(std::make_shared<arrow::Int64Type>()).ValueOrDie();
}

static double
getDoubleValue(const std::shared_ptr<arrow::ChunkedArray>& columnData, int row_idx)
{
    auto val = columnData->GetScalar(row_idx).ValueOrDie();
    return std::static_pointer_cast<arrow::DoubleScalar>(val)->value;
}

static int64_t
getInt64Value(const std::shared_ptr<arrow::ChunkedArray>& columnData, int row_idx)
{
    auto val = columnData->GetScalar(row_idx).ValueOrDie();
    return std::static_pointer_cast<arrow::Int64Scalar>(val)->value;
}

} // namespace arrowUtils

#endif // ARROW_UTILS_H
