#ifndef TASKSCHEDULER_HPP
#define TASKSCHEDULER_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <malloc.h>
#include <pthread.h>
#include <semaphore.h>
#include <csignal>

#include <sys/mman.h>
#include <stdexcept>

#include "spdlog/spdlog.h"

#include "config.hpp"
#include "tasks/Task.hpp"
#include "utils/Container.hpp"

// Include Tasks
#include "ControllerTask.hpp"
#include "NavigationTask.hpp"
#include "SlaveCommunicationTask.hpp"
#include "UserControlTask.hpp"

namespace tasks{

    enum SCHEDULER_STATUS{
        Started,
        Stopped
    };

    class TaskScheduler{
        private:
            container::LinkedList<Task*> taskList;

            //Task **taskList = new Task*[TASK_COUNT];
            size_t heap_size_;  //  Size of the heap to reserve in bytes

            void setTaskID(Task &task);

            void setTaskPriority(Task &task);

            void addTask(Task *task);

            void removeTask(Task *task);
            
            void removeTask(int TaskID);

            void joinTask(Task *task);

            void startTask(Task *task);
            
            TASK_STATUS stopTask(Task *task);


        public:
            SCHEDULER_STATUS schedulerStatus;
            TaskScheduler();
            ~TaskScheduler();

            TaskScheduler(const TaskScheduler&) = delete;
            TaskScheduler &operator=(const TaskScheduler&) = delete;

            SCHEDULER_STATUS startScheduler();
            SCHEDULER_STATUS stopScheduler();

            void lockMemory() const;

            void reserveHeap() const;

            void OnTerminationSignal(){
                stopScheduler();
            };
    };

    /**
     * @brief Sets up termination signal handlers which enables the calling of
     * App::OnTerminationSignal when used with
     * cactus_rt::WaitForAndHandleTerminationSignal. Without calling this method,
     * signals will not be caught.
     *
     * To use this, you must also use cactus_rt::WaitForAndHandleTerminationSignal(app)
     * following App::Start. For example (see
     * [signal_handler_example](https://github.com/cactusdynamics/cactus-rt/tree/master/examples/signal_handling_example)
     * for more details):
     *
     *     class MyApp : public cactus_rt::App { ... };
     *     int main() {
     *       cactus_rt::SetUpTerminationSignalHandler();
     *
     *       MyApp app;
     *       app.Start();
     *
     *       cactus_rt::WaitForAndHandleTerminationSignal(app);
     *       return 0;
     *     }
     *
     * When a signal listed in `signals` is sent,
     * cactus_rt::WaitForAndHandleTerminationSignal is unblocked and it calls
     * App::OnTerminationSignal. App::OnTerminationSignal is an user-defined method
     * on `MyApp` that should graceful shutdown the application.
     *
     * Readers familiar with signal handler safety (`man 7 signal-safety`) should
     * note that App::OnTerminationSignal is not a signal handler function, but
     * rather a function that can call any function without restrictions. This
     * should be obvious as it is called from
     * cactus_rt::WaitForAndHandleTerminationSignal on the main thread (in the above
     * case). This is implemented via a semaphore, which is an async-signal-safe
     * method as listed in `signal-safety(7)`.
     *
     * @param signals A vector of signals to catch. Default: SIGINT and SIGTERM.
     */
    void SetUpTerminationSignalHandler(std::vector<int> signals = {SIGINT, SIGTERM});

    /**
     * @brief Wait until a termination signal as setup via
     * cactus_rt::SetUpTerminationSignalHandler is sent, then runs
     * App::OnTerminationSignal. This function returns when
     * App::OnTerminationSignal returns.
     *
     * Calling this function effectively causes the application to run indefinitely
     * until a signal (such as via CTRL+C or kill) is received.
     *
     * This function should only be called from the main function. If this function
     * is called from multiple threads, it may block one of the threads
     * indefinitely.
     *
     * If this fingunction is never called after calling cactus_rt::SetUpTerminationSignalHandler,
     * the signal caught by this application will be ignored.
     *
     * @param app The application object
     */
    void WaitForAndHandleTerminationSignal(TaskScheduler& taskScheduler);
}


#endif  //TASKSCHEDULER_HPP