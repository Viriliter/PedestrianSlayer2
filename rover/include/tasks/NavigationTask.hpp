#ifndef NAVIGATIONTASK_HPP
#define NAVIGATIONTASK_HPP

#include <string>
#include <libserial/SerialPort.h>
#include <libserial/SerialStream.h>

#include "config.hpp"
#include "tasks/Task.hpp"
#include "modules/navigation/UBXDriver.hpp"
#include "modules/communication/Message.hpp"
#include "modules/communication/IPCMessage.hpp"
#include "utils/Timing.hpp"
#include "utils/Container.hpp"
#include "utils/Types.hpp"
#include "utils/Miscs.hpp"


using namespace LibSerial;
using namespace navigation;

namespace tasks{
    class NavigationTask: public Task{
        private:
            UBXDriver ubx_driver;

        public:
            NavigationTask(TaskConfig *taskConfig=NULL);
            
        protected:
            virtual void beforeTask() noexcept override;  // overwrite
            virtual void myTask() noexcept override;  // overwrite
            virtual void afterTask() noexcept override;  // overwrite

    };
}


#endif  //NAVIGATIONTASK_HPP