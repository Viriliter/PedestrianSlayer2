#ifndef NAVIGATIONTASK_HPP
#define NAVIGATIONTASK_HPP

#include <string>
#include <libserial/SerialPort.h>
#include <libserial/SerialStream.h>

#include "tasks/Task.hpp"

#include "config.hpp"
#include "modules/navigation/UBXDriver.hpp"
#include "modules/communication/Message.hpp"
#include "modules/communication/IPCMessage.hpp"
#include "utils/timing.h"
#include "utils/container.h"
#include "utils/types.h"
#include "utils/miscs.h"


using namespace LibSerial;
using namespace navigation;

namespace tasks{
    class NavigationTask: public Task{
        private:
            UBXDriver ubx_driver;

        public:
            NavigationTask(std::string task_name, SCHEDULE_POLICY policy, TASK_PRIORITY task_priority, int64_t period_ns=0, int64_t runtime_ns=0, int64_t deadline_ns=0, std::vector<size_t> cpu_affinity={});
            
        protected:
            void beforeTask() override;  // overwrite
            void runTask() override;  // overwrite
            void afterTask() override;  // overwrite

    };
}


#endif  //NAVIGATIONTASK_HPP