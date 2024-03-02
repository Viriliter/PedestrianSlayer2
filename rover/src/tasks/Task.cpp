#include "tasks/Task.hpp"

using namespace tasks;

unsigned int AbstractTask::CurrentRecursionDepth = 0;
size_t AbstractTask::sTaskID = 0;

AbstractTask::~AbstractTask(){};

void* AbstractTask::run(void* data) noexcept{
    auto* task = static_cast<AbstractTask*>(data);

    sched_attr attr = {.sched_nice = task->mConfig.nice,
                        .sched_priority = task->mConfig.priority,
                        .sched_runtime = task->mConfig.taskRuntimeUs * 1'000,
                        .sched_deadline = task->mConfig.taskDeadlineUs * 1'000,
                        .sched_period = task->mConfig.taskPeriodUs * 1'000};
    
    task->mScheduler->setAttribute(attr);

    task->runTask();
    return nullptr;
}

inline void AbstractTask::sleep(const struct timespec &nextWakeupTime) noexcept{
    mScheduler->sleep(nextWakeupTime);
}

void AbstractTask::requestStop() noexcept {
    mStopRequested.store(true, std::memory_order_relaxed);
}

AbstractTask::AbstractTask(TaskConfig *config){
    mTaskID = ++sTaskID; // Increase taskID on every new task creation
    
    if (config!=NULL){
        memcpy(&mConfig,config,sizeof(mConfig));
    }
    mSchedulePolicy = mConfig.schedulePolicy;
    mPeriodNs = mConfig.taskPeriodUs * 1'000;  // Convert microsecond to nanosecond 
    mCpuAffinity = mConfig.cpuAffinity;
    
    switch(mSchedulePolicy){
        case(SCHEDULE_POLICY::DEADLINE):
            mScheduler = std::make_shared<DeadlineScheduler>();
            break;
        case(SCHEDULE_POLICY::FIFO):
            mScheduler = std::make_shared<FifoScheduler>();
            break;
        case(SCHEDULE_POLICY::RR):
            mScheduler = std::make_shared<RRScheduler>();
            break;
        case(SCHEDULE_POLICY::OTHER):
        default:            
            mScheduler = std::make_shared<OtherScheduler>();
            break;
    }
}

size_t AbstractTask::taskID(){
        return mTaskID;
}

void AbstractTask::start(){
    pthread_attr_t attr;

    // Initialize the pthread attribute
    int ret = pthread_attr_init(&attr);
    if (ret != 0) {
        throw std::runtime_error(std::string("error in pthread_attr_init: ") + std::strerror(ret));
    }

    // Set a stack size
    ret = pthread_attr_setstacksize(&attr, mStackSize);
    if (ret != 0) {
        throw std::runtime_error(std::string("error in pthread_attr_setstacksize: ") + std::strerror(ret));
    }

    // Setting CPU mask
    if (!mCpuAffinity.empty()) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        for (auto cpu : mCpuAffinity) {
            CPU_SET(cpu, &cpuset);
        }

        ret = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
        if (ret != 0) {
            throw std::runtime_error(std::string("error in pthread_attr_setaffinity_np: ") + std::strerror(ret));
        }
    }

    // Create a new thread
    ret = pthread_create(&mTask, &attr, &AbstractTask::run, this);
    if (ret != 0) {
        throw std::runtime_error(std::string("error in pthread_create: ") + std::strerror(ret));
    }

};

int AbstractTask::join() noexcept{
    return pthread_join(mTask, nullptr);
};

inline bool AbstractTask::isStopRequested() const noexcept{
    return mStopRequested.load(std::memory_order_relaxed);
}

void AbstractTask::setTaskStatus(TASK_STATUS taskStatus) noexcept{
    mTaskStatus = taskStatus;
}

TASK_STATUS AbstractTask::TaskStatus() noexcept{
    return mTaskStatus;
}

void AbstractTask::terminateTask() noexcept{
    this->requestStop();

    // Wait to stop task 
    //while(mTaskStatus != TASK_STATUS::STOPPED_TASK){};
}


void Task::runTask() noexcept{
    setTaskStatus(TASK_STATUS::RUNNING_TASK);

    clock_gettime(CLOCK_MONOTONIC, &mNextWakeupTime);

    beforeTask();
    
    auto last_data_write_time = timing::NowNs();

    while(true){
        if (this->isStopRequested()) {
            break;
        }

        auto now = timing::NowNs();

        this->myTask();
    
        mNextWakeupTime = timing::AddTimespecByNs(mNextWakeupTime, mPeriodNs);
        
        // Second check is not to wait sleep function is to be elapsed. 
        if (this->isStopRequested()) {
            break;
        }
        this->sleep(mNextWakeupTime);
    }

    afterTask();

    setTaskStatus(TASK_STATUS::STOPPED_TASK);
}

Task::Task(TaskConfig *config): AbstractTask(config){ };

Task::~Task(){
    this->terminateTask();
};

void Task::beforeTask() noexcept{
    auto msg = "TaskID-" + std::to_string(taskID()) + " is about to start.";
    //Logging::getInstance()->writeLogsToConsole(logLevelEnum::Info, msg);
}

void Task::myTask() noexcept{}

void Task::afterTask() noexcept {
    auto msg = "TaskID-" + std::to_string(taskID()) + " is about to end.";
    //Logging::getInstance()->writeLogsToConsole(logLevelEnum::Info, msg);
}


void CyclicTask::runTask() noexcept{
    setTaskStatus(TASK_STATUS::RUNNING_TASK);
    clock_gettime(CLOCK_MONOTONIC, &mNextWakeupTime);
    
    int64_t nextWakeupTimeNs =  mNextWakeupTime.tv_sec * 1'000'000'000 + mNextWakeupTime.tv_nsec;
    int64_t loopStart, loopEnd;
    int64_t wakeupLatency, loopLatency;
    int64_t totalLatencyNs;

    beforeTask();

    while (true) {
        if(this->isStopRequested()){
            break;
        }

        loopStart = timing::NowNs();

        myTask();

        // Second check is not to wait sleep function is to be elapsed. 
        if(this->isStopRequested()){
            break;
        }

        loopEnd = timing::NowNs();

        wakeupLatency = enableWakeupLatencyTracer? loopStart - nextWakeupTimeNs: 0;
        loopLatency = enableLoopTracer? loopEnd - loopStart: 0;

        mWakeupLatencyTracker.recordValue(wakeupLatency);
        mLoopLatencyTracker.recordValue(loopLatency);

        totalLatencyNs = mWakeupLatencyTracker.Mean() +  mLoopLatencyTracker.Mean();

        if (static_cast<uint64_t>(wakeupLatency + loopLatency) >= mPeriodNs){
            if(enableTraceOverrun){
                auto msg = "Loop overrun is detected. Wakeup Latency[ns]: " + std::to_string(wakeupLatency) + ", Loop Latency[ns]:" + std::to_string(loopLatency) + ", Period[ns]:" + std::to_string(mPeriodNs);
                //Logging::getInstance()->writeLogsToConsole(logLevelEnum::Critical, msg);
            }
            clock_gettime(CLOCK_MONOTONIC, &mNextWakeupTime);
            nextWakeupTimeNs =  mNextWakeupTime.tv_sec * 1'000'000'000 + mNextWakeupTime.tv_nsec;
        }
        else{
            mNextWakeupTime =  timing::AddTimespecByNs(mNextWakeupTime, static_cast<int64_t>(mPeriodNs));
            nextWakeupTimeNs =  mNextWakeupTime.tv_sec * 1'000'000'000 + mNextWakeupTime.tv_nsec;
        }
        
        mNextWakeupTime = timing::SubtTimespecByNs(mNextWakeupTime, totalLatencyNs);

        this->sleep(mNextWakeupTime);
    }

    afterTask();

    setTaskStatus(TASK_STATUS::STOPPED_TASK);
}

CyclicTask::CyclicTask(TaskConfig *config): AbstractTask(config){
    if (config != NULL){
        enableWakeupLatencyTracer = config->enableWakeupLatencyTracer;
        enableLoopTracer = config->enableLoopTracer;
        enableTraceOverrun = config->enableTraceOverrun;
    }
}

CyclicTask::~CyclicTask(){
    this->terminateTask();
};

void CyclicTask::beforeTask() noexcept {}

void CyclicTask::myTask() noexcept {}

void CyclicTask::afterTask() noexcept {}