#include "TraceMobility.h"

#include "ArrowUtils.h"

#include <utility>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TraceMobility");

TraceMobility::TraceMobility(const std::string& filename)
    : m_traceReader(filename)
{
}

void TraceMobility::addWaypointsBetween(const Time& startTime,
                                   const Time& endTime,
                                   NodeContainer vehicleNodes)
{
    std::shared_ptr<arrow::Table> table =
        m_traceReader.streamDataBetween(startTime.GetMilliSeconds(), endTime.GetMilliSeconds());
    NS_LOG_DEBUG("Received Mobility data with " << table->num_rows() << " rows");
    if (table->num_rows() == 0)
    {
        NS_LOG_DEBUG("No more input data, no waypoints are added.");
        return;
    }

    auto xColumn = arrowUtils::getDoubleChunkedArray(table, CONST_COLUMNS::c_coordX);
    auto yColumn = arrowUtils::getDoubleChunkedArray(table, CONST_COLUMNS::c_coordY);
    auto timeColumn = arrowUtils::getInt64ChunkedArray(table, CONST_COLUMNS::c_timeStep);
    auto nodeIdColumn = arrowUtils::getInt64ChunkedArray(table, CONST_COLUMNS::c_nodeId);

    for (int row_idx = 0; row_idx < table->num_rows(); row_idx++)
    {
        double x = arrowUtils::getDoubleValue(xColumn, row_idx);
        double y = arrowUtils::getDoubleValue(yColumn, row_idx);
        int64_t time = arrowUtils::getInt64Value(timeColumn, row_idx);
        int64_t nodeId = arrowUtils::getInt64Value(nodeIdColumn, row_idx);

        NS_LOG_DEBUG("Node ID: " << nodeId << " X: " << x << " Y: " << y << " Time: " << time);

        Ptr<Node> node = vehicleNodes.Get(nodeId);
        Ptr<WaypointMobilityModel> mobility = node->GetObject<WaypointMobilityModel>();
        mobility->AddWaypoint(Waypoint(MilliSeconds(time), Vector(x, y, 0)));
    }
}
