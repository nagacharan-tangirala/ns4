/*
 * TraceMobility.h
 *
 * Created on: 2020-04-01
 * Author: charan
 */

#ifndef NS3_TRACE_MOBILITY_H
#define NS3_TRACE_MOBILITY_H

#include "TraceReader.h"

#include "ns3/mobility-helper.h"
#include "ns3/waypoint-mobility-model.h"

using namespace ns3;

class TraceMobility
{
  private:
    TraceReader m_traceReader;

  public:
    explicit TraceMobility(const std::string& filename);

    std::unique_ptr<NodeContainer> addWaypointsBetween(
        const Time& starTime,
        const Time& endTime,
        std::unique_ptr<NodeContainer> m_vehicleNodes);
};

#endif // NS3_TRACE_MOBILITY_H
