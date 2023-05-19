#include "TaskScheduler.hpp"

using namespace tasks;

TaskScheduler::TaskScheduler(){
    // Create Tasks
    //taskID | taskPeriod | taskPriority | estimatedTaskDuration | rtEnabled | cpuAffinityEnabled
    //NavigationTask *navigationTask = new NavigationTask<cactus_rt::schedulers::Other>();
    //RoverDriveTask *roverDriveTask = new RoverDriveTask<cactus_rt::schedulers::Other>();
    SlaveCommunicationTask *slaveCommunicationTask = new SlaveCommunicationTask("SlaveCommunicationTask", SCHEDULE_POLICY::OTHER, TASK_PRIORITY::RT_NORMAL_PRIORITY, timing::milisecondsToNanoseconds(10));
    //UserControlTask *userControlTask = new UserControlTask<cactus_rt::schedulers::Other>();
    
    // Add tasks into list
    
    //addTask(navigationTask);
    //addTask(roverDriveTask);
    addTask(slaveCommunicationTask);
    //addTask(userControlTask);
    std::cout << slaveCommunicationTask->taskID() << std::endl;
    
};

TaskScheduler::~TaskScheduler(){
    // Stops tasks in reverse order (Last added task removes first)
    SCHEDULER_STATUS schedulerStatus = stopScheduler();
    /*
    for(int i=taskList.Count()-1; i>=0; i--){
        std::cout << "removing tasks... " << std::to_string(i) << std::endl;

        stopTask(taskList[i]);
    }
    */

    // Clear tasks
    //taskList.clear();
    //delete[] taskList;
};

SCHEDULER_STATUS TaskScheduler::startScheduler(){
    //lockMemory();
    //reserveHeap();

    bool flag = true;
    // Start tasks
    for(int i=0; i<taskList.Count(); i++){
        Task *ptrTask = taskList[i];
        if (ptrTask == NULL) continue;
        
        auto start_monotonic_time_ns = timing::NowNs();
        auto start_wall_time_ns = timing::WallNowNs();
        SPDLOG_INFO("Task is starting");

        taskList[i]->start(start_monotonic_time_ns, start_wall_time_ns);
        SPDLOG_INFO("Task started");

    }
    // Join tasks
    for(int i=0; i<taskList.Count(); i++){
        Task *ptrTask = taskList[i]; 
        if (ptrTask == NULL) continue;
        
        int result = ptrTask->join();
        std::cout << "Join Result: " << result  << std::endl; 
    }

    schedulerStatus = (flag)? SCHEDULER_STATUS::Started: schedulerStatus;
    return schedulerStatus;
};

SCHEDULER_STATUS TaskScheduler::stopScheduler(){
    bool flag = true;
    // Stops tasks in reverse order (Last added task stops first)
    for(int i=taskList.Count()-1; i>=0; i--){
        if (taskList[i] == NULL) continue;

        flag &= (bool) (stopTask(taskList[i]) != TASK_STATUS::RUNNING_TASK);
    }
    schedulerStatus = (flag)? SCHEDULER_STATUS::Stopped: schedulerStatus;
    SPDLOG_INFO("All tasks stopped");

    return schedulerStatus;
};

void TaskScheduler::startTask(Task *task){
    if (task == NULL) return;
        
    auto start_monotonic_time_ns = timing::NowNs();
    auto start_wall_time_ns = timing::WallNowNs();
    task->start(start_monotonic_time_ns, start_wall_time_ns);
};

TASK_STATUS TaskScheduler::stopTask(Task *task){
    TASK_STATUS result = TASK_STATUS::IDLE_TASK;

    for(int i=0; i<taskList.Count(); i++){
        //Check whether task is NULL 
        if (taskList[i] == task) {
            Task *ptrTask = taskList[i]; 

            ptrTask->RequestStop();
            timing::sleep(100, timing::timeFormat::formatMiliseconds);
            result = ptrTask->StopRequested()? TASK_STATUS::STOPPED_TASK: TASK_STATUS::RUNNING_TASK;
            break;
        }
    };

    return result;   
};

void TaskScheduler::addTask(Task *task){
    taskList.add(task);
    /*
    if (availableTaskSpace<0) throw "Task list is full. Either delete unncessary tasks or increase TASK_COUNT";
    for(int i=0; i<taskList.Count(); i++){
        //Check whether task is NULL and add to NULL pointer
        taskList.add(task);
        
        if (taskList[i] == NULL) {
            taskList[i] = task;
            break;
        }
        
    }
    availableTaskSpace--;
    */
};

void TaskScheduler::removeTask(Task *task){
    taskList.remove(task);
};

void TaskScheduler::removeTask(int taskID){
    //taskList.remove(taskID)
};

void TaskScheduler::joinTask(Task *task){
    if (task == NULL) return;

    task->join();
};

void TaskScheduler::reserveHeap() const {
    if (heap_size_ == 0) return;

    SPDLOG_INFO("reserving {} bytes of heap memory", heap_size_);

    void* buf = malloc(heap_size_);
    if (buf == nullptr) {
        SPDLOG_ERROR("cannot malloc: {}", std::strerror(errno));
        throw std::runtime_error{"cannot malloc"};
    }

    // There is no need the poke each page in the buffer to ensure that the page
    // is actually allocated, because mlockall effectively turns off demand
    // paging. See mlockall(2) and "demand paging" on Wikipedia. Also see:
    // https://github.com/ros2-realtime-demo/pendulum/issues/90#issuecomment-1105844726
    free(buf);
}

void TaskScheduler::lockMemory() const{
    // See https://lwn.net/Articles/837019/

    // From the man page:
    //
    // Locks all pages mapped into the address space, including code, data, stack
    // segments, shared libraries, user space kernel data, shared memory, and
    // memory mapped files.
    //
    // All mapped pages are guaranteed to be resident in RAM when the call
    // returns successfully; the pages are guaranteed in RAM until later
    // unlocked.
    //
    // MCL_CURRENT: Lock all pages which are currently mapped into the address
    //              space of the process.
    //
    // MCL_FUTURE: Lock all pages which will become mapped into the address space
    //             of the process in the future. These could be, for instance,
    //             new pages required by a growing heap and stack as well as new
    //             memory mapped files and shared memory regions.
    int ret = mlockall(MCL_CURRENT | MCL_FUTURE);
    if (ret != 0) {
        SPDLOG_ERROR("mlockall failed: {}", std::strerror(errno));
        throw std::runtime_error{"mlockall failed"};
    }

    // Do not free any RAM to the OS if the continguous free memory at the top of
    // the heap grows too large. If RAM is freed, a syscall (sbrk) will be called
    // which can have unbounded execution time.
    ret = mallopt(M_TRIM_THRESHOLD, -1);
    if (ret == 0) {
        // on error, errno is not set by mallopt
        throw std::runtime_error{"mallopt M_TRIM_THRESHOLD failed"};
    }

    // Do not allow mmap.
    // TODO: give example why this is bad.
    ret = mallopt(M_MMAP_MAX, 0);
    if (ret == 0) {
        throw std::runtime_error{"mallopt M_TRIM_THRESHOLD failed"};
    }
}

sem_t signal_semaphore;

void HandleSignal(int /*sig*/) {
    // From the man page (sem_post(3)), it says:
    //
    // > sem_post() is async-signal-safe: it may be safely called within a
    // > signal handler.
    //
    // This allows it to be used for signaling for an async signal handler. This
    // is also according to Programming with POSIX Threads by Butenhof in 1997,
    // section 6.6.6.
    //
    // However, the situation is more complex, see https://stackoverflow.com/questions/48584862/sem-post-signal-handlers-and-undefined-behavior.
    // That said, overall this should be a good pattern to use.

    // write(STDERR_FILENO, "synchronous signal handler active\n", 34);
    int ret = sem_post(&signal_semaphore);
    if (ret != 0) {
    write(STDERR_FILENO, "failed to post semaphore\n", 25);
    std::_Exit(EXIT_FAILURE);
    }
    }

    void SetUpTerminationSignalHandler(std::vector<int> signals) {
    int ret = sem_init(&signal_semaphore, 0, 0);
    if (ret != 0) {
    throw std::runtime_error{std::string("cannot initialize semaphore: ") + std::strerror(errno)};
    }

    for (auto signal : signals) {
    auto sig_ret = std::signal(signal, HandleSignal);
    if (sig_ret == SIG_ERR) {
        throw std::runtime_error("failed to register signal handler");
    }
    }
};

void WaitForAndHandleTerminationSignal(TaskScheduler& taskScheduler) {
    // This function is not a part of the real signal handler. The real signal
    // handler (HandleSignal) posts to the semaphore, which unblocks this
    // function. This function then calls taskScheduler.OnTerminationSignal() to allow for
    // graceful shutdown. Since this function is not executed as a signal handler,
    // it can call any arbitrary function.
    while (sem_wait(&signal_semaphore) == -1) {
    }

    taskScheduler.OnTerminationSignal();
};