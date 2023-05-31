#ifndef CONTROLLERTASK_HPP
#define CONTROLLERTASK_HPP

#include "Task.hpp"

#include "../config.hpp"
#include "../modules/communication/messages/Message.hpp"
#include "../utils/container.h"

namespace tasks{
    class ControllerTask: public Task{
        public:
        public:
            ControllerTask(std::string task_name, SCHEDULE_POLICY policy, TASK_PRIORITY task_priority, int64_t period_ns=0, int64_t runtime_ns=0, int64_t deadline_ns=0, std::vector<size_t> cpu_affinity={});
            
        protected:
            void beforeTask() override;  // overwrite
            void runTask() override;  // overwrite
            void afterTask() override;  // overwrite
    };
}

#endif  //CONTROLLERTASK_HPP