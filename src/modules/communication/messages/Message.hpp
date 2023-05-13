#ifndef MESSAGE_HPP
#define MESSAGE_HPP 

#include <string>
#include <iostream>
#include <memory>
#include <sstream>

namespace communication::messages{
    template <typename T>
    class Node{
        public:
        Node *next = NULL;
        Node *prev = NULL;
        T val{};

        Node(){};
        Node(T val):val(val){};
    };

    template <typename T>
    class LinkedBytes{
        int count = 0;
        Node<T> *head = NULL;
        Node<T> *tail = NULL;
        
        public:
            LinkedBytes(){};

            ~LinkedBytes(){
                if (head==NULL) return;

                Node<T> *ptr = head;
                while(ptr != NULL){
                    Node<T> *temp = ptr;
                    ptr = ptr->prev;
                    delete temp;
                }
            };

            LinkedBytes(const LinkedBytes &srcLinkedBytes){
                // Copy constructor creates the list by taking deep copy of source list
                if (srcLinkedBytes.head == NULL){
                    clear();
                    head = NULL;
                    tail = NULL;
                    return;
                }

                tail = new Node<T>();
                head = new Node<T>();
                Node<T> *ptrSrc = srcLinkedBytes.tail;
                tail->val = ptrSrc->val;
                tail->next = NULL;

                Node<T> *current = tail;
                ptrSrc = ptrSrc->next;
                while(ptrSrc != NULL){
                    Node<T> *temp = current;
                    current->next = new Node<T>();
                    current = current->next;
                    current->val = ptrSrc->val;
                    current->prev = temp;
                    current->next = NULL;
                    ptrSrc = ptrSrc->next;
                }
                head = current;
                this->count = srcLinkedBytes.count;
            }

            LinkedBytes &operator+(LinkedBytes &newLinkedBytes){
                // Overloaded function creates extends current list by taking deep copy of new list
                if (newLinkedBytes.head == NULL) return *this;
                
                Node<T> *ptrSrc = newLinkedBytes.tail;
                Node<T> *current = head;
                ptrSrc = ptrSrc->next;
                while(ptrSrc != NULL){
                    Node<T> *temp = current;
                    current->next = new Node<T>();
                    current = current->next;
                    current->val = ptrSrc->val;
                    current->prev = temp;
                    current->next = NULL;
                    ptrSrc = ptrSrc->next;
                }
                head = current;
                this->count += newLinkedBytes.count;
                return *this;
            }
            
            LinkedBytes &operator+=(LinkedBytes &newLinkedBytes){
                // Overloaded function creates extends current list by taking deep copy of new list
                if (head == NULL){
                    *this = newLinkedBytes;
                    return *this;
                } 

                if (newLinkedBytes.head == NULL) return *this;
                
                Node<T> *ptrSrc = newLinkedBytes.tail;
                Node<T> *current = head;
                ptrSrc = ptrSrc->next;
                while(ptrSrc != NULL){
                    Node<T> *temp = current;
                    current->next = new Node<T>();
                    current = current->next;
                    current->val = ptrSrc->val;
                    current->prev = temp;
                    current->next = NULL;
                    ptrSrc = ptrSrc->next;
                }
                head = current;
                this->count += newLinkedBytes.count;
                return *this;
            }

            T operator[](int i) const {return get(i);}  // getter

            T &operator[](int i) {return get(i);}  // setter
            
            int Count(){
                return count;
            };

            bool isEmpty(){
                if (head==NULL) 
                    return true;
                else 
                    return false;
            };

            void add(T val){
                // Add new value after header node of the linked list
                Node<T> *node = new Node<T>();
                node->val = val;
                //node->prev = head;
                //node->next = NULL;
                if (head == NULL){  // No element exists
                    head = node;
                    tail = head;
                }
                else{  // Multiple elements exist 
                    head->next = node;
                    node->prev = head;
                    head = node;
                }
                count++;
            }

            void push(T val){
                // Push new value before tail node of the linked list
                Node<T> *newNode = new Node<uint8_t>();
                newNode->val = val;

                if (tail == NULL){
                    head = newNode; 
                    tail = newNode;
                    return; 
                }

                tail->prev = newNode;
                newNode->next = tail;
                tail = newNode;
            };

            T pop(){
                // Pops elements according to LIFO
                if (head == NULL) throw "Empty List Error!";
                
                Node<T> *ptr = head;
                Node<T> *temp = ptr;
                T value = ptr->val;
                ptr = ptr->prev;
                ptr->next = NULL;
                head = ptr;

                if (head == NULL){
                    tail == NULL;
                }
                else if(head != NULL & head->prev == NULL){ // 1 Element exists
                    tail->next = NULL;
                }

                delete temp;
                count--;
                return value;
            };

            T &get(int index){
                // Get elements according to index of first added element
                if (tail == NULL) throw "Empty List Error!";

                Node<T> *ptr = tail;
                while(ptr != NULL){
                    if (index==0){
                        return ptr->val;
                    }
                    ptr = ptr->next;
                    index--;
                }
                throw "Index Out of Bound Error!";
            };

            void set(int index, T val){
                // Set elements according to index of first added element
                if (tail == NULL) throw "Empty List Error!";

                Node<T> *ptr = tail;
                while(ptr != NULL){
                    if (index==0){
                        ptr->val = val;
                        return;
                    }
                    index--;
                }
                throw "Index Out of Bound Error!";
            };

            void clear(){
                if (head==NULL) return;

                Node<T> *ptr = head;
                while(ptr != NULL){
                    Node<T> *temp = ptr;
                    ptr = ptr->prev;
                    delete temp;
                }
                count = 0;
            }

            std::string toString(){
                std::string result = "";
                if (tail == NULL) return result;

                Node<T> *ptr = tail;
                while(ptr != NULL){
                    result += std::to_string(ptr->val) + " ";
                    ptr = ptr->next;
                }
                return result;
            };
    };

    class MessagePacket{
        public:
            uint8_t SOM = 0;
            uint8_t MsgSize = 0;
            uint8_t MsgID = 0;
            LinkedBytes<uint8_t> MessagePayload{};  // Store message payload in Linked List since its size may differ
            uint8_t Checksum = 0;

            std::string toString(){
                std::string result;

                result = "SOM: " + std::to_string(SOM) + " " +
                         "MsgSize: " + std::to_string(MsgSize) + " " +
                         "MsgID: " + std::to_string(MsgID) + " " +
                         "Payload: " + MessagePayload.toString() +
                         "Checksum: " + std::to_string(Checksum);
                return result;
            };

            uint8_t calcChecksum(){
                // Calculates checksum of the message by summing bytes. Note that in checksum calculation "Checksum" field is not included.
                int checksum = 0;
                checksum = SOM + MsgSize + MsgID;

                for (size_t i=0; i< MessagePayload.Count(); i++){
                    checksum += (int) MessagePayload[i];
                }
                return checksum % 255;
            }

            bool checkChecksum(){
                // Calculates the checksum and compares with the "Checksum" field.
                uint8_t calculatedChecksum = calcChecksum();
                return Checksum  == calculatedChecksum;
            }

            void toLinkedBytes(LinkedBytes<uint8_t> &linkedBytes){
                // Deep copy of MessagePayload to referenced linked list
                linkedBytes = MessagePayload;
                linkedBytes.push(MsgID);  // Push value to tail
                linkedBytes.push(MsgSize);  // Push value to tail
                linkedBytes.push(SOM);  // Push value to tail
                linkedBytes.add(Checksum);  // Add value to header
            };
    };
    
    class Message{
        public:
            std::string msgName;
            uint8_t msgSOM = 0xAA;
            uint8_t msgSize;
            uint8_t msgID;
            
            
            void decodeMessage(MessagePacket &messagePacket){
                messagePacket.MessagePayload;
            };

            void encodeMessage(){
                LinkedBytes<uint8_t> *tx_message = new LinkedBytes<uint8_t>();
                MessagePacket message{};
                message.SOM = msgSOM;
                message.MsgSize = msgSize;
                message.MsgID = msgID;
                message.Checksum = message.calcChecksum();
                message.toLinkedBytes(*tx_message);
                delete tx_message;

            };
    };

    class ControlRover: public Message{
        ControlRover(){
            msgName = "ControlRover";
            uint8_t msgSize;
            uint8_t msgID;
        };
    };

    class CalibrateRover: public Message{
        CalibrateRover(){
            msgName = "CalibrateRover";
        };     
    };

    class RoverState: public Message{
        RoverState(){
            msgName = "RoverState";
        };      
    };

    class RoverIMU: public Message{
        RoverIMU(){
            msgName = "RoverIMU";
        };
    };

    class RoverError: public Message{
        RoverError(){
            msgName = "RoverError";
        };
    };
}

#endif  // MESSAGE_HPP