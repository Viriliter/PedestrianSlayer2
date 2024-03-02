#ifndef LATENCY_TRACKER_H
#define LATENCY_TRACKER_H

#include <array>
#include <cstdint>
#include <limits>
#include <queue>

namespace tasks {

class LatencyTracker {
    private:
        double mMin = std::numeric_limits<double>::max();
        double mMean = 0.0;
        double mMax = std::numeric_limits<double>::min();

        std::queue<double> records;
        const std::size_t MAX_RECORD_SIZE = 100;

    public:
        double Min() const noexcept;
        double Mean() const noexcept;
        double Max() const noexcept;

        void recordValue(double v) noexcept;
        void dumpToLogger() const;
};


struct EventCountData{
    uint32_t mTotalEvents = 0;
    uint32_t mDroppedEvents = 0;

    EventCountData() = default;
    EventCountData(uint32_t total, uint32_t dropped): mTotalEvents(total), mDroppedEvents(dropped) {}  
};

}

#endif  // LATENCY_TRACKER_H