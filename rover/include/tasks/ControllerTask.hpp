#ifndef CONTROLLERTASK_HPP
#define CONTROLLERTASK_HPP

#include <string>
#include <sw/redis++/redis++.h>

#include "config.hpp"
#include "tasks/Task.hpp"
#include "modules/communication/Message.hpp"
#include "modules/communication/IPCMessage.hpp"
#include "utils/Container.hpp"
#include "utils/Types.hpp"
#include "utils/Miscs.hpp"
#include "modules/modes/Modes.hpp"


using namespace sw::redis;

namespace tasks{
    enum ModeEnums{
        IdleMode,
        StartupMode,
        TestMode,
        UserMode,
        AutoMode,
        TerminationMode
    };

    class ControllerTask: public Task{
    private:
        ModeEnums currentMode;
        ModeEnums lastMode;

        modes::Mode myMode;

        Redis redis = Redis("tcp://127.0.0.1:6379");

    public:
        ControllerTask(TaskConfig *taskConfig=NULL);
            
    protected:
        virtual void beforeTask() noexcept override;  // overwrite
        virtual void myTask() noexcept override;  // overwrite
        virtual void afterTask() noexcept override;  // overwrite
    };
}

#endif  //CONTROLLERTASK_HPP