#ifndef MODE_HPP
#define MODE_HPP

#include "../../config.hpp"
#include "../../modules/communication/Message.hpp"
#include "../../utils/container.h"
#include "../../utils/types.h"

namespace modes{
    // Base class
    class Mode{
    public:
        int modeID = 0;

        Mode(){};

        void begin(){}

        virtual void do_job(){}; 

        void end(){}
    };

    class StartupMode: public Mode{
        void do_job() override{
            
        }
    };

    class IdleMode: public Mode{
        void do_job() override{
            
        }
    };

    class TestMode: public Mode{
        void do_job() override{
            
        }
    };

    class UserMode: public Mode{
        void do_job() override{
            
        }
    };

    class AutoMode: public Mode{
        void do_job() override{
            
        }
    };

    class TerminationMode: public Mode{
        void do_job() override{
            
        }
    };

}
#endif  //MODE_HPP