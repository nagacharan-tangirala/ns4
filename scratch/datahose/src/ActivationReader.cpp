#include "ActivationReader.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ActivationReader");

ActivationReader::ActivationReader (const std::string &filename)
{
  PARQUET_ASSIGN_OR_THROW (this->m_activationFile, arrow::io::ReadableFile::Open (filename));
}

activation_map_t
ActivationReader::getActivationTimes ()
{
  activation_map_t activationTimes;
  auto table = this->readActivationData ();

  auto onTimeCol = table->GetColumnByName (CONST_COLUMNS::c_onTimes);
  auto offTimeCol = table->GetColumnByName (CONST_COLUMNS::c_offTimes);

  auto nodeIdColumn = table->GetColumnByName (CONST_COLUMNS::c_ns3Id);
  auto nodeIdData = nodeIdColumn->View (std::make_shared<arrow::Int64Type> ()).ValueOrDie ();

  for (int rowIdx = 0; rowIdx < nodeIdData->length (); rowIdx++)
    {
      auto nodeIdVal = nodeIdData->GetScalar (rowIdx).ValueOrDie ();
      uint64_t nodeId = std::static_pointer_cast<arrow::Int64Scalar> (nodeIdVal)->value;

      Time onTime = getTimeFromTimeColumn (onTimeCol, rowIdx);
      Time offTime = getTimeFromTimeColumn (offTimeCol, rowIdx);

      NS_LOG_DEBUG ("Node ID: " << nodeId << " On time: " << onTime.GetMilliSeconds ()
                                << " Off time: " << offTime.GetMilliSeconds ());
      activationTimes.insert (
          std::make_pair (nodeId, std::make_pair (MilliSeconds (0), MilliSeconds (0))));
    }
  return activationTimes;
}

Time
ActivationReader::getTimeFromTimeColumn (const std::shared_ptr<arrow::ChunkedArray> &timeColumn,
                                         int position) const
{
  auto onTimeVal = timeColumn->GetScalar (position).ValueOrDie ();
  auto onTimeList = std::static_pointer_cast<arrow::ListScalar> (onTimeVal)->value;
  auto onTimeScalar = onTimeList->GetScalar (0).ValueOrDie ();
  uint64_t onTime = std::static_pointer_cast<arrow::Int64Scalar> (onTimeScalar)->value;
  return MilliSeconds (onTime);
}

std::shared_ptr<arrow::Table>
ActivationReader::readActivationData ()

{
  std::unique_ptr<parquet::arrow::FileReader> reader;
  PARQUET_THROW_NOT_OK (
      parquet::arrow::OpenFile (this->m_activationFile, arrow::default_memory_pool (), &reader));
  std::shared_ptr<arrow::Table> table;
  PARQUET_THROW_NOT_OK (reader->ReadTable (&table));
  return table;
}