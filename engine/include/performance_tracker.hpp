#pragma once

class PerformanceTracker
{
public:
    PerformanceTracker();
    void Update();
    void Render();

private:
    static const uint32_t MAX_SAMPLES { 512 };

    std::vector<float> _fpsValues;

    std::vector<float> _frameDurations;
    std::vector<std::string> _labels;

    std::vector<float> _timePoints;
    std::chrono::steady_clock::time_point _lastFrameTime;
    float _totalTime;
    uint32_t _frameCounter { 0 };

    float _highestFps;
    uint32_t _highestFpsRecordIndex;
    float _highestFrameDuration;
    uint32_t _highestFrameDurationRecordIndex;
};