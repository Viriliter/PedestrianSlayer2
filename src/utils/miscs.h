#ifndef MISCS_H
#define MISCS_H

#include <iostream>
#include <string>
#include <chrono>

namespace miscs{
    
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
    
}

#endif  // MISCS_H