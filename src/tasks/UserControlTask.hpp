#ifndef USERCONTROLTASK_HPP
#define USERCONTROLTASK_HPP

#include "Task.hpp"

#include <string>
#include <sw/redis++/redis++.h>

#include "../utils/timing.h"
#include "../utils/container.h"
#include "../config.hpp"

using namespace sw::redis;

namespace tasks{
    class UserControlTask: public Task{
        private:
            Redis redis = Redis("tcp://127.0.0.1:6379");
        public:
            UserControlTask(std::string task_name, SCHEDULE_POLICY policy, TASK_PRIORITY task_priority, int64_t period_ns=0, int64_t runtime_ns=0, int64_t deadline_ns=0, std::vector<size_t> cpu_affinity={});
            
        protected:
            void beforeTask() override;  // overwrite
            void runTask() override;  // overwrite
            void afterTask() override;  // overwrite
    };
}


#endif  //USERCONTROLTASK_HPP