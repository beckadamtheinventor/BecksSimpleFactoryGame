#ifndef __DYNAMIC_ARRAY_HPP__
#define __DYNAMIC_ARRAY_HPP__

namespace Components {
    template <class T, size_t MIN_ALLOC>
    class DynamicArray {
        size_t length;
        T items[MIN_ALLOC];
        DynamicArray<T, MIN_ALLOC> *next;
        public:
        DynamicArray() {
            length = 0;
            next = nullptr;
        }
/*
        ~DynamicArray() {
            if (length > MIN_ALLOC && this->next != nullptr) {
                delete this->next;
            }
            length = 0;
        }
*/
        size_t Length() {
            return length;
        }
        void clear() {
            if (length > MIN_ALLOC && this->next != nullptr) {
                delete this->next;
            }
            length = 0;
        }
        // return the Nth allocated block if it exists. Starts at 0.
        DynamicArray<T, MIN_ALLOC> *getBlockNo(size_t num) {
            DynamicArray<T, MIN_ALLOC> *arr = this;
            for (size_t i=1; i<num; i++) {
                if (arr == nullptr) {
                    return nullptr;
                }
                arr = arr->next;
            }
            return arr;
        }
        T* append(T value) {
            length++;
            if (length <= MIN_ALLOC) {
                return &(this->items[length-1] = value);
            }
            if (this->next == nullptr) {
                this->next = new DynamicArray<T, MIN_ALLOC>();
            }
            return this->next->append(value);
        }
        T* operator[](size_t i) {
            DynamicArray<T, MIN_ALLOC> *arr = this;
            while (i >= MIN_ALLOC) {
                if (arr->next == nullptr) {
                    return nullptr;
                }
                arr = arr->next;
                i -= MIN_ALLOC;
            }
            return &arr->items[i];
        }
        T* collapse() {
            T* values = new T[length];
            DynamicArray<T, MIN_ALLOC> *arr = this;
            size_t j = 0;
            do {
                size_t l = length < MIN_ALLOC ? length : MIN_ALLOC;
                for (int i=0; i<l; i++) {
                    values[j++] = arr->items[i];
                }
                arr = arr->next;
            } while (arr);
            return values;
        }
    };
}

#endif