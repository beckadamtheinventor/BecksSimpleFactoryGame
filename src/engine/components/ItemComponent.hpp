#ifndef __ITEM_COMPONENT_H__
#define __ITEM_COMPONENT_H__

#include <malloc.h>
#include <string.h>
#include "VarInt.hpp"

#define STRING_MAX_SIZE 65534

namespace Components {
    class ItemComponent {
        public:
        enum Type {
            Null = 0,
            Int,
            String,
            IntArray,
        };
        private:
        Type type;
        unsigned int len;
        union {
            const char *s;
            long long i;
            long long *a;
        };
        public:
        ItemComponent() {
            this->type = Type::Null;
            this->i = 0;
            this->len = 0;
        }
        /* Initialize this ItemComponent as an integer. */
        ItemComponent(long long value) {
            this->type = Type::Int;
            this->i = value;
            this->len = 0;
        }
        /* Initialize this ItemComponent as a string. Ensure the data does not get freed unexpectedly. */
        ItemComponent(const char *value, size_t len) {
            this->type = Type::String;
            this->s = value;
            this->len = len;
        }
        /* Initialize this ItemComponent as a size-prefixed array of integers. Ensure the data does not get freed unexpectedly. */
        ItemComponent(long long *value, size_t len) {
            this->type = Type::IntArray;
            this->a = value;
            this->len = len;
        }
        bool getInt(long long &v) {
            if (type != Type::Int)
                return false;
            v = this->i;
            return true;
        }
        bool getString(const char *&v) {
            if (type != Type::String)
                return false;
            v = this->s;
            return true;
        }
        bool getStringLength(long long &v) {
            if (type != Type::String)
                return false;
            v = this->len;
            return true;
        }
        bool getArray(long long *&v) {
            if (type != Type::IntArray)
                return false;
            v = this->a;
            return true;
        }
        bool getArrayLength(long long &v) {
            if (type != Type::IntArray)
                return false;
            v = this->len;
            return true;
        }
        size_t serialize(unsigned char *buffer) {
            size_t j = 0;
            buffer[j++] = this->type;
            switch (this->type) {
                case Int:
                    return VarInt(this->i).write(buffer);
                case String:
                    VarInt(this->len).write(buffer, j);
                    for (size_t k=0; k<this->len; k++) {
                        buffer[j++] = s[k];
                    }
                    return j;
                case IntArray:
                    VarInt(this->len).write(buffer, j);
                    while (j < this->len) {
                        VarInt(a[j]).write(buffer, j);
                    }
                    return j;
                case Null:
                default:
                    break;
            }
            return 0;
        }
        static ItemComponent deserialize(unsigned char *buffer, size_t len, size_t &i) {
            size_t j, l;
            long long *a;
            char *s;
            switch (buffer[i++]) {
                case Int:
                    return ItemComponent(VarInt(buffer, i));
                case String:
                    l = VarInt(buffer, i);
                    s = (char*)alloca(l);
                    j = 0;
                    if (s == NULL) {
                        // TODO: Panic
                        return ItemComponent();
                    }
                    do {
                        s[j++] = buffer[i];
                    } while (buffer[i++]>0);
                    return ItemComponent(s, l);
                case IntArray:
                    l = VarInt(buffer, i);
                    a = (long long*)alloca(l * sizeof(long long));
                    for (j=0; j<l; j++) {
                        a[j] = VarInt(buffer, i);
                    }
                    return ItemComponent(a, l);
                case Null:
                default:
                    return ItemComponent();
            }
        }
    };
}

#endif