#ifndef TASK_HPP
#define TASK_HPP

#include <pthread.h>
#include <sys/mman.h>  // necessary for mlockall
#include <mqueue.h>
#include <bits/local_lim.h>  // It is neccessary to be seen PTHREAD_STACK_MIN macro by some compilers 

#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <iostream>
#include <vector>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <cerrno>
#include <ctime>

#include "schedulers.hpp"
#include "utils/threading/latency_tracker.h"
#include "utils/timing.h"
#include "config.hpp"
#include "utils/types.h"

const unsigned int MAX_RECURSION_LIMIT = 100;

namespace tasks{
    enum SCHEDULE_POLICY{
        DEADLINE,
        FIFO,
        OTHER,
    };

    enum TASK_PRIORITY{
        RT_VERY_HIGH_PRIORITY = 88,
        RT_HIGH_PRIORITY = 80,
        RT_NORMAL_PRIORITY = 60,
        RT_LOW_PRIORITY = 10,
        HIGH_PRIORITY = 0,
        NORMAL_PRIORITY = 10,
        LOW_PRIORITY = 20,
    };

    enum TASK_STATUS{
        IDLE_TASK,  // The task has not been run before 
        RUNNING_TASK,  // The task is running
        STOPPED_TASK,  // The task has been run at least once before but it is not running now.
    };

    /*
    struct mq_attr{
        long mq_flags;  // Flags: 0 or O_NONBLOCK
        long mq_maxmsg;  // Max # of messages on queue
        long mq_msgsize;  // Max message size (bytes)
        long mq_curmsgs;  // # of messages currently in queue
    };
    */

    constexpr size_t kDefaultStackSize = 8 * 1024 * 1024;  // 8MB
    static size_t task_id_;

    // Base Task is a template for different type of tasks (Task, CyclicTask e.g.)
    class BaseTask {
        //std::atomic_bool stop_requested_ = false;
        std::atomic<bool> stop_requested_ = {false};
        size_t taskID_ = 0;
        std::string task_name_ = "";
        SCHEDULE_POLICY policy_;
        TASK_STATUS taskStatus_ = TASK_STATUS::IDLE_TASK;  // Define task status as idle at first
        static unsigned int CurrentRecursionDepth;

    protected:
        std::vector<size_t> cpu_affinity_;
        size_t stack_size_ = (static_cast<size_t>(PTHREAD_STACK_MIN) + kDefaultStackSize);
        TASK_PRIORITY task_priority_;
        int64_t period_ns_ = 0;
        int64_t runtime_ns_ = 0;
        int64_t deadline_ns_ = 0;
        std::vector<size_t> cpu_affinity = {};

        void taskStatus(TASK_STATUS taskStatus) {taskStatus_ = taskStatus;}; // Setter

    public:
        TASK_STATUS taskStatus() const {return taskStatus_;};
        size_t taskID() {return taskID_;};
        mqd_t mq_port_in;
        mqd_t mq_port_out;
        virtual void start(int64_t start_monotonic_time_ns, int64_t start_wall_time_ns) = 0;
        virtual int join() = 0;

        virtual void RequestStop() noexcept {
            stop_requested_ = true;
        }

        // The constructors and destructors are needed because we need to delete
        // objects of type BaseTask polymorphically.
        BaseTask(std::string &task_name, SCHEDULE_POLICY &policy, TASK_PRIORITY &task_priority, int64_t period_ns=1000000000, int64_t runtime_ns=0, int64_t deadline_ns=0, std::vector<size_t> cpu_affinity = {})
        : task_name_(task_name), policy_(policy), task_priority_(task_priority), period_ns_(period_ns), runtime_ns_(runtime_ns), deadline_ns_(deadline_ns), cpu_affinity_(cpu_affinity){
            //taskID_ = task_id_;
            //task_id_++;

            sched_attr attr = {};
            attr.size = sizeof(attr);
            attr.sched_flags = 0;
            switch(policy){
                case(SCHEDULE_POLICY::DEADLINE):
                    attr.sched_nice = 0;
                    attr.sched_priority = task_priority;

                    attr.sched_policy = SCHED_DEADLINE;  // Set the scheduler policy
                    attr.sched_runtime = runtime_ns;
                    attr.sched_deadline = deadline_ns;
                    attr.sched_period = period_ns;

                    break;
                case(SCHEDULE_POLICY::FIFO):
                    attr.sched_nice = 0;
                    attr.sched_priority = task_priority;  // Set the scheduler priority
                    
                    attr.sched_policy = SCHED_FIFO;         // Set the scheduler policy
                    break;
                case(SCHEDULE_POLICY::OTHER): 
                    // Self scheduling attributes
                    attr.sched_nice = task_priority;    // Set the thread niceness
                    
                    attr.sched_policy = SCHED_OTHER;  // Set the scheduler policy
                    break;
                default:
                    break;
            };

            auto ret = sched_setattr(0, &attr, 0);
            if (ret < 0) {
                SPDLOG_ERROR("unable to sched_setattr: {}", std::strerror(errno));
                throw std::runtime_error{"failed to sched_setattr"};
            }          
        };
        virtual ~BaseTask() = default;

        // Copy constructors are not allowed
        BaseTask(const BaseTask&) = delete;
        BaseTask& operator=(const BaseTask&) = delete;

        // Should the thread be moveable? std::thread is moveable
        // TODO: investigate moving the stop_requested_ flag somewhere else
        // Move constructors are not allowed because of the atomic_bool
        BaseTask(BaseTask&&) noexcept = delete;
        BaseTask& operator=(BaseTask&&) noexcept = delete;

        /**
         * @brief Check if stop has been requested
         *
         * @return true if stop is requested
         */
        bool StopRequested() const noexcept {
            // Memory order relaxed is OK, because we don't really care when the signal
            // arrives, we just care that it is arrived at some point.
            //
            // Also this could be used in a tight loop so we don't want to waste time when we don't need to.
            //
            // https://stackoverflow.com/questions/70581645/why-set-the-stop-flag-using-memory-order-seq-cst-if-you-check-it-with-memory
            // TODO: possibly use std::stop_source and std::stop_token (C++20)
            return stop_requested_.load(std::memory_order_relaxed);
        };

        inline double Sleep(const struct timespec& next_wakeup_time) noexcept {
            // TODO: check for errors?
            // TODO: can there be remainders?
            switch(policy_){
                case(SCHEDULE_POLICY::DEADLINE):
                    // Ignoring error as man page says "In the Linux implementation, sched_yield() always succeeds."
                    sched_yield();
                    return 0.0;               
                default:
                    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_wakeup_time, nullptr);
                    return 0.0;
            };
        };
    
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
    };

    /*
    * Simple real-time compilent task class. Task runs in defined period with proper task settings.
    * However, its timing is not as accurate as CyclicTask. 
    * @param cpu_affinity - mask for cpu cores so that task runs in specified cores
    * @param period_ns - period of task in nanoseconds
    */
    class Task : public BaseTask {
    private:
        pthread_t task_;

        struct timespec next_wakeup_time_;
    
        /**
         * A wrapper function that is passed to pthread. This starts the task and
         * performs any necessary setup.
         */
        static void* RunTask(void* data) {
            Task* task = static_cast<Task*>(data);
            //SetThreadScheduling(task->scheduler_config_);  // TODO: return error instead of throwing
            task->run();
            return nullptr;
        };

    public:
        Task(std::string &task_name, SCHEDULE_POLICY &policy, TASK_PRIORITY &task_priority, int64_t period_ns=1000000000, int64_t runtime_ns=0, int64_t deadline_ns=0, std::vector<size_t> cpu_affinity = {})
        : BaseTask(task_name, policy, task_priority, period_ns, runtime_ns, deadline_ns, cpu_affinity){
        };
        ~Task(){
            std::cout << "task deconstructor called" << std::endl;
        };
        
        /**
         * Starts the task in the background.
         *
         * @param start_monotonic_time_ns should be the start time in nanoseconds for the monotonic clock.
         * @param start_wall_time_ns should be the start time in nanoseconds for the realtime clock.
         */
        void start(int64_t start_monotonic_time_ns, int64_t start_wall_time_ns) override {
            pthread_attr_t attr;

            // Initialize the pthread attribute
            int ret = pthread_attr_init(&attr);
            if (ret != 0) {
            throw std::runtime_error(std::string("error in pthread_attr_init: ") + std::strerror(ret));
            }

            // Set a stack size
            //
            // Note the stack is prefaulted if mlockall(MCL_FUTURE | MCL_CURRENT) is
            // called, which under this app framework should be.
            //
            // Not even sure if there is an optimizer-safe way to prefault the stack
            // anyway, as the compiler optimizer may realize that buffer is never used
            // and thus will simply optimize it out.
            ret = pthread_attr_setstacksize(&attr, stack_size_);
            if (ret != 0) {
            throw std::runtime_error(std::string("error in pthread_attr_setstacksize: ") + std::strerror(ret));
            }

            // Setting CPU mask
            if (!cpu_affinity_.empty()) {
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            for (auto cpu : cpu_affinity_) {
                CPU_SET(cpu, &cpuset);
            }

            ret = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
            if (ret != 0) {
                throw std::runtime_error(std::string("error in pthread_attr_setaffinity_np: ") + std::strerror(ret));
            }
            }

            ret = pthread_create(&task_, &attr, &Task::RunTask, this);
            if (ret != 0) {
            throw std::runtime_error(std::string("error in pthread_create: ") + std::strerror(ret));
            }
        };

        /**
         * Joins the thread.
         *
         * @returns the return value of pthread_join
         */
        int join() override {
            return pthread_join(task_, nullptr);
        };

    protected:
        void terminateTask(){
            this->RequestStop();
        }
       
        virtual void beforeTask(){};

        virtual void afterTask(){};

        virtual void runTask(){};

        virtual void run() noexcept final{
            taskStatus(TASK_STATUS::RUNNING_TASK);

            clock_gettime(CLOCK_MONOTONIC, &next_wakeup_time_);

            beforeTask();
            
            auto last_data_write_time = timing::NowNs();

            while(true){
                if (this->StopRequested()) {
                    break;
                }

                auto now = timing::NowNs();

                this->runTask();
           
                next_wakeup_time_ = timing::AddTimespecByNs(next_wakeup_time_, period_ns_);
                clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_wakeup_time_, nullptr);
                
                std::this_thread::sleep_for(std::chrono::nanoseconds(period_ns_));
            }

            afterTask();
            SPDLOG_INFO("Aborted");

           taskStatus(TASK_STATUS::STOPPED_TASK);
        };

    };

    /*
    * Real-time compilent task class. CyclicTask runs in defined period with higher resolution by estimating the elapsed time to call the task.
    * @param cpu_affinity - mask for cpu cores so that task runs in specified cores
    * @param period_ns - period of task in nanoseconds
    */
    class CyclicTask: public BaseTask{
    private:
        schedulers::Config scheduler_config_;
        pthread_t task_;
        bool rt_enabled = false;

        int64_t start_monotonic_time_ns_ = 0;
        int64_t start_wall_time_ns_ = 0;
        struct timespec next_wakeup_time_;

        // Debug information
        threading::LatencyTracker wakeup_latency_tracker_;
        threading::LatencyTracker loop_latency_tracker_;
        threading::LatencyTracker busy_wait_latency_tracker_;

        static void* RunTask(void* data) {
            CyclicTask* task = static_cast<CyclicTask*>(data);
            //SchedulerT::SetThreadScheduling(task->scheduler_config_);  // TODO: return error instead of throwing
            task->run();
            return nullptr;
        }

        virtual void traceLoopStart(double /* wakeup_latency_us */) noexcept {};

        virtual void traceLoopEnd(double /* loop_latency_us */) noexcept {};

    public:
        CyclicTask(std::string &task_name, SCHEDULE_POLICY &policy, TASK_PRIORITY &task_priority, int64_t period_ns=1000000000, int64_t runtime_ns=0, int64_t deadline_ns=0, std::vector<size_t> cpu_affinity = {})
        : BaseTask(task_name, policy, task_priority, period_ns, runtime_ns, deadline_ns, cpu_affinity){
        };

        int64_t StartMonotonicTimeNs() const { return start_monotonic_time_ns_; };
        int64_t StartWallTimeNs() const { return start_wall_time_ns_; };
        
        /**
         * Track the latency wakeup and loop latency. The default behavior is to track them in histograms that updates online.
         * @param wakeup_latency the latency of wakeup (scheduling latency) in us.
         * @param loop_latency the latency of Loop() call in us.
         */
        virtual void trackLatency(double /*wakeup_latency*/, double /*loop_latency*/) noexcept {};

        /**
         * Starts the task in the background.
         *
         * @param start_monotonic_time_ns should be the start time in nanoseconds for the monotonic clock.
         * @param start_wall_time_ns should be the start time in nanoseconds for the realtime clock.
         */
        void start(int64_t start_monotonic_time_ns, int64_t start_wall_time_ns) override {
            pthread_attr_t attr;

            // Initialize the pthread attribute
            int ret = pthread_attr_init(&attr);
            if (ret != 0) {
            throw std::runtime_error(std::string("error in pthread_attr_init: ") + std::strerror(ret));
            }

            // Set a stack size
            //
            // Note the stack is prefaulted if mlockall(MCL_FUTURE | MCL_CURRENT) is
            // called, which under this app framework should be.
            //
            // Not even sure if there is an optimizer-safe way to prefault the stack
            // anyway, as the compiler optimizer may realize that buffer is never used
            // and thus will simply optimize it out.
            ret = pthread_attr_setstacksize(&attr, stack_size_);
            if (ret != 0) {
            throw std::runtime_error(std::string("error in pthread_attr_setstacksize: ") + std::strerror(ret));
            }

            // Setting CPU mask
            if (!cpu_affinity_.empty()) {
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            for (auto cpu : cpu_affinity_) {
                CPU_SET(cpu, &cpuset);
            }

            ret = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
            if (ret != 0) {
                throw std::runtime_error(std::string("error in pthread_attr_setaffinity_np: ") + std::strerror(ret));
            }
            }

            ret = pthread_create(&task_, &attr, &CyclicTask::RunTask, this);
            if (ret != 0) {
            throw std::runtime_error(std::string("error in pthread_create: ") + std::strerror(ret));
            }
        };

        /**
         * Joins the thread.
         *
         * @returns the return value of pthread_join
         */
        int join() override {
            return pthread_join(task_, nullptr);
        };

    protected:
        void terminateTask(){
            this->RequestStop();
        };

        virtual void beforeTask(){};

        virtual void afterTask(){
            SPDLOG_DEBUG("wakeup_latency:");
            wakeup_latency_tracker_.DumpToLogger();

            SPDLOG_DEBUG("loop_latency:");
            loop_latency_tracker_.DumpToLogger();

            SPDLOG_DEBUG("busy_wait_latency:");
            busy_wait_latency_tracker_.DumpToLogger();
        };

        virtual bool runTask(int64_t ellapsed_ns) noexcept { return true;};

        virtual void run() noexcept final {
            taskStatus(TASK_STATUS::RUNNING_TASK);
            clock_gettime(CLOCK_MONOTONIC, &next_wakeup_time_);
            int64_t loop_start, loop_end, should_have_woken_up_at;

            int64_t wakeup_latency, loop_latency, busy_wait_latency;

            while (!this->StopRequested()) {
                should_have_woken_up_at = next_wakeup_time_.tv_sec * 1000000000 + next_wakeup_time_.tv_nsec;
                loop_start = timing::NowNs();

                wakeup_latency = loop_start - should_have_woken_up_at;

                traceLoopStart(wakeup_latency);

                if (runTask(loop_start - StartMonotonicTimeNs())) {
                    break;
                }

                loop_end = timing::NowNs();
                loop_latency = static_cast<double>(loop_end - loop_start);

                traceLoopEnd(loop_latency);

                trackLatency(wakeup_latency, loop_latency);

                wakeup_latency_tracker_.RecordValue(wakeup_latency);
                loop_latency_tracker_.RecordValue(loop_latency);

                next_wakeup_time_ = timing::AddTimespecByNs(next_wakeup_time_, period_ns_);
                busy_wait_latency = Sleep(next_wakeup_time_);

                busy_wait_latency_tracker_.RecordValue(busy_wait_latency);
            }
            taskStatus(TASK_STATUS::STOPPED_TASK);
        };

    };

}


#endif  //TASK_HPP