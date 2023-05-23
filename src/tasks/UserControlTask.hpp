#ifndef USERCONTROLTASK_HPP
#define USERCONTROLTASK_HPP


#include "Task.hpp"

#include "../config.hpp"

namespace tasks{
    class UserControlTask: public Task{
        public:
            UserControlTask(std::string task_name, SCHEDULE_POLICY policy, TASK_PRIORITY task_priority, int64_t period_ns=0, int64_t runtime_ns=0, int64_t deadline_ns=0, std::vector<size_t> cpu_affinity={});
            
        protected:
            void runTask() override;  // overwrite
            void afterTask() override;  // overwrite

    };
}


#endif  //USERCONTROLTASK_HPP