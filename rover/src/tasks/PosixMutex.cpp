#include "tasks/PosixMutex.hpp"


PMutex::PMutex(){
    pthread_mutexattr_t attr;
    int res;

    res = pthread_mutexattr_init(&attr);
    if (res != 0){
        throw std::runtime_error{std::string("cannot pthread_mutexattr_init :") + std::strerror(res)};
    }

    res = pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
    if (res != 0){
        throw std::runtime_error{std::string("cannot pthread_mutexattr_setprotocol :") + std::strerror(res)};
    
    }

    res = pthread_mutex_init(&m_, &attr);
    if (res != 0){
        throw std::runtime_error{std::string("cannot pthread_mutex_init :") + std::strerror(res)};
    }
}

PMutex::~PMutex(){}

void PMutex::lock(){
    auto res = pthread_mutex_lock(&m_);
    if (res != 0){
        throw std::runtime_error(std::string("failed pthread_mutex_lock: ") + std::strerror(res));
    }
};

void PMutex::unlock() noexcept{
    pthread_mutex_unlock(&m_);
};


bool PMutex::try_lock() noexcept{
    return pthread_mutex_trylock(&m_) == 0;
};

pthread_mutex_t* PMutex::nativeHandle() noexcept{
    return &m_;
};
