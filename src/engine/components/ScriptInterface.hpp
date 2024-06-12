#ifndef __SCRIPT_INTERFACE_HPP__
#define __SCRIPT_INTERFACE_HPP__

#include <stdint.h>

namespace Components {
    class ScriptInterface {
        public:
        ScriptInterface();
        size_t getBlock(long long x, long long y, long long z);
        void setBlock(long long x, long long y, long long z, size_t b);
        void breakBlock(long long x, long long y, long long z);
        size_t check3x3(long long x, long long y, long long z, size_t b);
        size_t check3x3x3(long long x, long long y, long long z, size_t b);
        size_t check5x5(long long x, long long y, long long z, size_t b);
        size_t check5x5x5(long long x, long long y, long long z, size_t b);
        void replaceBlocks3x3(long long x, long long y, long long z, size_t r, size_t b);
        void replaceBlocks3x3x3(long long x, long long y, long long z, size_t r, size_t b);
        void replaceBlocks5x5(long long x, long long y, long long z, size_t r, size_t b);
        void replaceBlocks5x5x5(long long x, long long y, long long z, size_t r, size_t b);
        void spreadBlocks(long long x, long long y, long long z, size_t r, size_t b);
        bool isSolid(size_t b);
        bool isOpaque(size_t b);
        bool isTicking(size_t b);
    };
}

#endif