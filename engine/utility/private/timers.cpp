#include "timers.hpp"

Stopwatch::Stopwatch()
{
    Reset();
}

DeltaMS Stopwatch::GetElapsed() const
{
    return std::chrono::duration_cast<DeltaMS>(std::chrono::high_resolution_clock::now() - _start);
}

void Stopwatch::Reset()
{
    _start = std::chrono::high_resolution_clock::now();
}