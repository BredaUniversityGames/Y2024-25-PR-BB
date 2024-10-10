#pragma once
#include <chrono>

using DeltaMS = std::chrono::duration<float, std::milli>;

class Stopwatch {
    public:
        Stopwatch();
        DeltaMS GetElapsed() const;
        void Reset();

    private:
        std::chrono::high_resolution_clock::time_point _start;
    };