#include "tasks/LatencyTracker.hpp"

using namespace tasks;

double LatencyTracker::Min() const noexcept { return mMin; }

double LatencyTracker::Mean() const noexcept { return mMean; }

double LatencyTracker::Max() const noexcept { return mMax; }

void LatencyTracker::recordValue(double v) noexcept {
    if (v < mMin) {
        mMin = v;
    }

    if (v > mMax) {
        mMax = v;
    }
    
    records.push(v);
    if (records.size()>=MAX_RECORD_SIZE) records.pop();

    std::queue<double> temp_records = records;
    auto count = records.size();
    double sum;
    
    while(!temp_records.empty()){
        sum += temp_records.front();
        temp_records.pop();
    }

    mMean = (count>0)? (sum / count): 0;
}

void LatencyTracker::dumpToLogger() const {
    //spdlog::set_level(spdlog::level::debug);
    //SPDLOG_DEBUG("min: {:.4f} | mean: {:.4f} | max: {:.4f} | count: {}", min_, mean_, max_, count_);
}