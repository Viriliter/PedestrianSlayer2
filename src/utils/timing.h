#ifndef TIMING_H
#define TIMING_H

#include <iostream>
#include <string>
#include <chrono>
#include <thread>

namespace timing{
    
    typedef std::chrono::steady_clock::time_point Time;
    typedef std::chrono::seconds Seconds;
    typedef std::chrono::milliseconds Miliseconds;
    typedef std::chrono::microseconds Microseconds;
    typedef std::chrono::nanoseconds Nanoseconds;

    enum timeFormat{
        formatSeconds,
        formatMiliseconds,
        formatMicroseconds,
        formatNanoseconds
    };

    inline unsigned long getEpoch(){
        return std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
    };

    inline Time pickTime(){
        return std::chrono::steady_clock::now();
    };
    
    inline void showElapsedTime(Time begin, Time end, int format){
        //Calculates differences between two time according to provided format (miscs::Seconds, miscs::Miliseconds miscs::Nanoseconds)
        
        switch(format){
            case(timeFormat::formatSeconds): 
                std::cout << std::chrono::duration_cast<Seconds>(end - begin).count() << " s" << std::endl;
                break;
            case(timeFormat::formatMiliseconds):
                std::cout << std::chrono::duration_cast<Miliseconds>(end - begin).count() << " ms" << std::endl;
                break;
            case(timeFormat::formatMicroseconds):
                std::cout << std::chrono::duration_cast<Microseconds>(end - begin).count() << " us" << std::endl;
                break;
            case(timeFormat::formatNanoseconds):
                std::cout << std::chrono::duration_cast<Nanoseconds>(end - begin).count() << " ns" << std::endl;
                break;
       }
    };
    
    inline void sleep(long long duration, int format){
        // Sleeps current thread in predefined duration with appropriate time format
        switch(format){
        case(timeFormat::formatSeconds): 
            std::this_thread::sleep_for(std::chrono::seconds(duration));
            break;
        case(timeFormat::formatMiliseconds):
            std::this_thread::sleep_for(std::chrono::milliseconds(duration));
            break;
        case(timeFormat::formatMicroseconds):
            std::this_thread::sleep_for(std::chrono::microseconds(duration));
            break;
        case(timeFormat::formatNanoseconds):
            std::this_thread::sleep_for(std::chrono::nanoseconds(duration));
            break;
        }
    };
}
#endif  // TIMING_H