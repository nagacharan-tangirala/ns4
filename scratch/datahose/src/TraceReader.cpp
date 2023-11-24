#include "TraceReader.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TraceReader");

TraceReader::TraceReader (const std::string &filename) : m_rowGroupIdx (0)
{
  PARQUET_ASSIGN_OR_THROW (this->m_traceFile, arrow::io::ReadableFile::Open (filename));
}

std::shared_ptr<arrow::Table>
TraceReader::streamDataBetween (int64_t startTime, int64_t endTime)
{
  NS_LOG_DEBUG ("TraceReader: Reading data between " << startTime << " and " << endTime);
  std::shared_ptr<arrow::Table> combinedTable;
  std::unique_ptr<parquet::arrow::FileReader> reader;
  PARQUET_THROW_NOT_OK (
      parquet::arrow::OpenFile (this->m_traceFile, arrow::default_memory_pool (), &reader));

  if (this->m_rowGroupIdx == reader->num_row_groups ())
    {
      NS_LOG (LOG_DEBUG, "TraceReader: Trace data is used up.");
      return combinedTable;
    }

  bool dataFound = false;
  while (!dataFound)
    {
      std::shared_ptr<arrow::Table> table;
      PARQUET_THROW_NOT_OK (reader->RowGroup (this->m_rowGroupIdx)->ReadTable (&table));
      NS_LOG_DEBUG ("TraceReader: Table before starting time " << table->num_rows () << " rows");

      int64_t firstTimeStep = TraceReader::getFirstTimeStep (table);
      NS_LOG_DEBUG ("TraceReader: First time step is " << firstTimeStep);
      if (firstTimeStep < startTime)
        {
          NS_LOG_DEBUG ("TraceReader: Start table from start time " << startTime);
          table = TraceReader::startTableFrom (table, startTime);
        }

      int64_t lastTimeStep = TraceReader::getLastTimeStep (table);
      NS_LOG_DEBUG ("TraceReader: Last time step is " << lastTimeStep);

      if (lastTimeStep > endTime)
        {
          NS_LOG_DEBUG ("TraceReader: End table at end time " << endTime);
          table = TraceReader::pruneTableUntil (table, endTime);
          NS_LOG_DEBUG ("TraceReader: Table after ending has " << table->num_rows () << " rows");
          dataFound = true;
        }
      else
        {
          this->m_rowGroupIdx++;
          if (this->m_rowGroupIdx == reader->num_row_groups ())
            {
              NS_LOG_DEBUG ("TraceReader: All row groups are read.");
              dataFound = true;
            }
        }

      NS_LOG_DEBUG ("TraceReader: Table after starting has " << table->num_rows () << " rows");
      combinedTable = combinedTable == nullptr ? table : getCombinedTable (combinedTable, table);
    }
  return combinedTable;
}

std::shared_ptr<arrow::Table>
TraceReader::getCombinedTable (std::shared_ptr<arrow::Table> &combinedTable,
                               const std::shared_ptr<arrow::Table> &table)
{
  std::vector<std::shared_ptr<arrow::Table>> tables = {combinedTable, table};
  return arrow::ConcatenateTables (tables).ValueOrDie ();
}

int64_t
TraceReader::getLastTimeStep (const std::shared_ptr<arrow::Table> &table)
{
  auto lastRow = table->num_rows () - 1;
  auto inTimeStep =
      table->GetColumnByName (CONST_COLUMNS::c_timeStep)->GetScalar (lastRow).ValueOrDie ();
  return std::static_pointer_cast<arrow::Int64Scalar> (inTimeStep)->value;
}

int64_t
TraceReader::getFirstTimeStep (const std::shared_ptr<arrow::Table> &table)
{
  auto firstRow = 0;
  auto inTimeStep =
      table->GetColumnByName (CONST_COLUMNS::c_timeStep)->GetScalar (firstRow).ValueOrDie ();
  return std::static_pointer_cast<arrow::Int64Scalar> (inTimeStep)->value;
}

std::shared_ptr<arrow::Table>
TraceReader::pruneTableUntil (const std::shared_ptr<arrow::Table> &table, int64_t timeStep)
{
  int64_t idx = TraceReader::findIndexOfTimeStep (table, timeStep);
  return table->Slice (0, idx);
}

std::shared_ptr<arrow::Table>
TraceReader::startTableFrom (const std::shared_ptr<arrow::Table> &table, int64_t timeStep)
{
  // We are finding the index of time step to start from
  int64_t idx = TraceReader::findIndexOfTimeStep (table, timeStep);
  return table->Slice (idx, table->num_rows ());
}

int64_t
TraceReader::findIndexOfTimeStep (const std::shared_ptr<arrow::Table> &table, int64_t timeStep)
{
  auto timeStepCol = table->GetColumnByName (CONST_COLUMNS::c_timeStep);
  auto timeStepColData = timeStepCol->View (std::make_shared<arrow::Int64Type> ()).ValueOrDie ();

  int64_t idx = -1;
  for (int64_t i = 0; i < timeStepColData->length (); i++)
    {
      auto timeStepData = timeStepColData->GetScalar (i).ValueOrDie ();
      int64_t timeStepValue = std::static_pointer_cast<arrow::Int64Scalar> (timeStepData)->value;
      if (timeStepValue == timeStep)
        {
          idx = i;
          break;
        }
    }
  if (idx == -1)
    {
      NS_LOG (LOG_DEBUG, "TraceReader: Couldn't index for time step " << timeStep << ". Exiting.");
      exit (1);
    }
  return idx;
}
