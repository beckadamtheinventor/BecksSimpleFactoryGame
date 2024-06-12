#ifndef __QUEUE_HPP__
#define __QUEUE_HPP__

#include <exception>
namespace Components {
    template<class T, size_t SIZE>
    class Queue {
        size_t readoffset;
        size_t writeoffset;
        size_t length;
        T items[SIZE];
        public:
        Queue() {
            length = readoffset = writeoffset = 0;
        }
        // returns the allocated length of the queue
        size_t Length() {
            return SIZE;
        }
        // returns the number of unread items available in the queue
        size_t available() {
            return length;
        }
        // returns the number of unwritten items in the queue
        size_t remaining() {
            return SIZE-length;
        }
        // clears the queue by resetting the read/write offsets
        void clear() {
            length = readoffset = writeoffset = 0;
        }
        // peek and return the item at the current read offset
        T peek() {
            readoffset %= SIZE;
            return items[readoffset];
        }
        // pop and return the item at the current read offset
        // throws an exception if there's nothing left to pop
        T pop() {
            readoffset %= SIZE;
            if (length == 0) {
                printf("Error: Can't pop queue with no available entries!");
                throw std::exception();
            }
            length--;
            return items[readoffset++];
        }
        // append a new item to the queue
        // throws an exception if there's no space left
        void append(T item) {
            if (length >= SIZE) {
                printf("Error: Queue not long enough!\n");
                throw std::exception();
            }
            items[writeoffset++] = item;
            length++;
            writeoffset %= SIZE;
        }

    };
}

#endif