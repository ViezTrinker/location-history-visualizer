/*!
 *\file heatmap_renderer.h
 *\brief Heatmap accumulation, color ramp, and Gaussian blur kernels
 */

#ifndef HEATMAP_RENDERER_H
#define HEATMAP_RENDERER_H

#include <cstdint>
#include <vector>

namespace LocationHistory
{
   using HeatBuffer = std::vector<float>;
   using ArgbBuffer = std::vector<uint32_t>;

   inline constexpr double HeatmapSigmaPx = 16.0;
   inline constexpr int32_t BlurRadiusPx = 8;
   inline constexpr int32_t HeatmapDownsample = 4;
   inline constexpr float HeatEpsilon = 0.000001f;
   inline constexpr float HeatScaleMin = 1.0f;
   inline constexpr float HeatScaleMax = 100.0f;
   inline constexpr float HeatScaleDefault = 1.0f;
   inline constexpr float HeatScaleLogBase = 10.0f;
   inline constexpr float HeatScaleLogDivisor = 50.0f;
   inline constexpr int32_t HeatScaleSliderMin = 0;
   inline constexpr int32_t HeatScaleSliderMax = 100;

   /*!
    *\brief Evaluates a Gaussian kernel at a pixel distance
    *
    *\param[in] distancePx Distance from the center in pixels
    *\param[in] sigma Standard deviation in pixels
    */
   float GaussianKernel(double distancePx, double sigma);

   /*!
    *\brief Adds a Gaussian spot into a heat buffer
    *
    *\param[in,out] buffer Intensity buffer
    *\param[in] width Buffer width in pixels
    *\param[in] height Buffer height in pixels
    *\param[in] centerX Spot center X in pixels
    *\param[in] centerY Spot center Y in pixels
    *\param[in] sigma Standard deviation in pixels
    *\param[in] weight Intensity weight of the spot
    */
   void AddGaussianSpot(HeatBuffer& buffer, int32_t width, int32_t height, double centerX, double centerY, double sigma, float weight);

   /*!
    *\brief Adds a single sample to the nearest pixel of a heat buffer
    *
    * Samples outside the buffer are ignored.
    *
    *\param[in,out] buffer Intensity buffer
    *\param[in] width Buffer width in pixels
    *\param[in] height Buffer height in pixels
    *\param[in] centerX Sample X in pixels
    *\param[in] centerY Sample Y in pixels
    *\param[in] weight Intensity added to the pixel
    */
   void AddHeatSample(HeatBuffer& buffer, int32_t width, int32_t height, double centerX, double centerY, float weight);

   /*!
    *\brief Maps a normalized heat value 0..1 to an ARGB color
    *
    *\param[in] normalizedIntensity Intensity in the range 0..1
    */
   uint32_t ColorFromHeat(float normalizedIntensity);

   /*!
    *\brief Converts a heat buffer to ARGB using the maximum value for scaling
    *
    *\param[in] buffer Intensity buffer
    *\param[out] output ARGB pixels
    *\param[in] maxHeat Maximum heat used for normalization
    */
   void HeatBufferToArgb(const HeatBuffer& buffer, ArgbBuffer& output, float maxHeat);

   /*!
    *\brief Applies a separable Gaussian blur to a heat buffer
    *
    *\param[in] input Source intensity buffer
    *\param[out] output Blurred intensity buffer
    *\param[in] width Buffer width in pixels
    *\param[in] height Buffer height in pixels
    *\param[in] radius Blur radius in pixels
    */
   void GaussianBlur(const HeatBuffer& input, HeatBuffer& output, int32_t width, int32_t height, int32_t radius);

   /*!
    *\brief Returns the maximum value in a heat buffer
    *
    *\param[in] buffer Intensity buffer
    */
   float MaxHeat(const HeatBuffer& buffer);

   /*!
    *\brief Converts a logarithmic slider position into a heat scale factor
    *
    *\param[in] sliderValue Slider position from HeatScaleSliderMin to HeatScaleSliderMax
    */
   float HeatScaleFromSlider(int32_t sliderValue);

   /*!
    *\brief Returns the color-mapping ceiling after applying a heat scale factor
    *
    * Higher scale values lower the ceiling so weaker locations become visible.
    *
    *\param[in] maxHeat Peak intensity in the buffer
    *\param[in] heatScale Scale factor of at least HeatScaleMin
    */
   float ScaledHeatCeiling(float maxHeat, float heatScale);
} // namespace LocationHistory

#endif // HEATMAP_RENDERER_H
