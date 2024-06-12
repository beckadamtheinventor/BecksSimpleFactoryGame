#ifndef __BLOCKSTATE_H__
#define __BLOCKSTATE_H__

#include "Block.hpp"
#include "BlockPos.hpp"
#include "Types.hpp"

namespace Components {
    class BlockState {
        protected:
        Block *block;
        BlockPos pos;
        public:
        BlockState() : BlockState(NULL) {}
        BlockState(Block *block) : BlockState(block, BlockPos()) {}
        BlockState(Block *block, long long x, long long y, long long z) : BlockState(block, BlockPos(x, y, z)) {}
        BlockState(Block *block, BlockPos pos) {
            if (block == NULL) {
                block = new Block(0);
            }
            this->block = block;
            this->pos = pos;
        }
        BlockPos getPosition() {
            return pos;
        }
        Block *getBlock() {
            return block;
        }
        Block *setBlock(Block *block) {
            return (this->block = block);
        }
        blockid_t getId() {
            return block->getId();
        }
    };
}

#endif