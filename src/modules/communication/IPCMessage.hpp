#ifndef IPCMESSAGE_HPP
#define IPCMESSAGE_HPP 

#include <string>
#include <iostream>
#include <memory>
#include <vector>
#include <sstream>

#include <map>
#include <any>
#include <variant>
//#include "simdjson.h"
#include "../../third-party/nlohmann/json.hpp"
#include "../../utils/container.h"
#include "../../utils/timing.h"
#include "../../utils/types.h"

namespace comm::ipc{

    typedef std::string PackageKeyType; // <data type, data value> 
    typedef std::variant<UINT8, UINT16, UINT32, UINT64, 
                         INT8, INT16, INT32, INT64,
                         FLOAT, DOUBLE, LONG, LONG_LONG,
                         std::vector<UINT8>> PackageValueType;

    typedef std::map<PackageKeyType, PackageValueType> PayloadType; // <data key, data pair>

    //std::map<std::string, std::vector<uint8_t>> posixDataType;

    struct messageStruct{
        unsigned long epoch;
        std::string keyword;
        PayloadType payload;
    };

    class IPCMessage{
    private:
        nlohmann::json obj_;
    public:
        unsigned long epoch;
        std::string keyword;
        PayloadType payload;

        
        IPCMessage(nlohmann::json &obj): obj_(obj){
            //epoch = obj_["epoch"];
            //keyword = obj_["keyword"];
        };

        IPCMessage(std::string keyword, PayloadType &payload): keyword(keyword), payload(payload){
            epoch = timing::getEpoch();
        };

        std::vector<uint8_t> serialize(){
            obj_["_epoch"] = epoch;
            obj_["_keyword"] = keyword;
        
            for(auto const &package:payload){
                std::string key = (std::string) package.first;
                // Check whether first character is underscore. (Underscore keys are omitted by default)
                if (key[0] == '_') continue;
                
                if (const auto ptrValue(std::get_if<UINT8>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                else if (const auto ptrValue(std::get_if<UINT16>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                else if (const auto ptrValue(std::get_if<UINT32>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                else if (const auto ptrValue(std::get_if<UINT64>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                else if (const auto ptrValue(std::get_if<INT8>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                else if (const auto ptrValue(std::get_if<INT16>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                else if (const auto ptrValue(std::get_if<INT32>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                //else if (const auto ptrValue(std::get_if<INT64>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                else if (const auto ptrValue(std::get_if<FLOAT>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                else if (const auto ptrValue(std::get_if<DOUBLE>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                //else if (const auto ptrValue(std::get_if<LONG>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                // Compilation error on GCC 9.5 so following line is commented out
                //else if (const auto ptrValue(std::get_if<LONG_LONG>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                else if (const auto ptrValue(std::get_if<std::vector<UINT8>>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                
                // obj_[key] = package.second;
                /*
                PackageType value = std::make_pair("-", package.second);
                obj_[key] = value;
                */
            }
            std::vector<uint8_t> v = nlohmann::json::to_msgpack(obj_);

            return v;
            //return nlohmann::to_string(obj_);
        };
        
        char *serialize2char(){
            obj_["_epoch"] = epoch;
            obj_["_keyword"] = keyword;
        
            for(auto const &package:payload){
                std::string key = (std::string) package.first;
                // Check whether first character is underscore. (Underscore keys are omitted by default)
                if (key[0] == '_') continue;
                
                if (const auto ptrValue(std::get_if<UINT8>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                else if (const auto ptrValue(std::get_if<UINT16>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                else if (const auto ptrValue(std::get_if<UINT32>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                else if (const auto ptrValue(std::get_if<UINT64>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                else if (const auto ptrValue(std::get_if<INT8>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                else if (const auto ptrValue(std::get_if<INT16>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                else if (const auto ptrValue(std::get_if<INT32>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                //else if (const auto ptrValue(std::get_if<INT64>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                else if (const auto ptrValue(std::get_if<FLOAT>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                else if (const auto ptrValue(std::get_if<DOUBLE>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                //else if (const auto ptrValue(std::get_if<LONG>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                // Compilation error on GCC 9.5 so following line is commented out
                //else if (const auto ptrValue(std::get_if<LONG_LONG>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                else if (const auto ptrValue(std::get_if<std::vector<UINT8>>(&package.second)); ptrValue) obj_[key] = *ptrValue;
                
                // obj_[key] = package.second;
                /*
                PackageType value = std::make_pair("-", package.second);
                obj_[key] = value;
                */
            }
            std::vector<uint8_t> serialized_vector = nlohmann::json::to_msgpack(obj_);
            char *buf = new char[serialized_vector.size()];
            size_t i = 0;
            for (auto &b:serialized_vector){
                buf[i] = b;
                i++;
            }
            return buf;
            //return nlohmann::to_string(obj_);
        };
        
        template<typename T>
        T getPackageValue(const std::string &packageName){
            for(auto &item:obj_.items()){
                if (item.key()[0] == '_') continue;

                if (packageName == item.key()){
                    //obj_.get<decltype(T)>();
                    return static_cast<T>(obj_[packageName]);
                }
            }
            throw "No Package named '" + packageName + "' exists in " +  keyword + " message";
        }
    };

    template<typename T>
    void insert_package(PayloadType &payload, std::string packageKeyName, T &packageValue){
        payload.insert( std::pair<PackageKeyType, PackageValueType>(packageKeyName, static_cast<T>(packageValue)));
    };

    inline IPCMessage deserialize(const std::vector<UINT8> &rawMessage){
        nlohmann::json obj_ = nlohmann::json::from_msgpack(rawMessage);

        IPCMessage message(obj_);
        for (auto &item: obj_.items()){
            if (item.key() == "_epoch") message.epoch = static_cast<unsigned long>(item.value());
            else if (item.key() == "_keyword") message.keyword = item.value();
            else continue;
        }
        /*
        for (auto &item: obj_.items()){
            if (item.key() == "_epoch") message.epoch = static_cast<unsigned long>(item.value());
            else if (item.key() == "_keyword") message.keyword = item.value();
            else {
                message.payload.insert(std::pair<std::string, PackageType>(item.key(), get_to(item.value())));

                    
                //nlohmann::json packages = nlohmann::json::from_msgpack(item.value());
                //for(auto const &package:packages.items()){
                //    PackageType my_package = std::make_pair(package.key(), package.value());
                //    message.payload.insert(std::pair<std::string, PackageType>(item.key(), my_package));
                //    //message.data[el.key()] = el.value();
                //    //message.data.insert(std::pair<std::string, VariantTypes>(el.key(), el.value()));
                //}
            }
        }
        */
        return message;
    };

    inline IPCMessage deserialize(char *buf, int bufSize){
        nlohmann::json obj_;

        if (bufSize<=0) {
            IPCMessage message(obj_);
            return message;
        }
        
        obj_ = nlohmann::json::from_msgpack(buf, buf+bufSize);

        IPCMessage message(obj_);
        for (auto &item: obj_.items()){
            if (item.key() == "_epoch") message.epoch = static_cast<unsigned long>(item.value());
            else if (item.key() == "_keyword") message.keyword = item.value();
            else continue;
        }
        return message;
    };

}

#endif  // IPCMESSAGE_HPP