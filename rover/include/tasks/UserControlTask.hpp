#ifndef USERCONTROLTASK_HPP
#define USERCONTROLTASK_HPP

#include <string>
#include <sw/redis++/redis++.h>

#include "config.hpp"
#include "tasks/Task.hpp"
#include "utils/Timing.hpp"
#include "utils/Container.hpp"

using namespace sw::redis;

namespace tasks{
    class UserControlTask: public Task{
        private:
            Redis redis = Redis("tcp://127.0.0.1:6379");
        public:
            UserControlTask(TaskConfig *taskConfig=NULL);
            
        protected:
            virtual void beforeTask() noexcept override;  // overwrite
            virtual void myTask() noexcept override;  // overwrite
            virtual void afterTask() noexcept override;  // overwrite
    };
}


#endif  //USERCONTROLTASK_HPP