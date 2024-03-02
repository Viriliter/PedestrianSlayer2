#include "tasks/Schedulers.hpp"

using namespace tasks;

void OtherScheduler::setAttribute(const sched_attr &attr_) const{
    sched_attr attr = {};
    attr.size = sizeof(attr);

    attr.sched_flags = 0;
    attr.sched_nice = attr_.sched_nice;
    attr.sched_policy = SCHED_OTHER;
    
    auto ret = setSchedAttr(0, &attr, 0);
    if (ret < 0){
        throw std::runtime_error{std::string("failed to setSchedAttr: ") + std::strerror(errno)};
    }
}

void OtherScheduler::sleep(const struct timespec &nextWakeupTime) const noexcept{
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &nextWakeupTime, NULL);
}


void FifoScheduler::setAttribute(const sched_attr &attr_) const{
    sched_attr attr = {};
    attr.size = sizeof(attr);

    attr.sched_flags = 0;
    attr.sched_nice = 0;
    attr.sched_priority = attr_.sched_priority;

    attr.sched_policy = SCHED_FIFO;

    auto ret = setSchedAttr(0, &attr, 0);
    if (ret < 0){
        throw std::runtime_error{std::string("failed to setSchedAttr: ") + std::strerror(errno)};
    }
}

void FifoScheduler::sleep(const struct timespec &nextWakeupTime) const noexcept{
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &nextWakeupTime, NULL);
}


void DeadlineScheduler::setAttribute(const sched_attr &attr_) const{
    sched_attr attr = {};
    attr.size = sizeof(attr);

    attr.sched_flags = 0;
    attr.sched_nice = 0;
    attr.sched_priority = 0;

    attr.sched_policy = SCHED_DEADLINE;
    attr.sched_runtime =  attr_.sched_runtime;
    attr.sched_deadline = attr_.sched_deadline;
    attr.sched_period = attr_.sched_period;

    auto ret = setSchedAttr(0, &attr, 0);
    if (ret < 0){
        throw std::runtime_error{std::string("failed to setSchedAttr: ") + std::strerror(errno)};
    }
}

void DeadlineScheduler::sleep(const struct timespec& /*nextWakeupTime*/) const noexcept{
    sched_yield();
}    


void RRScheduler::setAttribute(const sched_attr &attr_) const{
    sched_attr attr = {};
    attr.size = sizeof(attr);

    attr.sched_flags = 0;
    attr.sched_nice = 0;
    attr.sched_priority = attr_.sched_priority;
    
    attr.sched_policy = SCHED_RR;
    
    auto ret = setSchedAttr(0, &attr, 0);
    if (ret < 0){
        throw std::runtime_error{std::string("failed to setSchedAttr: ") + std::strerror(errno)};
    }
}

void RRScheduler::sleep(const struct timespec &nextWakeupTime) const noexcept{
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &nextWakeupTime, NULL);
}