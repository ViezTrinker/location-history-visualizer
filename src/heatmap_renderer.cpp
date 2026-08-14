/*!
 *\file heatmap_renderer.cpp
 *\brief Heatmap accumulation, color ramp, and Gaussian blur kernels
 */

#include "heatmap_renderer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace LocationHistory
{
   namespace
   {
      inline constexpr double Pi = 3.14159265358979323846;
      inline constexpr int32_t KernelMinRadius = 1;
      inline constexpr uint32_t AlphaShift = 24;
      inline constexpr uint32_t RedShift = 16;
      inline constexpr uint32_t GreenShift = 8;

      uint32_t MakeArgb(const uint8_t alpha, const uint8_t red, const uint8_t green, const uint8_t blue)
      {
         return (static_cast<uint32_t>(alpha) << AlphaShift) |
                (static_cast<uint32_t>(red) << RedShift) |
                (static_cast<uint32_t>(green) << GreenShift) |
                static_cast<uint32_t>(blue);
      }

      uint8_t MixChannel(const uint8_t left, const uint8_t right, const float amount)
      {
         const float mixed = static_cast<float>(left) + (static_cast<float>(right) - static_cast<float>(left)) * amount;
         const int32_t rounded = static_cast<int32_t>(mixed + 0.5f);
         return static_cast<uint8_t>(std::clamp(rounded, 0, 255));
      }

      int32_t ClampIndex(const int32_t value, const int32_t minValue, const int32_t maxValue)
      {
         if (value < minValue)
         {
            return minValue;
         }
         if (value > maxValue)
         {
            return maxValue;
         }
         return value;
      }
   } // namespace

   float GaussianKernel(const double distancePx, const double sigma)
   {
      if (sigma <= 0.0)
      {
         return 0.0f;
      }

      const double exponent = -(distancePx * distancePx) / (2.0 * sigma * sigma);
      const double normalization = 1.0 / (2.0 * Pi * sigma * sigma);
      return static_cast<float>(normalization * std::exp(exponent));
   }

   void AddGaussianSpot(HeatBuffer& buffer, const int32_t width, const int32_t height, const double centerX, const double centerY, const double sigma, const float weight)
   {
      if (width <= 0)
      {
         return;
      }
      if (height <= 0)
      {
         return;
      }
      if (buffer.size() != static_cast<size_t>(width) * static_cast<size_t>(height))
      {
         return;
      }
      if (sigma <= 0.0)
      {
         return;
      }

      const auto radius = static_cast<int32_t>(std::ceil(sigma * 3.0));
      const auto minX = ClampIndex(static_cast<int32_t>(std::floor(centerX)) - radius, 0, width - 1);
      const auto maxX = ClampIndex(static_cast<int32_t>(std::ceil(centerX)) + radius, 0, width - 1);
      const auto minY = ClampIndex(static_cast<int32_t>(std::floor(centerY)) - radius, 0, height - 1);
      const auto maxY = ClampIndex(static_cast<int32_t>(std::ceil(centerY)) + radius, 0, height - 1);

      for (int32_t pixelY = minY; pixelY <= maxY; ++pixelY)
      {
         for (int32_t pixelX = minX; pixelX <= maxX; ++pixelX)
         {
            const double deltaX = static_cast<double>(pixelX) - centerX;
            const double deltaY = static_cast<double>(pixelY) - centerY;
            const double distance = std::sqrt(deltaX * deltaX + deltaY * deltaY);
            const size_t bufferIndex = static_cast<size_t>(pixelY) * static_cast<size_t>(width) + static_cast<size_t>(pixelX);
            buffer[bufferIndex] += GaussianKernel(distance, sigma) * weight;
         }
      }
   }

   void AddHeatSample(HeatBuffer& buffer, const int32_t width, const int32_t height, const double centerX, const double centerY, const float weight)
   {
      if (width <= 0)
      {
         return;
      }
      if (height <= 0)
      {
         return;
      }
      if (buffer.size() != static_cast<size_t>(width) * static_cast<size_t>(height))
      {
         return;
      }

      const auto pixelX = static_cast<int32_t>(std::lround(centerX));
      const auto pixelY = static_cast<int32_t>(std::lround(centerY));
      if ((pixelX < 0) || (pixelX >= width))
      {
         return;
      }
      if ((pixelY < 0) || (pixelY >= height))
      {
         return;
      }

      const size_t bufferIndex = static_cast<size_t>(pixelY) * static_cast<size_t>(width) + static_cast<size_t>(pixelX);
      buffer[bufferIndex] += weight;
   }

   uint32_t ColorFromHeat(const float normalizedIntensity)
   {
      const float clamped = std::clamp(normalizedIntensity, 0.0f, 1.0f);
      if (clamped <= HeatEpsilon)
      {
         return MakeArgb(0, 0, 0, 0);
      }

      const uint8_t alpha = MixChannel(0, 220, std::min(clamped * 1.5f, 1.0f));
      if (clamped < 0.25f)
      {
         const float amount = clamped / 0.25f;
         return MakeArgb(alpha, MixChannel(0, 0, amount), MixChannel(0, 0, amount), MixChannel(255, 255, amount));
      }
      if (clamped < 0.5f)
      {
         const float amount = (clamped - 0.25f) / 0.25f;
         return MakeArgb(alpha, MixChannel(0, 0, amount), MixChannel(0, 255, amount), MixChannel(255, 255, amount));
      }
      if (clamped < 0.75f)
      {
         const float amount = (clamped - 0.5f) / 0.25f;
         return MakeArgb(alpha, MixChannel(0, 255, amount), MixChannel(255, 255, amount), MixChannel(255, 0, amount));
      }

      const float amount = (clamped - 0.75f) / 0.25f;
      return MakeArgb(alpha, MixChannel(255, 255, amount), MixChannel(255, 0, amount), MixChannel(0, 0, amount));
   }

   float MaxHeat(const HeatBuffer& buffer)
   {
      float maximum = 0.0f;
      for (size_t index = 0; index < buffer.size(); ++index)
      {
         if (buffer[index] > maximum)
         {
            maximum = buffer[index];
         }
      }
      return maximum;
   }

   float HeatScaleFromSlider(const int32_t sliderValue)
   {
      int32_t clampedSlider = sliderValue;
      if (clampedSlider < HeatScaleSliderMin)
      {
         clampedSlider = HeatScaleSliderMin;
      }
      if (clampedSlider > HeatScaleSliderMax)
      {
         clampedSlider = HeatScaleSliderMax;
      }

      const auto exponent = static_cast<float>(clampedSlider) / HeatScaleLogDivisor;
      const float scale = std::pow(HeatScaleLogBase, exponent);
      return std::clamp(scale, HeatScaleMin, HeatScaleMax);
   }

   float ScaledHeatCeiling(const float maxHeat, const float heatScale)
   {
      if (maxHeat <= HeatEpsilon)
      {
         return maxHeat;
      }

      float clampedScale = heatScale;
      if (clampedScale < HeatScaleMin)
      {
         clampedScale = HeatScaleMin;
      }
      if (clampedScale > HeatScaleMax)
      {
         clampedScale = HeatScaleMax;
      }

      return maxHeat / clampedScale;
   }

   void HeatBufferToArgb(const HeatBuffer& buffer, ArgbBuffer& output, const float maxHeat)
   {
      output.resize(buffer.size());
      if (maxHeat <= HeatEpsilon)
      {
         for (size_t index = 0; index < output.size(); ++index)
         {
            output[index] = MakeArgb(0, 0, 0, 0);
         }
         return;
      }

      for (size_t index = 0; index < buffer.size(); ++index)
      {
         output[index] = ColorFromHeat(buffer[index] / maxHeat);
      }
   }

   void GaussianBlur(const HeatBuffer& input, HeatBuffer& output, const int32_t width, const int32_t height, const int32_t radius)
   {
      output.clear();
      if (width <= 0)
      {
         return;
      }
      if (height <= 0)
      {
         return;
      }
      if (input.size() != static_cast<size_t>(width) * static_cast<size_t>(height))
      {
         return;
      }

      int32_t blurRadius = radius;
      if (blurRadius < KernelMinRadius)
      {
         blurRadius = KernelMinRadius;
      }

      const auto sigma = static_cast<double>(blurRadius) / 2.0;
      std::vector<float> kernel(static_cast<size_t>(blurRadius * 2 + 1));
      float kernelSum = 0.0f;
      for (int32_t offset = -blurRadius; offset <= blurRadius; ++offset)
      {
         const float value = GaussianKernel(static_cast<double>(offset), sigma);
         kernel[static_cast<size_t>(offset + blurRadius)] = value;
         kernelSum += value;
      }
      if (kernelSum <= HeatEpsilon)
      {
         output = input;
         return;
      }
      for (size_t index = 0; index < kernel.size(); ++index)
      {
         kernel[index] /= kernelSum;
      }

      HeatBuffer horizontal(input.size(), 0.0f);
      for (int32_t pixelY = 0; pixelY < height; ++pixelY)
      {
         for (int32_t pixelX = 0; pixelX < width; ++pixelX)
         {
            float accumulated = 0.0f;
            for (int32_t offset = -blurRadius; offset <= blurRadius; ++offset)
            {
               const int32_t sampleX = ClampIndex(pixelX + offset, 0, width - 1);
               const size_t sampleIndex = static_cast<size_t>(pixelY) * static_cast<size_t>(width) + static_cast<size_t>(sampleX);
               accumulated += input[sampleIndex] * kernel[static_cast<size_t>(offset + blurRadius)];
            }
            const size_t outIndex = static_cast<size_t>(pixelY) * static_cast<size_t>(width) + static_cast<size_t>(pixelX);
            horizontal[outIndex] = accumulated;
         }
      }

      output.assign(input.size(), 0.0f);
      for (int32_t pixelY = 0; pixelY < height; ++pixelY)
      {
         for (int32_t pixelX = 0; pixelX < width; ++pixelX)
         {
            float accumulated = 0.0f;
            for (int32_t offset = -blurRadius; offset <= blurRadius; ++offset)
            {
               const int32_t sampleY = ClampIndex(pixelY + offset, 0, height - 1);
               const size_t sampleIndex = static_cast<size_t>(sampleY) * static_cast<size_t>(width) + static_cast<size_t>(pixelX);
               accumulated += horizontal[sampleIndex] * kernel[static_cast<size_t>(offset + blurRadius)];
            }
            const size_t outIndex = static_cast<size_t>(pixelY) * static_cast<size_t>(width) + static_cast<size_t>(pixelX);
            output[outIndex] = accumulated;
         }
      }
   }
} // namespace LocationHistory
