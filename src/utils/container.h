#ifndef CONTAINER_H
#define CONTAINER_H 

#include <string>
#include <cstdlib>

namespace container{
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
    class LinkedList{
        uint32_t count = 0;
        Node<T> *head = NULL;
        Node<T> *tail = NULL;
        
        public:
            LinkedList(){};

            ~LinkedList(){
                if (head==NULL) return;

                clear();
            };

            LinkedList(const LinkedList &srcLinkedList){
                // Copy constructor creates the list by taking deep copy of source list
                clear();
                if (srcLinkedList.head == NULL){
                    head = NULL;
                    tail = NULL;
                    return;
                }

                tail = new Node<T>();
                Node<T> *ptrSrc = srcLinkedList.tail;
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
                this->count = srcLinkedList.count;
            }

            void operator=(LinkedList &srcLinkedList){
                // Assignment operator creates the list by taking deep copy of source list
                clear();
                if (srcLinkedList.head == NULL){
                    head = NULL;
                    tail = NULL;
                    return;
                }

                tail = new Node<T>();
                Node<T> *ptrSrc = srcLinkedList.tail;
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
                this->count = srcLinkedList.count;
            };

            LinkedList &operator+(LinkedList &newLinkedList){
                // Overloaded function creates extends current list by taking deep copy of new list
                if (newLinkedList.head == NULL) return *this;
                
                Node<T> *ptrSrc = newLinkedList.tail;
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
                this->count += newLinkedList.count;
                return *this;
            }
            
            LinkedList &operator+=(LinkedList &newLinkedList){
                // Overloaded function creates extends current list by taking deep copy of new list
                if (head == NULL){
                    *this = newLinkedList;
                    return *this;
                } 

                if (newLinkedList.head == NULL) return *this;
                
                Node<T> *ptrSrc = newLinkedList.tail;
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
                this->count += newLinkedList.count;
                return *this;
            }
                       
            T operator[](int i) const {return get(i);}  // getter

            T &operator[](int i) {return get(i);}  // setter

            LinkedList<T> operator()(int begin, int final) {
                // out is a subset in the range (begin, final) of original linked list. 
                // element of final index is not included in the output.
                if (begin<0) begin = std::abs((int) count + begin) % count;
                if (final<0) final = std::abs((int) count + final) % count;

                if (count < final) throw "Out of bound error";
                if (begin > final) throw "Starting index should be greater final index";

                LinkedList<T> out;
                Node<T> *ptr = tail;
                size_t i = 0;
                while (ptr != NULL){
                    if (i>= begin && i<final){
                        out.add(ptr->val);                        
                    }

                    if (i>=final) break;

                    ptr = ptr->next;
                    i++;
                }
                return out;
            }

            void operator()(int begin, int final, LinkedList<T> &out) const {
                // out is a subset in the range (begin, final) of original linked list. 
                // element of final index is not included in the output.
                if (begin<0) begin = std::abs((int) count + begin) % count;
                if (final<0) final = std::abs((int) count + final) % count;

                if (count < final) throw "Out of bound error";
                if (begin > final) throw "Starting index should be greater final index";

                Node<T> *ptr = tail;
                size_t i = 0;
                while (ptr != NULL){
                    if (i>= begin && i<final){
                        out.add(ptr->val);                        
                    }

                    if (i>=final) break;

                    ptr = ptr->next;
                    i++;
                }
            }

            Node<T> *begin(){
                // Returns head
                return head;
            }

            Node<T> *end(){
                // Returns tail
                return tail;
            }

            size_t Count(){
                // Returns number of nodes in the list
                return count;
            };

            bool isEmpty(){
                // Checks is list empty
                if (head==NULL) 
                    return true;
                else 
                    return false;
            };

            void add(T val){
                // Add new value after header node of the linked list
                Node<T> *node = new Node<T>();
                node->val = val;
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
            
            void add(char *c_, int size) {
                // Add char array with specified size
                for (int i=0; i<size; i++){
                    add(c_[i]);
                }
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
                count ++;
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

            void remove(int index){
                // Remove elements according to index of first added element
                if (tail == NULL) throw "Empty List Error!";

                if (index>count) throw "Index Out of Bound Error!";

                Node<T> *ptr = tail;
                while(ptr != NULL){
                    if (index == 0){
                        if (ptr == tail){
                            tail = ptr->next;
                            if (tail != NULL) tail->prev = NULL;
                            else head = NULL;
                            delete ptr;
                        }
                        else if (ptr == head){
                            head = ptr->prev;
                            if (head != NULL) head->next = NULL;
                            else head = NULL;
                            delete ptr;
                        }
                        else {
                            Node<T> *nextNode = ptr->next;
                            Node<T> *prevNode = ptr->prev;
                            if (nextNode!=NULL) nextNode->prev = prevNode;
                            if (prevNode!=NULL) prevNode->next = nextNode;
                            delete ptr;
                            ptr = NULL;
                        }
                        count--;
                        return;
                    }
                    ptr = ptr->next;
                    index--;
                }                
            };

            void remove(Node<T> &val){
                // Remove Node from the list
                if (tail == NULL) throw "Empty List Error!";

                if (&val == NULL) throw "NULL pointer cannot be removed!";

                Node<T> *ptr = tail;
                while(ptr != NULL){
                    if (ptr == &val){
                        if (ptr == tail){
                            tail = ptr->next;
                            if (tail != NULL) tail->prev = NULL;
                            else head = NULL;
                            delete ptr;
                        }
                        else if (ptr == head){
                            head = ptr->prev;
                            if (head != NULL) head->next = NULL;
                            else head = NULL;
                            delete ptr;
                        }
                        else {
                            Node<T> *nextNode = ptr->next;
                            Node<T> *prevNode = ptr->prev;
                            if (nextNode!=NULL) nextNode->prev = prevNode;
                            if (prevNode!=NULL) prevNode->next = nextNode;
                            delete ptr;
                            ptr = NULL;
                        }
                        count--;
                        return;
                    }
                    ptr = ptr->next;
                }
            };

            void remove(T val){
                // Removes first occurance of the value starting from first added Node
                if (tail == NULL) throw "Empty List Error!";

                Node<T> *ptr = tail;
                while(ptr != NULL){
                    if (ptr->val == val){
                        if (ptr == tail){
                            tail = ptr->next;
                            if (tail != NULL) tail->prev = NULL;
                            else head = NULL;
                            delete ptr;
                        }
                        else if (ptr == head){
                            head = ptr->prev;
                            if (head != NULL) head->next = NULL;
                            else head = NULL;
                            delete ptr;
                        }
                        else {
                            Node<T> *nextNode = ptr->next;
                            Node<T> *prevNode = ptr->prev;
                            if (nextNode!=NULL) nextNode->prev = prevNode;
                            if (prevNode!=NULL) prevNode->next = nextNode;
                            delete ptr;
                            ptr = NULL;
                        }
                        count--;
                        return;
                    }
                    ptr = ptr->next;
                }
            };

            void removeFirst(){
                // Remove from tail
                remove(*tail);
            };

            void removeLast(){
                // Remove from head
                remove(*head);
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
                // Clears list
                if (head==NULL) return;

                Node<T> *ptr = head;
                while(ptr != NULL){
                    Node<T> *temp = ptr;
                    ptr = ptr->prev;
                    delete temp;
                }
                count = 0;
                
                head = NULL;
                tail = NULL;
            }

            std::string toString(){
                // Converts value of each node into string
                std::string result = "";
                if (tail == NULL) return result;

                Node<T> *ptr = tail;
                while(ptr != NULL){
                    result += std::to_string(ptr->val) + " ";
                    ptr = ptr->next;
                }
                return result;
            };

            template<typename V>
            std::vector<V> toVector(){
                // Converts value of each node into std::vector with appropriate type
                std::vector<uint8_t> result;
                if (tail == NULL) return result;

                Node<T> *ptr = tail;
                while(ptr != NULL){
                    result.push_back(static_cast<T>(ptr->val));
                    ptr = ptr->next;
                }
                return result;
            };

            uint8_t toUint8(){
                if (count<1) throw "type does not match with size of link list";
                uint8_t result = 0;
                Node<T> *ptr = tail;
                size_t i = 0;
                while(ptr != NULL){
                    if (i==1) break;
                    result |= (ptr->val)<<(8*i);
                    ptr = ptr->next;
                    i++;
                }
                return (uint8_t) result;
            };

            uint16_t toUint16(char endian='l'){
                if (count<2) throw "type does not match with size of link list";
                uint16_t result = 0;
                Node<T> *ptr = tail;
                size_t i = 0;
                while(ptr != NULL){
                    if (i==2) break;
                    result |= (ptr->val)<<(8*i);
                    ptr = ptr->next;
                    i++;
                }
                return (uint16_t) result;
            };

            uint32_t toUint32(){
                if (count<4) throw "type does not match with size of link list";
                uint32_t result = 0;
                Node<T> *ptr = tail;
                size_t i = 0;
                while(ptr != NULL){
                    if (i==4) break;
                    result |= (ptr->val)<<(8*i);
                    ptr = ptr->next;
                    i++;
                }
                return (uint32_t) result;
            };

            uint64_t toUint64(){
                if (count<8) throw "type does not match with size of link list";
                uint64_t result = 0;
                Node<T> *ptr = tail;
                size_t i = 0;
                while(ptr != NULL){
                    if (i==8) break;
                    result |= (ptr->val)<<(8*i);
                    ptr = ptr->next;
                    i++;
                }
                return (uint64_t) result;
            };

            int8_t toInt8(){
                if (count<1) throw "type does not match with size of link list";
                int8_t result = 0;
                Node<T> *ptr = tail;
                size_t i = 0;
                while(ptr != NULL){
                    if (i==4) break;
                    result |= (ptr->val)<<(8*i);
                    ptr = ptr->next;
                    i++;
                }
                return (int8_t) result;
            };

            int16_t toInt16(){
                if (count<2) throw "type does not match with size of link list";
                int16_t result = 0;
                Node<T> *ptr = tail;
                size_t i = 0;
                while(ptr != NULL){
                    if (i==2) break;
                    result |= (ptr->val)<<(8*i);
                    ptr = ptr->next;
                    i++;
                }
                return (int16_t) result;
            };

            int32_t toInt32(){
                if (count<4) throw "type does not match with size of link list";
                int32_t result = 0;
                Node<T> *ptr = tail;
                size_t i = 0;
                while(ptr != NULL){
                    if (i==4) break;
                    result |= (ptr->val)<<(8*i);
                    ptr = ptr->next;
                    i++;
                }
                return (int32_t) result;
            };

            int64_t toInt64(){
                if (count<8) throw "type does not match with size of link list";
                int64_t result = 0;
                Node<T> *ptr = tail;
                size_t i = 0;
                while(ptr != NULL){
                    if (i==8) break;
                    result |= (ptr->val)<<(8*i);
                    ptr = ptr->next;
                    i++;
                }
                return (int64_t) result;
            };

            long toFloat(){
                if (count<4) throw "type does not match with size of link list";
                int32_t result = 0;
                Node<T> *ptr = tail;
                size_t i = 0;
                while(ptr != NULL){
                    if (i==4) break;
                    result |= (ptr->val)<<(8*i);
                    ptr = ptr->next;
                    i++;
                }
                return static_cast<float>(result);
            };

            long long toDouble(){
                if (count<8) throw "type does not match with size of link list";
                int64_t result = 0;
                Node<T> *ptr = tail;
                size_t i = 0;
                while(ptr != NULL){
                    if (i==8) break;
                    result |= (ptr->val)<<(8*i);
                    ptr = ptr->next;
                    i++;
                }
                return static_cast<double>(result);
            };

            uint8_t toUint8L(){
                if (count<1) throw "type does not match with size of link list";
                uint8_t result = 0;
                Node<T> *ptr = head;
                size_t i = 0;
                while(ptr != NULL){
                    if (i==1) break;
                    result |= (ptr->val)<<(8*i);
                    ptr = ptr->prev;
                    i++;
                }
                return (uint8_t) result;
            };

            uint16_t toUint16L(){
                if (count<2) throw "type does not match with size of link list";
                uint16_t result = 0;
                Node<T> *ptr = head;
                size_t i = 0;
                while(ptr != NULL){
                    if (i==2) break;
                    result |= (ptr->val)<<(8*i);
                    ptr = ptr->prev;
                    i++;
                }
                return (uint16_t) result;
            };

            uint32_t toUint32L(){
                if (count<4) throw "type does not match with size of link list";
                uint32_t result = 0;
                Node<T> *ptr = head;
                size_t i = 0;
                while(ptr != NULL){
                    if (i==4) break;
                    result |= (ptr->val)<<(8*i);
                    ptr = ptr->prev;
                    i++;
                }
                return (uint32_t) result;
            };

            uint64_t toUint64L(){
                if (count<8) throw "type does not match with size of link list";
                uint64_t result = 0;
                Node<T> *ptr = head;
                size_t i = 0;
                while(ptr != NULL){
                    if (i==8) break;
                    result |= (ptr->val)<<(8*i);
                    ptr = ptr->prev;
                    i++;
                }
                return (uint64_t) result;
            };

            int8_t toInt8L(){
                if (count<1) throw "type does not match with size of link list";
                int8_t result = 0;
                Node<T> *ptr = head;
                size_t i = 0;
                while(ptr != NULL){
                    if (i==4) break;
                    result |= (ptr->val)<<(8*i);
                    ptr = ptr->prev;
                    i++;
                }
                return (int8_t) result;
            };

            int16_t toInt16L(){
                if (count<2) throw "type does not match with size of link list";
                int16_t result = 0;
                Node<T> *ptr = head;
                size_t i = 0;
                while(ptr != NULL){
                    if (i==2) break;
                    result |= (ptr->val)<<(8*i);
                    ptr = ptr->prev;
                    i++;
                }
                return (int16_t) result;
            };

            int32_t toInt32L(){
                if (count<4) throw "type does not match with size of link list";
                int32_t result = 0;
                Node<T> *ptr = head;
                size_t i = 0;
                while(ptr != NULL){
                    if (i==4) break;
                    result |= (ptr->val)<<(8*i);
                    ptr = ptr->prev;
                    i++;
                }
                return (int32_t) result;
            };

            int64_t toInt64L(){
                if (count<8) throw "type does not match with size of link list";
                int64_t result = 0;
                Node<T> *ptr = head;
                size_t i = 0;
                while(ptr != NULL){
                    if (i==8) break;
                    result |= (ptr->val)<<(8*i);
                    ptr = ptr->prev;
                    i++;
                }
                return (int64_t) result;
            };

            long toFloatL(){
                if (count<4) throw "type does not match with size of link list";
                int32_t result = 0;
                Node<T> *ptr = head;
                size_t i = 0;
                while(ptr != NULL){
                    if (i==4) break;
                    result |= (ptr->val)<<(8*i);
                    ptr = ptr->prev;
                    i++;
                }
                return static_cast<float>(result);
            };

            long long toDoubleL(){
                if (count<8) throw "type does not match with size of link list";
                int64_t result = 0;
                Node<T> *ptr = head;
                size_t i = 0;
                while(ptr != NULL){
                    if (i==8) break;
                    result |= (ptr->val)<<(8*i);
                    ptr = ptr->prev;
                    i++;
                }
                return static_cast<double>(result);
            };

    };

}
#endif  // CONTAINER_H

