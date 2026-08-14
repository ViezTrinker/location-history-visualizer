/*!
 *\file heatmap_renderer_test.cpp
 *\brief Unit tests for heatmap kernels and Gaussian blur
 */

#include "heatmap_renderer.h"

#include <cstdint>

#include <gtest/gtest.h>

TEST(HeatmapRenderer, KernelIsPositiveAtCenterAndSmallFarAway)
{
   const float center = LocationHistory::GaussianKernel(0.0, 8.0);
   const float farAway = LocationHistory::GaussianKernel(80.0, 8.0);
   EXPECT_GT(center, 0.0f);
   EXPECT_GT(center, farAway);
   EXPECT_NEAR(farAway, 0.0f, 1.0e-6f);
}

TEST(HeatmapRenderer, InvalidSigmaReturnsZero)
{
   EXPECT_EQ(LocationHistory::GaussianKernel(1.0, 0.0), 0.0f);
}

TEST(HeatmapRenderer, SpotRaisesCenterIntensity)
{
   constexpr int32_t Width = 64;
   constexpr int32_t Height = 64;
   LocationHistory::HeatBuffer buffer(static_cast<size_t>(Width * Height), 0.0f);
   LocationHistory::AddGaussianSpot(buffer, Width, Height, 32.0, 32.0, 8.0, 1.0f);

   const size_t centerIndex = static_cast<size_t>(32 * Width + 32);
   const size_t cornerIndex = 0;
   EXPECT_GT(buffer[centerIndex], 0.0f);
   EXPECT_GT(buffer[centerIndex], buffer[cornerIndex]);
}

TEST(HeatmapRenderer, AddHeatSampleHitsNearestPixel)
{
   constexpr int32_t Width = 8;
   constexpr int32_t Height = 8;
   LocationHistory::HeatBuffer buffer(static_cast<size_t>(Width * Height), 0.0f);
   LocationHistory::AddHeatSample(buffer, Width, Height, 3.0, 4.0, 2.5f);
   LocationHistory::AddHeatSample(buffer, Width, Height, 3.2, 4.1, 1.0f);
   LocationHistory::AddHeatSample(buffer, Width, Height, -1.0, 0.0, 9.0f);

   const size_t sampleIndex = static_cast<size_t>(4 * Width + 3);
   EXPECT_FLOAT_EQ(buffer[sampleIndex], 3.5f);
   EXPECT_FLOAT_EQ(LocationHistory::MaxHeat(buffer), 3.5f);
}

TEST(HeatmapRenderer, ColorFromHeatZeroIsTransparent)
{
   const uint32_t color = LocationHistory::ColorFromHeat(0.0f);
   EXPECT_EQ((color >> 24) & 0xFFu, 0u);
}

TEST(HeatmapRenderer, ColorFromHeatOneIsOpaqueReddish)
{
   const uint32_t color = LocationHistory::ColorFromHeat(1.0f);
   const uint32_t alpha = (color >> 24) & 0xFFu;
   const uint32_t red = (color >> 16) & 0xFFu;
   const uint32_t green = (color >> 8) & 0xFFu;
   EXPECT_GT(alpha, 0u);
   EXPECT_GT(red, green);
}

TEST(HeatmapRenderer, BlurSpreadsCenterToNeighbors)
{
   constexpr int32_t Width = 16;
   constexpr int32_t Height = 16;
   LocationHistory::HeatBuffer input(static_cast<size_t>(Width * Height), 0.0f);
   const size_t centerIndex = static_cast<size_t>(8 * Width + 8);
   input[centerIndex] = 1.0f;

   LocationHistory::HeatBuffer output;
   LocationHistory::GaussianBlur(input, output, Width, Height, 3);
   ASSERT_EQ(output.size(), input.size());
   EXPECT_GT(output[centerIndex], 0.0f);
   EXPECT_GT(output[centerIndex - 1], 0.0f);
   EXPECT_LT(output[centerIndex], 1.0f);
}

TEST(HeatmapRenderer, MaxHeatFindsPeak)
{
   LocationHistory::HeatBuffer buffer = {0.1f, 2.5f, 0.4f};
   EXPECT_FLOAT_EQ(LocationHistory::MaxHeat(buffer), 2.5f);
}

TEST(HeatmapRenderer, HeatScaleFromSliderIsLogarithmic)
{
   EXPECT_FLOAT_EQ(LocationHistory::HeatScaleFromSlider(LocationHistory::HeatScaleSliderMin), LocationHistory::HeatScaleMin);
   const int32_t sliderMidpoint = LocationHistory::HeatScaleSliderMax / 2;
   EXPECT_NEAR(LocationHistory::HeatScaleFromSlider(sliderMidpoint), 10.0f, 1.0e-5f);
   EXPECT_FLOAT_EQ(LocationHistory::HeatScaleFromSlider(LocationHistory::HeatScaleSliderMax), LocationHistory::HeatScaleMax);
}

TEST(HeatmapRenderer, ScaledHeatCeilingLowersPeak)
{
   EXPECT_FLOAT_EQ(LocationHistory::ScaledHeatCeiling(100.0f, 1.0f), 100.0f);
   EXPECT_FLOAT_EQ(LocationHistory::ScaledHeatCeiling(100.0f, 10.0f), 10.0f);
   EXPECT_FLOAT_EQ(LocationHistory::ScaledHeatCeiling(100.0f, 0.5f), 100.0f);
}

TEST(HeatmapRenderer, HigherScaleSaturatesDominantAndLiftsWeak)
{
   LocationHistory::HeatBuffer buffer = {1.0f, 100.0f};
   LocationHistory::ArgbBuffer linearColors;
   LocationHistory::ArgbBuffer scaledColors;
   LocationHistory::HeatBufferToArgb(buffer, linearColors, LocationHistory::ScaledHeatCeiling(100.0f, 1.0f));
   LocationHistory::HeatBufferToArgb(buffer, scaledColors, LocationHistory::ScaledHeatCeiling(100.0f, 10.0f));

   const uint32_t linearWeakAlpha = (linearColors[0] >> 24) & 0xFFu;
   const uint32_t scaledWeakAlpha = (scaledColors[0] >> 24) & 0xFFu;
   EXPECT_GT(scaledWeakAlpha, linearWeakAlpha);
   EXPECT_EQ(scaledColors[1], LocationHistory::ColorFromHeat(1.0f));
}
