#ifndef MISCS_H
#define MISCS_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>

namespace miscs{

    template<typename T>
    std::string Dec2HexString(std::vector<T> &input){
        std::stringstream ss;
        for(auto &dec:input)
        {
            ss << "0x" << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << (int) dec << " ";
        }
        return ss.str();
    }

}

#endif //MISCS_H