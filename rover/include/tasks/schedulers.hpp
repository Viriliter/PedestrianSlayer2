#ifndef SCHEDULERS_HPP
#define SCHEDULERS_HPP

#include <sched.h>
#include <spdlog/spdlog.h>

#include <cerrno>
#include <ctime>

#include "utils/threading/sched_ext.h"

constexpr size_t kDefaultStackSize = 8 * 1024 * 1024;  // 8MB

// Earliest Deadline First scheduler. More information on this can be found here:
// https://docs.kernel.org/scheduler/sched-deadline.html

namespace schedulers {
    struct Config {
        // For Deadline settings
        uint64_t runtime_ns = 0;
        uint64_t deadline_ns = 0;
        uint64_t period_ns = 0;
        // For Fifo settings
        uint32_t priority = 80;
        // For Other settings
        int32_t nice = 0;
    };

}  // namespace schedulers

namespace policies {
    class Policy{};

    class Deadline: public Policy {
    public:
        inline static void SetThreadScheduling(const schedulers::Config& config) {
            // Self scheduling attributes
            sched_attr attr = {};
            attr.size = sizeof(attr);
            attr.sched_flags = 0;
            attr.sched_nice = 0;
            attr.sched_priority = 0;

            attr.sched_policy = SCHED_DEADLINE;  // Set the scheduler policy
            attr.sched_runtime = config.runtime_ns;
            attr.sched_deadline = config.deadline_ns;
            attr.sched_period = config.period_ns;

            auto ret = sched_setattr(0, &attr, 0);
            if (ret < 0) {
            SPDLOG_ERROR("unable to sched_setattr: {}", std::strerror(errno));
            throw std::runtime_error{"failed to sched_setattr"};
            }
        }

        inline static double Sleep(const struct timespec& /*next_wakeup_time */) noexcept {
            // Ignoring error as man page says "In the Linux implementation, sched_yield() always succeeds."
            sched_yield();
            return 0.0;
        }
    };

    class Fifo: public Policy {
    public:
        inline static void SetThreadScheduling(const schedulers::Config& config) {
            // Self scheduling attributes
            sched_attr attr = {};
            attr.size = sizeof(attr);
            attr.sched_flags = 0;
            attr.sched_nice = 0;
            attr.sched_priority = config.priority;  // Set the scheduler priority
            attr.sched_policy = SCHED_FIFO;         // Set the scheduler policy

            auto ret = sched_setattr(0, &attr, 0);
            if (ret < 0) {
                SPDLOG_ERROR("unable to sched_setattr: {}", std::strerror(errno));
                throw std::runtime_error{"failed to sched_setattr"};
            }
        }

        inline static double Sleep(const struct timespec& next_wakeup_time) noexcept {
            // TODO: check for errors?
            // TODO: can there be remainders?
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_wakeup_time, nullptr);
            return 0.0;
        }
    };

    class Other: public Policy {
    public:
        inline static void SetThreadScheduling(const schedulers::Config& config) {
            // Self scheduling attributes
            sched_attr attr = {};
            attr.size = sizeof(attr);
            attr.sched_flags = 0;
            attr.sched_nice = config.nice;    // Set the thread niceness
            attr.sched_policy = SCHED_OTHER;  // Set the scheduler policy

            auto ret = sched_setattr(0, &attr, 0);
            if (ret < 0) {
            SPDLOG_ERROR("unable to sched_setattr: {}", std::strerror(errno));
            throw std::runtime_error{"failed to sched_setattr"};
            }
        }

        // Kind of meaningless for SCHED_OTHER because there's no RT guarantees
        inline static double Sleep(const struct timespec& next_wakeup_time) noexcept {
            // TODO: check for errors?
            // TODO: can there be remainders?
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_wakeup_time, nullptr);
            return 0.0;
        }
    };
}  // namespace policy

#endif  // SCHEDULERS_HPP