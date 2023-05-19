#ifndef CONTAINER_H
#define CONTAINER_H 

#include <string>

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
        int count = 0;
        Node<T> *head = NULL;
        Node<T> *tail = NULL;
        
        public:
            LinkedList(){};

            ~LinkedList(){
                if (head==NULL) return;

                Node<T> *ptr = head;
                while(ptr != NULL){
                    Node<T> *temp = ptr;
                    ptr = ptr->prev;
                    delete temp;
                }
            };

            LinkedList(const LinkedList &srcLinkedBytes){
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

            LinkedList &operator+(LinkedList &newLinkedBytes){
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
            
            LinkedList &operator+=(LinkedList &newLinkedBytes){
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

            void remove(int index){
                // Remove elements according to index of first added element
                if (tail == NULL) throw "Empty List Error!";

                if (index>count) throw "Index Out of Bound Error!";

                Node<T> *ptr = tail;
                while(ptr != NULL){
                    if (index==0){
                        delete ptr;
                        return;
                    }
                    ptr = ptr->next;
                    index--;
                }
            };

            void remove(T val){
                if (tail == NULL) throw "Empty List Error!";

                if (val == NULL) throw "NULL pointer cannot be removed!";

                Node<T> *ptr = tail;
                while(ptr != NULL){
                    if (ptr->val == val){
                        Node<T> *prevNode = ptr->prev;
                        Node<T> *nextNode = ptr->next;
                        if (prevNode!=NULL) prevNode->next = nextNode;
                        if (nextNode!=NULL) nextNode->prev = prevNode;
                         
                        delete ptr;
                        return;
                    }
                    ptr = ptr->next;
                }
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
}
#endif  // CONTAINER_H

