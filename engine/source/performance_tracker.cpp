#include "performance_tracker.hpp"
#include "imgui.h"
#include "implot.h"
#include <algorithm>
#include <iostream>
#include <string>

#include "tracy/Tracy.hpp"

PerformanceTracker::PerformanceTracker()
{
    _totalTime = 0;
    _fpsValues.reserve(MAX_SAMPLES);
    _frameDurations.reserve(MAX_SAMPLES);
    _timePoints.reserve(MAX_SAMPLES);
    _lastFrameTime = std::chrono::steady_clock::now();
}

void PerformanceTracker::Update()
{
    ZoneScoped;
    auto currentTime = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - _lastFrameTime).count();
    _lastFrameTime = currentTime;

    float fps = 1.0f / deltaTime;
    float frameDuration = deltaTime * 1000.0f;
    _totalTime += deltaTime;

    if(_frameCounter < 4)
    {
        ++_frameCounter;
        return;
    }

    if(fps >= _highestFps)
    {
        _highestFps = fps;
        _highestFpsRecordIndex = _frameCounter;
    }
    if(frameDuration >= _highestFrameDuration)
    {
        _highestFrameDuration = frameDuration;
        _highestFrameDurationRecordIndex = _frameCounter;
    }
    if(_frameCounter - _highestFpsRecordIndex > MAX_SAMPLES)
    {
        auto it = std::max_element(_fpsValues.begin(), _fpsValues.end());
        _highestFps = *it;
        _highestFpsRecordIndex = _frameCounter - std::distance(_fpsValues.begin(), it);
    }
    if(_frameCounter - _highestFrameDurationRecordIndex > MAX_SAMPLES)
    {
        auto it = std::max_element(_frameDurations.begin(), _frameDurations.end());
        _highestFrameDuration = *it;
        _highestFrameDurationRecordIndex = _frameCounter - std::distance(_frameDurations.begin(), it);
    }

    _fpsValues.emplace_back(fps);
    _frameDurations.emplace_back(frameDuration);

    _timePoints.emplace_back(_totalTime);

    if(_fpsValues.size() > MAX_SAMPLES)
    {
        _fpsValues.erase(_fpsValues.begin());
        _frameDurations.erase(_frameDurations.begin());
        _timePoints.erase(_timePoints.begin());
    }

    ++_frameCounter;
}

void PerformanceTracker::Render()
{
    ZoneScoped;

    if(_timePoints.empty())
        return;

    ImGui::Begin("Performance metrics");

    if(ImPlot::BeginPlot("FPS"))
    {
        ImPlot::SetupAxes("Time (s)", "Value", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxisLimits(ImAxis_X1, _timePoints.front(), _timePoints.back(), ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, _highestFps * 1.05f, ImGuiCond_Always);

        ImPlot::PushStyleColor(ImPlotCol_Line, 0xFF24ac3d);
        ImPlot::PlotLine("FPS", _timePoints.data(), _fpsValues.data(), _fpsValues.size());
        ImPlot::PopStyleColor();

        ImPlot::EndPlot();
    }
    if(ImPlot::BeginPlot("Frame Duration"))
    {
        ImPlot::SetupAxes("Time (s)", "Value (ms)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxisLimits(ImAxis_X1, _timePoints.front(), _timePoints.back(), ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, _highestFrameDuration * 1.05f, ImGuiCond_Always);

        ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
        ImPlot::PlotShaded("Frame Duration (ms)", _timePoints.data(), _frameDurations.data(), _frameDurations.size());

        ImPlot::PlotLine("Frame Duration (ms)", _timePoints.data(), _frameDurations.data(), _frameDurations.size());



        ImPlot::EndPlot();
    }

    ImGui::End();
}
