#ifndef POSIX_MUTEX_H
#define POSIX_MUTEX_H

#include <pthread.h>
#include <cstring>
#include <stdexcept>

class PMutex{
    private:
        pthread_mutex_t m_;

    public:
        PMutex();

        ~PMutex();

        PMutex(const PMutex&) = delete;

        PMutex& operator=(const PMutex&) = delete;

        void lock();

        void unlock() noexcept;
        
        bool try_lock() noexcept;

        pthread_mutex_t* nativeHandle() noexcept;
};

#endif  // POSIX_MUTEX_H