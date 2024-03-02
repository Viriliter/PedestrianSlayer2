#ifndef TASK_H
#define TASK_H

#include <pthread.h>
#include <sys/mman.h>  // necessary for mlockall
#include <mqueue.h>
#include <bits/local_lim.h>  // It is neccessary to be seen PTHREAD_STACK_MIN macro by some compilers 

#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <vector>
#include <iostream>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <atomic>
#include <ctime>

//#include "LogManager/Logging.hpp"

#include "config.hpp"
#include "utils/Timing.hpp"
#include "tasks/LatencyTracker.hpp"
#include "tasks/Schedulers.hpp"

namespace tasks {

const unsigned int MAX_RECURSION_LIMIT = 100;

/**
 * @brief This enum contains task priorties.
*/
enum class TASK_PRIORITY{
    RT_HIGH_PRIORITY,
    RT_NORMAL_PRIORITY,
    RT_LOW_PRIORITY,
    HIGH_PRIORITY,
    NORMAL_PRIORITY,
    LOW_PRIORITY,
};

/**
 * @brief This enum contains statuses of task.
*/
enum class TASK_STATUS{
    IDLE_TASK,
    RUNNING_TASK,
    STOPPED_TASK,
};

/**
 * @brief This enum contains schedule policies.
*/
enum class SCHEDULE_POLICY{
    DEADLINE,
    FIFO,
    OTHER,
    RR
};

constexpr size_t kDefaultStackSize = 8 * 1024 * 1024;  // 8MB for stack size

struct TaskConfig{
    SCHEDULE_POLICY schedulePolicy = SCHEDULE_POLICY::OTHER;  ///> Schedule Policy (SCHEDULE_POLICY::OTHER is non-rt scheduler)
    
    uint64_t taskPeriodUs = 0;  ///> Execution period between tasks in microseconds
    uint64_t taskDeadlineUs = 0;  ///> Deadline period for task in microseconds (Applicable for SCHEDULE_POLICY::DEADLINE)
    uint64_t taskRuntimeUs = 0;  ///> Runtime period for task in microseconds (Applicable for SCHEDULE_POLICY::DEADLINE)
    int32_t nice = 19;  ///> Nice value of process (Applicable only for SCHEDULE_POLICY::OTHER) [-20 (highest prio), 19(lowest prio)]
    uint32_t priority = 1;  ///> Priority of process (Applicable for SCHEDULE_POLICY::FIFO, SCHEDULE_POLICY::RR, and SCHEDULE_POLICY::DEADLINE) [1 (lowest prio), 99(highest prio)]

    std::vector<size_t> cpuAffinity = {};  // List of the cpu cores that will be used for the task

    bool enableWakeupLatencyTracer = true;  // Traces wakeup latency  (Applicable for CyclicTask)
    bool enableLoopTracer = true;  // Traces loop latency (Applicable for CyclicTask)
    bool enableTraceOverrun = true;  // Traces if total latency exceeds the task period (Applicable for CyclicTask)
};

/**
 * @class AbstractTask
 * 
 * @brief This abstract class provides the base functionality for POSIX threadding.
 * 
 * It is the base class for Task and CyclicTask, and contains all common functionalities of these two classes.
*/
class AbstractTask{
    private:
        static size_t sTaskID;
        size_t mTaskID = 0;
        TASK_STATUS mTaskStatus = TASK_STATUS::IDLE_TASK;
        std::atomic<bool> mStopRequested{false};
        static unsigned int CurrentRecursionDepth;

        pthread_t mTask;
       
        /**
         * @brief This static function is for pthread application.
         * 
         * @param[in] data A pointer of AbstractTask instance 
        */
        static void* run(void* data) noexcept;

    protected:
        TASK_PRIORITY mTaskPriority = TASK_PRIORITY::NORMAL_PRIORITY;
        SCHEDULE_POLICY mSchedulePolicy = SCHEDULE_POLICY::OTHER;
        size_t mStackSize = (static_cast<size_t>(PTHREAD_STACK_MIN) + kDefaultStackSize);
        std::vector<size_t> mCpuAffinity = {};
        mqd_t mq_port_in;
        mqd_t mq_port_out;
        
        std::shared_ptr<Scheduler> mScheduler;
        TaskConfig mConfig;

        int64_t mPeriodNs = 1'000'000'000;  // 1s
        struct timespec mNextWakeupTime;

        /**
         * @brief This function halts the execution.
         * @param[in] nextWakeupTime Timespec when execution continue  
        */
        inline void sleep(const struct timespec &nextWakeupTime) noexcept;

        /**
         * @brief This function request stop for the task.
        */
        void requestStop() noexcept;

        bool createMsgQueue(const char *msgQueueName, mq_attr _attr_in, mq_attr _attr_out){
            size_t lenQueueName = strlen(msgQueueName);
            char *msgQueueNameIn = new char[lenQueueName+4];
            char *msgQueueNameOut = new char[lenQueueName+4];
            std::strcpy(msgQueueNameIn, msgQueueName);
            std::strcat(msgQueueNameIn, "_in");
            std::strcpy(msgQueueNameOut, msgQueueName);
            std::strcat(msgQueueNameOut, "_out");
            mq_port_in = mq_open(msgQueueNameIn, O_CREAT|O_RDWR|O_NONBLOCK, 0744, &_attr_in);
            mq_port_out = mq_open(msgQueueNameOut, O_CREAT|O_RDWR|O_NONBLOCK, 0744, &_attr_out);
            delete[] msgQueueNameIn;
            delete[] msgQueueNameOut;

            //std::cout << mq_port_in << std::endl;
            /*
            if (mq_port_in != 3){
                int errvalue = errno;
                std::cout << "The error generated was " << std::to_string(errvalue) << " in createMsgQueue(in)"<< std::endl;
                std::cout << "That means: " << strerror( errvalue ) << std::endl;
            }*/
            if (mq_port_in != 3){
                int errvalue = errno;
                std::string nn;
                for(int i=0; i<strlen(msgQueueName); i++) nn += msgQueueName[i]; 
                std::cout << "The error generated was " << std::to_string(errvalue) << " in createMsgQueue - " << nn << std::endl;
                std::cout << "That means: " << strerror( errvalue ) << std::endl;
            }
            if (mq_port_out != 3){
                int errvalue = errno;
                std::string nn;
                for(int i=0; i<strlen(msgQueueName); i++) nn += msgQueueName[i]; 
                std::cout << "The error generated was " << std::to_string(errvalue) << " in createMsgQueue - " << nn << std::endl;
                std::cout << "That means: " << strerror( errvalue ) << std::endl;
            }
            return (bool) (mq_port_in == 3) && (mq_port_out == 3);
        };

        uint16_t readMsgQueue(const char *msgQueueName, char *queue){
            uint16_t queueSize = 0;
            mq_attr mq_attr_;
            mqd_t mqd_t_ = mq_open(msgQueueName, O_RDONLY);
            
            
            if (mq_getattr(mqd_t_, &mq_attr_) == -1) {
                int errvalue = errno;
                std::string nn;
                for(int i=0; i<strlen(msgQueueName); i++) nn += msgQueueName[i]; 
                std::cout << "The error generated was " << std::to_string(errvalue) << " in readMsgQueue - " << nn << std::endl;
                std::cout << "That means: " << strerror( errvalue ) << std::endl;
                return 0;
            }
            
            // Check whether there is any available message in the queue
            if (mq_attr_.mq_curmsgs<=0) return 0;

            size_t ret_rec = mq_receive(mqd_t_, queue, MAX_MQ_MSG_SIZE+1, NULL);
            mq_close(mqd_t_);

            /*
            struct timespec tm;
            clock_gettime(CLOCK_REALTIME, &tm);
            tm.tv_nsec += 1*1000000;  // Set for 1 miliseconds
            size_t ret_rec = mq_timedreceive(mqd_t, queue, MAX_MQ_MSG_SIZE+1, NULL, &tm);
            */

            if (ret_rec < 2){
                int errvalue = errno;
                std::string nn;
                for(int i=0; i<strlen(msgQueueName); i++) nn += msgQueueName[i]; 
                std::cout << "The error generated was " << std::to_string(errvalue) << " in readMsgQueue - " << nn << std::endl;
                std::cout << "That means: " << strerror( errvalue ) << std::endl;
            }
            else{
                queueSize = queue[0] | (queue[1] << 8);
                //queue += 2;  // Skip queue size in message queue
            }

            return queueSize;
        };

        template<typename T>
        bool writeMsgQueue(const char *msgQueueName, std::vector<T> &queue, uint64_t queueSize){
            CurrentRecursionDepth++;
            if (CurrentRecursionDepth > MAX_RECURSION_LIMIT) {std::cout << "MAX_RECURSION_LIMIT is reached" << std::endl; return false;}

            mqd_t mqd_t_ = mq_open(msgQueueName, O_WRONLY|O_NONBLOCK);
            // TODO send message to the queue according to task priority              
            if (queueSize > MAX_MQ_MSG_SIZE-2) throw "queueSize exceeds MAX_MQ_MSG_SIZE. Try to increase MAX_MQ_MSG_SIZE inside configuration file";

            char *buf = new char[MAX_MQ_MSG_SIZE];
            // First two bytes defines the size of the queue
            buf[0] = queueSize & 0x00FF;
            buf[1] = (queueSize & 0xFF00) >> 8;

            for (size_t i=0; i< queueSize; i++){
                buf[i+2] = queue[i];
            }

            int ret_rec = mq_send(mqd_t_, buf, MAX_MQ_MSG_SIZE, 0);
            delete[] buf;
            mq_close(mqd_t_);
            
            if (ret_rec==-1){
                int errvalue = errno;
                if (errvalue==EAGAIN){  // Resource temporarily unavailable (Occurs when queue reaches max size)
                    // Erase a message by reading from queue
                    char *temp = new char[MAX_MQ_MSG_SIZE];
                    mqd_t mqd_temp = mq_open(msgQueueName, O_RDONLY);
                    mq_receive(mqd_temp, temp, MAX_MQ_MSG_SIZE, NULL);
                    delete[] temp;
                    mq_close(mqd_temp);
                    // Try to write again
                    writeMsgQueue(msgQueueName, queue, queueSize);
                }
                else {
                    std::string nn;
                    for(int i=0; i<strlen(msgQueueName); i++) nn += msgQueueName[i]; 
                    std::cout << "The error generated was " << std::to_string(errvalue) << " in writeMsgQueue - " << nn << std::endl;
                    std::cout << "That means: " << strerror( errvalue ) << std::endl;
                }
            }
            
            if (CurrentRecursionDepth>0) CurrentRecursionDepth--;
            return (bool) (ret_rec==0);
        };
        
        int closeMsgQueue(){
            mq_close(mq_port_in);
            mq_close(mq_port_out);
            return 0;          
        };

        int unlinkMsgQueue(const char *msgQueueName){
            size_t lenQueueName = strlen(msgQueueName);
            char *msgQueueNameIn = new char[lenQueueName+3];
            char *msgQueueNameOut = new char[lenQueueName+4];
            std::strcpy(msgQueueNameIn, msgQueueName);
            std::strcat(msgQueueNameIn, "_in");
            std::strcpy(msgQueueNameOut, msgQueueName);
            std::strcat(msgQueueNameOut, "_out");
            mq_unlink(msgQueueNameIn);
            mq_unlink(msgQueueNameOut);
            delete[] msgQueueNameIn;
            delete[] msgQueueNameOut;
            return 0;
        };

        size_t getAvailableMsgQueue(const char *msgQueueName){
            mq_attr mq_attr_;
            mqd_t mqd_port = mq_open(msgQueueName, O_RDONLY);
            if (mq_getattr(mqd_port, &mq_attr_) == -1)
                return -1;
            else
                return mq_attr_.mq_curmsgs;
        }
    
    public:
        /**
         * @brief Constructor of abstract class
        */
        AbstractTask(TaskConfig *config=NULL);

        // Destructor is not allowed for abstract class
        virtual ~AbstractTask() = 0;

        // Copy constructors are not allowed
        AbstractTask(const AbstractTask&) = delete;
        AbstractTask& operator=(const AbstractTask&) = delete;
        
        // Move constructors are not allowed because of the atomic_bool
        AbstractTask(AbstractTask&&) noexcept = delete;
        AbstractTask& operator=(AbstractTask&&) noexcept = delete;

        /**
         * @brief This function returns task ID.
         * @return taskID
        */
        size_t taskID();

        /**
         * @brief This function initialize thread attributes and starts a new thread.
        */
        void start();
        
        /**
         * @brief This function joins the thread.
         * @return Exit Status
        */
        virtual int join() noexcept;

        /**
         * @brief This function checks stop is requested.
         * @return Returns true stop is requested.  
        */
        inline bool isStopRequested() const noexcept;

        /**
         * @brief This function sets Task Status.
         * @param[in] taskStatus Task status to be set
        */
        void setTaskStatus(TASK_STATUS taskStatus) noexcept;

        /**
         * @brief This function returns status of task.
         * @return Status of task
        */
        TASK_STATUS TaskStatus() noexcept;

        /**
         * @brief This function terminates the ongoing task.
        */
        virtual void terminateTask() noexcept;
        
        /**
         * @brief This pure virtual function executes task functions.
        */
        virtual void runTask() noexcept = 0;

        /**
         * @brief This pure virtual function runs before the actual task is performed.
        */
        virtual void beforeTask() noexcept = 0;

        /**
         * @brief This pure virtual function is where actual task is performed.
        */
        virtual void myTask() noexcept = 0;

        /**
         * @brief This pure virtual function runs after the actual task is performed.
        */
        virtual void afterTask() noexcept = 0;

};

/**
 * @class Task
 * 
 * @brief This class provides basic POSIX thread functionalities.
 * 
 * This class inherits from AbstactTask class. This class performs better than 
 * CyclicTask in terms of speed. However, resolution of time interval between tasks is lower.
*/
class Task : public AbstractTask{
    private:
        virtual void runTask() noexcept final;

    public:
        Task(TaskConfig *config=NULL);

        ~Task();

        virtual void beforeTask() noexcept;
        
        virtual void myTask() noexcept;

        virtual void afterTask() noexcept;
};

/**
 * @class CyclicTask
 * 
 * @brief This class provides POSIX thread functionalities with tracker mechanism.
 * 
 * This class inherits from AbstactTask class. This class performs more determisnistic than 
 * Task class thanks to its own time tracking mechanism. However, it introduces extra overheads 
 * which may perform worse than Task. Overall, overheads can be neglegible and they will not 
 * differ much in most of the applications.
*/
class CyclicTask : public AbstractTask{
    private:
        bool enableWakeupLatencyTracer = false;
        bool enableLoopTracer = true;
        bool enableTraceOverrun = true;

        LatencyTracker mWakeupLatencyTracker;
        LatencyTracker mLoopLatencyTracker;
        LatencyTracker mBusyWaitLatencyTracker;

        virtual void runTask() noexcept final;

    public:
        CyclicTask(TaskConfig *config=NULL);
     
        ~CyclicTask();

        virtual void beforeTask() noexcept;
        
        virtual void myTask() noexcept;

        virtual void afterTask() noexcept;

};

}


#endif  // TASK_H