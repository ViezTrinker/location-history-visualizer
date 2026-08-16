/*!
 *\file json_loader.cpp
 *\brief SAX loader for Google Timeline JSON exports
 */

#include "json_loader.h"

#include <array>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <istream>
#include <sstream>
#include <streambuf>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <nlohmann/json.hpp>

#include "civil_time.h"
#include "load_observer.h"
#include "load_result.h"
#include "location_data.h"
#include "location_point.h"

namespace LocationHistory
{
   namespace
   {
      inline constexpr int64_t ProgressReportIntervalBytes = 256 * 1024;
      inline constexpr size_t StreamBufferSize = 65536;
      using StreamBuffer = std::array<char, StreamBufferSize>;

      enum class SaxContext : uint8_t
      {
         None = 0,
         SemanticSegments = 1,
         Segment = 2,
         TimelinePath = 3,
         TimelinePoint = 4,
         Visit = 5,
         TopCandidate = 6,
         PlaceLocation = 7,
         RawSignals = 8,
         RawSignal = 9,
         Position = 10
      };

      enum class ForceProgressReport : uint8_t
      {
         No = 0,
         Yes = 1
      };

      ParseResult ParseDoubleToken(const std::string_view text, double& value)
      {
         size_t startIndex = 0;
         while (startIndex < text.size())
         {
            const char current = text[startIndex];
            if ((current == '-') || (current == '+') || (current == '.') ||
                ((current >= '0') && (current <= '9')))
            {
               break;
            }
            startIndex += 1;
         }
         if (startIndex >= text.size())
         {
            return ParseResult::InvalidFormat;
         }

         size_t endIndex = startIndex + 1;
         while (endIndex < text.size())
         {
            const char current = text[endIndex];
            if ((current == '.') || (current == 'e') || (current == 'E') || (current == '+') || (current == '-') ||
                ((current >= '0') && (current <= '9')))
            {
               endIndex += 1;
               continue;
            }
            break;
         }

         const std::string_view token = text.substr(startIndex, endIndex - startIndex);
         double parsedValue = 0.0;
         const char* pBegin = token.data();
         const char* pEnd = pBegin + token.size();
         const auto parseResult = std::from_chars(pBegin, pEnd, parsedValue);
         if (parseResult.ec != std::errc())
         {
            return ParseResult::InvalidFormat;
         }

         value = parsedValue;
         return ParseResult::Ok;
      }

      void ReportProgress(
         LoadObserver* pObserver,
         const int64_t bytesRead,
         const int64_t bytesTotal,
         int64_t& lastReported,
         const ForceProgressReport force)
      {
         if (pObserver == nullptr)
         {
            return;
         }
         if (force == ForceProgressReport::No)
         {
            const int64_t delta = bytesRead - lastReported;
            if ((delta < ProgressReportIntervalBytes) && (bytesRead < bytesTotal))
            {
               return;
            }
         }

         lastReported = bytesRead;
         pObserver->OnProgress(bytesRead, bytesTotal);
      }

      class ProgressStreambuf : public std::streambuf
      {
         public:
            ProgressStreambuf(std::streambuf* pSource, LoadObserver* pObserver, const int64_t bytesTotal)
               : _pSource(pSource)
               , _pObserver(pObserver)
               , _bytesTotal(bytesTotal)
               , _bytesRead(0)
               , _lastReported(0)
            {
               setg(_buffer.data(), _buffer.data(), _buffer.data());
            }

         protected:
            int_type underflow(void) override
            {
               if (gptr() < egptr())
               {
                  return traits_type::to_int_type(*gptr());
               }
               if (_pSource == nullptr)
               {
                  return traits_type::eof();
               }

               const std::streamsize got = _pSource->sgetn(
                  _buffer.data(),
                  static_cast<std::streamsize>(_buffer.size()));
               if (got <= 0)
               {
                  ReportProgress(_pObserver, _bytesRead, _bytesTotal, _lastReported, ForceProgressReport::Yes);
                  return traits_type::eof();
               }

               _bytesRead += static_cast<int64_t>(got);
               ReportProgress(_pObserver, _bytesRead, _bytesTotal, _lastReported, ForceProgressReport::No);
               setg(_buffer.data(), _buffer.data(), _buffer.data() + static_cast<size_t>(got));
               return traits_type::to_int_type(*gptr());
            }

         private:
            std::streambuf* _pSource;
            LoadObserver* _pObserver;
            int64_t _bytesTotal;
            int64_t _bytesRead;
            int64_t _lastReported;
            StreamBuffer _buffer{};
      };

      class TimelineSaxHandler : public nlohmann::json_sax<nlohmann::json>
      {
         public:
            TimelineSaxHandler(LocationPointList& points, LoadObserver* pObserver)
               : _points(points)
               , _pObserver(pObserver)
            {
            }

            bool null(void) override
            {
               return ShouldContinue();
            }

            bool boolean(bool) override
            {
               return ShouldContinue();
            }

            bool number_integer(number_integer_t) override
            {
               return ShouldContinue();
            }

            bool number_unsigned(number_unsigned_t) override
            {
               return ShouldContinue();
            }

            bool number_float(number_float_t, const string_t&) override
            {
               return ShouldContinue();
            }

            bool binary(binary_t&) override
            {
               return ShouldContinue();
            }

            bool string(string_t& value) override
            {
               if (!ShouldContinue())
               {
                  return false;
               }

               const SaxContext current = CurrentContext();
               if ((_currentKey == "startTime") && (current == SaxContext::Segment))
               {
                  _segmentStartTime = value;
                  return true;
               }
               if ((_currentKey == "endTime") && (current == SaxContext::Segment))
               {
                  _segmentEndTime = value;
                  return true;
               }
               if ((_currentKey == "point") && (current == SaxContext::TimelinePoint))
               {
                  _pointLatLng = value;
                  return true;
               }
               if ((_currentKey == "time") && (current == SaxContext::TimelinePoint))
               {
                  _pointTime = value;
                  return true;
               }
               if (((_currentKey == "latLng") || (_currentKey == "LatLng")) && (current == SaxContext::PlaceLocation))
               {
                  _visitLatLng = value;
                  return true;
               }
               if (((_currentKey == "latLng") || (_currentKey == "LatLng")) && (current == SaxContext::Position))
               {
                  _pointLatLng = value;
                  return true;
               }
               if ((_currentKey == "timestamp") && (current == SaxContext::Position))
               {
                  _pointTime = value;
                  return true;
               }
               return true;
            }

            bool key(string_t& value) override
            {
               if (!ShouldContinue())
               {
                  return false;
               }
               _currentKey = value;
               return true;
            }

            bool start_object(std::size_t) override
            {
               if (!ShouldContinue())
               {
                  return false;
               }

               const SaxContext parent = CurrentContext();
               if (parent == SaxContext::SemanticSegments)
               {
                  _context.push_back(SaxContext::Segment);
                  _segmentStartTime.clear();
                  _segmentEndTime.clear();
                  _visitLatLng.clear();
                  return true;
               }
               if (parent == SaxContext::TimelinePath)
               {
                  _context.push_back(SaxContext::TimelinePoint);
                  _pointLatLng.clear();
                  _pointTime.clear();
                  return true;
               }
               if (parent == SaxContext::RawSignals)
               {
                  _context.push_back(SaxContext::RawSignal);
                  return true;
               }
               if ((parent == SaxContext::Segment) && (_currentKey == "visit"))
               {
                  _context.push_back(SaxContext::Visit);
                  return true;
               }
               if ((parent == SaxContext::Visit) && (_currentKey == "topCandidate"))
               {
                  _context.push_back(SaxContext::TopCandidate);
                  return true;
               }
               if ((parent == SaxContext::TopCandidate) && (_currentKey == "placeLocation"))
               {
                  _context.push_back(SaxContext::PlaceLocation);
                  return true;
               }
               if ((parent == SaxContext::RawSignal) && (_currentKey == "position"))
               {
                  _context.push_back(SaxContext::Position);
                  _pointLatLng.clear();
                  _pointTime.clear();
                  return true;
               }

               _context.push_back(SaxContext::None);
               return true;
            }

            bool end_object(void) override
            {
               if (!ShouldContinue())
               {
                  return false;
               }
               if (_context.empty())
               {
                  return true;
               }

               const SaxContext current = _context.back();
               if (current == SaxContext::TimelinePoint)
               {
                  TryAddPoint(_pointLatLng, _pointTime, "", PointSource::TimelinePath, _currentPathId);
               }
               if (current == SaxContext::Segment)
               {
                  TryAddPoint(_visitLatLng, _segmentStartTime, _segmentEndTime, PointSource::Visit, NoPathId);
               }
               if (current == SaxContext::Position)
               {
                  TryAddPoint(_pointLatLng, _pointTime, "", PointSource::RawPosition, NoPathId);
               }

               _context.pop_back();
               return true;
            }

            bool start_array(std::size_t) override
            {
               if (!ShouldContinue())
               {
                  return false;
               }
               if (_currentKey == "semanticSegments")
               {
                  _context.push_back(SaxContext::SemanticSegments);
                  return true;
               }
               if (_currentKey == "timelinePath")
               {
                  _currentPathId += 1;
                  _context.push_back(SaxContext::TimelinePath);
                  return true;
               }
               if (_currentKey == "rawSignals")
               {
                  _context.push_back(SaxContext::RawSignals);
                  return true;
               }

               _context.push_back(SaxContext::None);
               return true;
            }

            bool end_array(void) override
            {
               if (!ShouldContinue())
               {
                  return false;
               }
               if (!_context.empty())
               {
                  _context.pop_back();
               }
               return true;
            }

            bool parse_error(std::size_t, const std::string&, const nlohmann::json::exception&) override
            {
               _hadError = true;
               return false;
            }

            bool HadError(void) const
            {
               return _hadError;
            }

            bool WasCancelled(void) const
            {
               return _cancelled;
            }

         private:
            SaxContext CurrentContext(void) const
            {
               if (_context.empty())
               {
                  return SaxContext::None;
               }
               return _context.back();
            }

            bool ShouldContinue(void)
            {
               if ((_pObserver != nullptr) && _pObserver->IsCancelled())
               {
                  _cancelled = true;
                  return false;
               }
               return true;
            }

            void TryAddPoint(
               const std::string& latLngText,
               const std::string& timeText,
               const std::string& endTimeText,
               const PointSource source,
               const int32_t pathId)
            {
               if (latLngText.empty())
               {
                  return;
               }
               if (timeText.empty())
               {
                  return;
               }

               double latitude = 0.0;
               double longitude = 0.0;
               if (IsErr(ParseLatLng(latLngText, latitude, longitude)))
               {
                  return;
               }

               int64_t unixTimeMs = 0;
               int32_t utcOffsetMinutes = 0;
               if (IsErr(ParseIso8601(timeText, unixTimeMs, utcOffsetMinutes)))
               {
                  return;
               }

               LocationPoint point{};
               point.latitude = latitude;
               point.longitude = longitude;
               point.unixTimeMs = unixTimeMs;
               point.utcOffsetMinutes = utcOffsetMinutes;
               point.source = source;
               point.endUnixTimeMs = unixTimeMs;
               point.pathId = pathId;
               if (!endTimeText.empty())
               {
                  int64_t endUnixTimeMs = 0;
                  int32_t endUtcOffsetMinutes = 0;
                  if (IsOk(ParseIso8601(endTimeText, endUnixTimeMs, endUtcOffsetMinutes)))
                  {
                     point.endUnixTimeMs = endUnixTimeMs;
                  }
               }
               _points.push_back(point);
            }

            LocationPointList& _points;
            LoadObserver* _pObserver;
            std::vector<SaxContext> _context;
            std::string _currentKey;
            std::string _segmentStartTime;
            std::string _segmentEndTime;
            std::string _visitLatLng;
            std::string _pointLatLng;
            std::string _pointTime;
            int32_t _currentPathId = NoPathId;
            bool _hadError = false;
            bool _cancelled = false;
      };

      bool IsCancelled(LoadObserver* pObserver, const TimelineSaxHandler& handler)
      {
         if (handler.WasCancelled())
         {
            return true;
         }
         if (pObserver == nullptr)
         {
            return false;
         }
         return pObserver->IsCancelled();
      }

      LoadResult ParseJsonStream(
         std::istream& input,
         const int64_t bytesTotal,
         LocationPointList& points,
         LoadObserver* pObserver)
      {
         if (pObserver != nullptr)
         {
            pObserver->OnProgress(0, bytesTotal);
            if (pObserver->IsCancelled())
            {
               return LoadResult::Cancelled;
            }
         }

         ProgressStreambuf progressBuf(input.rdbuf(), pObserver, bytesTotal);
         std::istream progressStream(&progressBuf);
         TimelineSaxHandler handler(points, pObserver);
         const bool parsed = nlohmann::json::sax_parse(progressStream, &handler);
         if (IsCancelled(pObserver, handler))
         {
            return LoadResult::Cancelled;
         }
         if (!parsed)
         {
            return LoadResult::InvalidJson;
         }
         if (handler.HadError())
         {
            return LoadResult::InvalidJson;
         }
         if (points.empty())
         {
            return LoadResult::NoPoints;
         }
         if (pObserver != nullptr)
         {
            pObserver->OnProgress(bytesTotal, bytesTotal);
         }
         return LoadResult::Ok;
      }

      int64_t StreamSize(std::istream& input)
      {
         input.clear();
         input.seekg(0, std::ios::end);
         const std::streampos endPos = input.tellg();
         input.clear();
         input.seekg(0, std::ios::beg);
         if (endPos < std::streampos(0))
         {
            return 0;
         }
         return static_cast<int64_t>(endPos);
      }
   } // namespace

   ParseResult ParseLatLng(const std::string_view text, double& latitude, double& longitude)
   {
      const size_t comma = text.find(',');
      if (comma == std::string_view::npos)
      {
         return ParseResult::InvalidFormat;
      }

      if (IsErr(ParseDoubleToken(text.substr(0, comma), latitude)))
      {
         return ParseResult::InvalidFormat;
      }
      if (IsErr(ParseDoubleToken(text.substr(comma + 1), longitude)))
      {
         return ParseResult::InvalidFormat;
      }
      if ((latitude < -90.0) || (latitude > 90.0))
      {
         return ParseResult::InvalidFormat;
      }
      if ((longitude < -180.0) || (longitude > 180.0))
      {
         return ParseResult::InvalidFormat;
      }

      return ParseResult::Ok;
   }

   LoadResult LoadFromString(const std::string_view jsonText, LocationPointList& points, LoadObserver* pObserver)
   {
      points.clear();
      if (jsonText.empty())
      {
         return LoadResult::InvalidJson;
      }

      const std::string jsonString(jsonText);
      std::istringstream input(jsonString, std::ios::binary);
      return ParseJsonStream(input, static_cast<int64_t>(jsonString.size()), points, pObserver);
   }

   LoadResult LoadFromFile(const std::string_view path, LocationPointList& points, LoadObserver* pObserver)
   {
      points.clear();
      if (path.empty())
      {
         return LoadResult::FileNotFound;
      }

      const std::string pathString(path);
      std::ifstream input(pathString, std::ios::binary);
      if (!input.is_open())
      {
         return LoadResult::FileNotFound;
      }

      const int64_t bytesTotal = StreamSize(input);
      return ParseJsonStream(input, bytesTotal, points, pObserver);
   }
} // namespace LocationHistory
