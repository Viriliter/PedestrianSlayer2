#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <cstring>
#include <ctime>
#include <cstdint>
#include <iostream>

#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdexcept>

#include <unistd.h>

namespace tasks {

struct sched_attr {
  uint32_t size = 0;

  uint32_t sched_policy = 0;
  uint64_t sched_flags = 0;

  /* SCHED_NORMAL, SCHED_BATCH */
  int32_t sched_nice = 0;

  /* SCHED_FIFO, SCHED_RR */
  uint32_t sched_priority = 0;

  /* SCHED_DEADLINE (nsec) */
  uint64_t sched_runtime = 0;
  uint64_t sched_deadline = 0;
  uint64_t sched_period = 0;
};

class Scheduler{
    public:
        virtual ~Scheduler() = default;

        virtual void setAttribute(const sched_attr &attr_) const = 0;

        virtual void sleep(const timespec &nextWakeupTime) const noexcept = 0;

};

inline long setSchedAttr(pid_t pid, const struct sched_attr *attr, unsigned int flags) {
    auto cUser = getuid();
    //if (cUser != 0) return 0;  // Application is not running as root
            
    auto r = syscall(SYS_sched_setattr, pid, attr, flags);
    return r;
}

inline long getSchedAttr(pid_t pid, struct sched_attr *attr, unsigned int size, unsigned int flags) {
  return syscall(SYS_sched_getattr, pid, attr, size, flags);
}

class OtherScheduler : public Scheduler{
    public:
        void setAttribute(const sched_attr &attr_) const override;

        void sleep(const struct timespec &nextWakeupTime) const noexcept override;

};

class FifoScheduler : public Scheduler{
    public:
        void setAttribute(const sched_attr &attr_) const override;

        void sleep(const struct timespec &nextWakeupTime) const noexcept override;
};

class DeadlineScheduler : public Scheduler{
    public:
        void setAttribute(const sched_attr &attr_) const override;

        void sleep(const struct timespec& /*nextWakeupTime*/) const noexcept override;
};

class RRScheduler: public Scheduler{
    public:
        void setAttribute(const sched_attr &attr_) const override;

        void sleep(const struct timespec &nextWakeupTime) const noexcept override;
};

}

#endif  // SCHEDULER_H
