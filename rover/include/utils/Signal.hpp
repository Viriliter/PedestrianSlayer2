#ifndef SIGNAL_H
#define SIGNAL_H

#include <vector>
#include <utility>
#include <functional>

#include "tasks/PosixMutex.hpp"


//#define SLOT(slotName, slotFunc,...) template<typename ...Args> class slotName##Slot__: public ISlot<__VA_ARGS__>{ public: void operator()(Args... args) override{slotFunc(args...);}}; \
//                                     public: slotName##Slot__<__VA_ARGS__> slotName;

enum class CONNECTION_MODE{
    DIRECT_CONNECTION,  ///> Blocking Connection (uses Posix Mutex)
    QUEUED_CONNECTION   ///> Non-Blocking Connection
};

/*
template<typename... Args>
class ISlot{
    private:
        //std::function<void(Args..)> mCallback;
    public:
        virtual ~ISlot() = default;
        virtual void operator()(Args...) = 0;
};
*/

/**
 * @brief Generic signal class that can be used to connect and emit signals.
 * 
 * @tparam Args Variadic template for signal arguments.
 */
template<typename... Args>
class Signal{
    private:
        PMutex mMutex;

    public:        
        std::vector<std::pair<std::function<void(Args...)>, CONNECTION_MODE>> mSlots;
        /**
         * @brief Destructor for the Signal class.
         * 
         * Clears all connected slots.
         */
        ~Signal(){
            mSlots.clear();
        }

        /**
         * @brief This function emits the signal to all connected slots.
         * 
         * @param args Variadic arguments to be passed to the slots.
         */
        void emit(Args... args){
            for (const auto &slot:mSlots){
                if (slot.second == CONNECTION_MODE::QUEUED_CONNECTION){
                    slot.first(args...);
                }
                else{  // DIRECT_CONNECTION
                    mMutex.lock();
                    slot.first(args...);
                    mMutex.unlock();
                }
            }
        }

        /**
         * @brief This function returns slot object.
         * 
         * @return Slot object.
         */
        std::vector<std::pair<std::function<void(Args...)>, CONNECTION_MODE>> slots() const noexcept{
            return mSlots;
        }
};

/**
 * @brief This function connects Signal to the slot function.
 * 
 * @tparam Class Type of the class containing the slot function.
 * @tparam Args Variadic template for slot arguments.
 * @param signal Reference to the Signal object.
 * @param instance Pointer to an instance of the slot function.
 * @param slot Pointer of slot function.
 * @param mode Connection mode (DIRECT_CONNECTION or QUEUED_CONNECTION).
 */
/*
template<typename ClassSlot, typename... Args>
static void connect(Signal<Args...> &signal,
                    ClassSlot *instanceSlot, 
                    void (ClassSlot::*slot)(Args...),
                    CONNECTION_MODE mode=CONNECTION_MODE::DIRECT_CONNECTION){

                    auto callback = [instanceSlot, slot] (Args... args) {(instanceSlot->*slot)(args...);};
                    signal.slots.push_back(std::make_pair(callback, mode));
}
*/

/**
 * @brief This function connects Signal to the slot function.
 * 
 * @tparam ClassSignal Type of the class where Signal object is emitted from.
 * @tparam ClassSignalObj Type of the class containing the Signal object 
 * ClassSignalObj may differ from ClassSignal, if Signal object is inherited from parent class.
 * @tparam ClassSlot Type of the class containing the slot function.
 * @tparam ClassSlotObj Type of the class which the slot function is declared first.
 * ClassSlotObj may differ from ClassSlot, if Slot function is inherited from parent class.
 * @tparam Args Variadic template for slot arguments.
 * @param instanceSignal Pointer to an instance of the Signal object.
 * @param signal Pointer to the Signal object.
 * @param instanceSlot Pointer to an instance of the slot function.
 * @param slot Pointer of slot function.
 * @param mode Connection mode (DIRECT_CONNECTION or QUEUED_CONNECTION).
 */
template<typename ClassSignal, typename ClassSignalObj, typename ClassSlot, typename ClassSlotObj, typename... Args>
static void connect(ClassSignal *instanceSignal, 
                    Signal<Args...> ClassSignalObj::*signal,
                    ClassSlot *instanceSlot, 
                    void (ClassSlotObj::*slot)(Args...),
                    CONNECTION_MODE mode=CONNECTION_MODE::DIRECT_CONNECTION){

                    auto callback = [instanceSlot, slot] (Args... args) {(instanceSlot->*slot)(args...);};
                    (instanceSignal->*signal).mSlots.push_back(std::make_pair(callback, mode));
}

/**
 * @brief This function disconnects signal from all slot functions.
 * 
 * @tparam Class Type of the class containing the slot function.
 * @tparam Args Variadic template for slot arguments.
 * @param signal Reference to the Signal object.
 * @param instance Pointer to an instance of the slot function.
 * @param slot Pointer of slot function.
 * @param mode Connection mode (DIRECT_CONNECTION or QUEUED_CONNECTION).
 */
template<typename... Args>
static void disconnect(Signal<Args...> &signal){
    signal.mSlots.clear();
};

#endif  // SIGNAL_H