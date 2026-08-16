/*!
 *\file gpx_exporter.cpp
 *\brief Writes filtered location points as GPX 1.1
 */

#include "gpx_exporter.h"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "civil_time.h"
#include "export_result.h"
#include "location_data.h"
#include "location_point.h"
#include "version.h"

namespace LocationHistory
{
   namespace
   {
      struct PathTrack
      {
         int32_t pathId;
         LocationPointList points;
      };

      using PathTrackList = std::vector<PathTrack>;
      inline constexpr int32_t CoordinateDecimals = 8;

      bool RawPositionTimeLess(const LocationPoint& left, const LocationPoint& right)
      {
         return left.unixTimeMs < right.unixTimeMs;
      }

      void AppendEscaped(std::string& output, const std::string_view text)
      {
         for (size_t index = 0; index < text.size(); ++index)
         {
            const char current = text[index];
            if (current == '&')
            {
               output += "&amp;";
               continue;
            }
            if (current == '<')
            {
               output += "&lt;";
               continue;
            }
            if (current == '>')
            {
               output += "&gt;";
               continue;
            }
            if (current == '"')
            {
               output += "&quot;";
               continue;
            }
            output.push_back(current);
         }
      }

      void AppendCoordinate(std::string& output, const double value)
      {
         std::ostringstream stream;
         stream.imbue(std::locale::classic());
         stream << std::fixed << std::setprecision(CoordinateDecimals) << value;
         output += stream.str();
      }

      void AppendIsoTime(std::string& output, const LocationPoint& point)
      {
         std::string isoTime;
         FormatIso8601(point.unixTimeMs, point.utcOffsetMinutes, isoTime);
         AppendEscaped(output, isoTime);
      }

      void AppendWaypoint(std::string& output, const LocationPoint& point)
      {
         output += "  <wpt lat=\"";
         AppendCoordinate(output, point.latitude);
         output += "\" lon=\"";
         AppendCoordinate(output, point.longitude);
         output += "\">\n    <time>";
         AppendIsoTime(output, point);
         output += "</time>\n    <name>Visit</name>\n  </wpt>\n";
      }

      void AppendTrackPoint(std::string& output, const LocationPoint& point)
      {
         output += "      <trkpt lat=\"";
         AppendCoordinate(output, point.latitude);
         output += "\" lon=\"";
         AppendCoordinate(output, point.longitude);
         output += "\">\n        <time>";
         AppendIsoTime(output, point);
         output += "</time>\n      </trkpt>\n";
      }

      void AppendTrack(std::string& output, const std::string_view name, const LocationPointList& points)
      {
         if (points.empty())
         {
            return;
         }

         output += "  <trk>\n    <name>";
         AppendEscaped(output, name);
         output += "</name>\n    <trkseg>\n";
         for (size_t index = 0; index < points.size(); ++index)
         {
            AppendTrackPoint(output, points[index]);
         }
         output += "    </trkseg>\n  </trk>\n";
      }

      PathTrack* FindPathTrack(PathTrackList& tracks, const int32_t pathId)
      {
         for (size_t index = 0; index < tracks.size(); ++index)
         {
            if (tracks[index].pathId == pathId)
            {
               return &tracks[index];
            }
         }
         return nullptr;
      }

      void AppendPathIndexName(std::string& name, const size_t pathIndex)
      {
         name = "Path ";
         std::ostringstream stream;
         stream << (pathIndex + 1);
         name += stream.str();
      }
   } // namespace

   ExportResult WriteGpx(const LocationPointList& points, std::string& output)
   {
      output.clear();
      if (points.empty())
      {
         return ExportResult::NoPoints;
      }

      PathTrackList pathTracks;
      LocationPointList rawPoints;
      LocationPointList visits;
      for (size_t index = 0; index < points.size(); ++index)
      {
         const LocationPoint& point = points[index];
         if (point.source == PointSource::Visit)
         {
            visits.push_back(point);
            continue;
         }
         if (point.source == PointSource::RawPosition)
         {
            rawPoints.push_back(point);
            continue;
         }
         if (point.source != PointSource::TimelinePath)
         {
            continue;
         }

         PathTrack* pTrack = FindPathTrack(pathTracks, point.pathId);
         if (pTrack == nullptr)
         {
            PathTrack track{};
            track.pathId = point.pathId;
            track.points.push_back(point);
            pathTracks.push_back(track);
            continue;
         }
         pTrack->points.push_back(point);
      }

      std::sort(rawPoints.begin(), rawPoints.end(), RawPositionTimeLess);

      output += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
      output += "<gpx version=\"1.1\" creator=\"";
      AppendEscaped(output, AppName);
      output += " ";
      AppendEscaped(output, AppVersion);
      output += "\" xmlns=\"http://www.topografix.com/GPX/1/1\">\n";

      for (size_t index = 0; index < visits.size(); ++index)
      {
         AppendWaypoint(output, visits[index]);
      }
      for (size_t index = 0; index < pathTracks.size(); ++index)
      {
         std::string trackName;
         AppendPathIndexName(trackName, index);
         AppendTrack(output, trackName, pathTracks[index].points);
      }
      AppendTrack(output, "Raw positions", rawPoints);
      output += "</gpx>\n";
      return ExportResult::Ok;
   }
} // namespace LocationHistory
