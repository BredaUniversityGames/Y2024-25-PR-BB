#pragma once

#include <chrono>

class Stopwatch {
private:
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
    bool running;

public:
    Stopwatch() : running(false) {}

    void start() {
        if (!running) {
            start_time = std::chrono::high_resolution_clock::now();
            running = true;
        }
    }

    void stop() {
        if (running) {
            end_time = std::chrono::high_resolution_clock::now();
            running = false;
        }
    }

    void reset() {
        running = false;
    }

    float elapsed_seconds() {
        if (running) {
            auto current_time = std::chrono::high_resolution_clock::now();
            return std::chrono::duration<double>(current_time - start_time).count();
        } else {
            return std::chrono::duration<double>(end_time - start_time).count();
        }
    }

    float elapsed_milliseconds() {
        return elapsed_seconds() * 1000;
    }

    float elapsed_microseconds() {
        return elapsed_seconds() * 1000000;
    }
};
