#ifndef __COORDINATE_KEYED_MAP__
#define __COORDINATE_KEYED_MAP__

#include <stdint.h>

#include "BlockPos.hpp"

namespace Components {
    template <class V, size_t WIDTH>
    class CoordinateKeyedMap {
        typedef struct Symbol {
            BlockPos pos;
            struct Symbol *next;
            V value;
        } Symbol;
        Symbol *items[WIDTH*WIDTH] = {nullptr};
        size_t index(BlockPos pos) {
            return index(pos.x, pos.z);
        }
        size_t index(long long x, long long z) {
            return (x & (WIDTH - 1)) + (z & (WIDTH - 1)) * WIDTH;
        }
        public:

        CoordinateKeyedMap<V, WIDTH>() {
            for (size_t i=0; i<WIDTH*WIDTH; i++) {
                items[i] = nullptr;
            }
        }

        // clear all items
        void clear() {
            for (size_t z=0; z<WIDTH; z++) {
                for (size_t x=0; x<WIDTH; x++) {
                    size_t i = index(x, z);
                    if (items[i] != nullptr) {
                        Symbol *sym = items[i];
                        while (sym->next) {
                            Symbol *next = sym->next;
                            delete sym;
                            sym = next;
                        }
                        delete items[i];
                        items[i] = nullptr;
                    }
                }
            }
        }

        // return the width of the coordinate map
        size_t getWidth() {
            return WIDTH;
        }

        // pop the first item from a given coordinate
        V popItem(BlockPos pos) {
            return popItem(pos.x, pos.z);
        }

        V popItem(long long x, long long z) {
            x = (x & ~(CHUNK_W*REGION_W)) / (CHUNK_W*REGION_W);
            z = (z & ~(CHUNK_W*REGION_W)) / (CHUNK_W*REGION_W);
            size_t i = index(x, z);
            Symbol *sym = items[i];
            if (sym != nullptr) {
                items[i] = sym->next;
            }
            return sym->value;
        }

        // set value at position
        V set(BlockPos pos, V value) {
            pos = pos.regionPos() / (CHUNK_W*REGION_W);
            size_t i = index(pos);
            Symbol *sym = items[i];
            if (sym != nullptr) {
                Symbol *prev = sym;
                while (sym->next != nullptr) {
                    sym = sym->next;
                }
                sym = prev->next = new Symbol {pos, nullptr, value};
            } else {
                sym = items[i] = new Symbol {pos, nullptr, value};
            }
            return sym->value;
        }

        // returns true if there is a value at the position, false otherwise.
        bool get(BlockPos pos, V *value) {
            pos = pos.regionPos() / (CHUNK_W*REGION_W);
            Symbol *sym = items[index(pos)];
            if (sym == nullptr) {
                return false;
            }
            while (sym->pos != pos) {
                sym = sym->next;
                if (sym == nullptr) {
                    return false;
                }
            }
            *value = sym->value;
            return true;
        }
    };
}

#endif