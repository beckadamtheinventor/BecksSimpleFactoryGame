#ifndef __TICKING_BLOCK_H__
#define __TICKING_BLOCK_H__

#include "Block.hpp"
#include "BlockPos.hpp"
#include "TickingFunction.hpp"
#include "TickingFunctionRegistry.hpp"

extern Components::TickingFunctionRegistry GlobalTickingFunctionRegistry;

namespace Components {

    class TickingBlock {
        Block *block;
        BlockPos pos;
        TickingFunction *fn;
        public:
        TickingBlock() : TickingBlock(new Block(0), 0, 0, 0) {}
        TickingBlock(Block *block, long long x, long long y, long long z) {
            this->block = block;
            this->pos = BlockPos(x, y, z);
            this->fn = nullptr;
        }
        void setTick(TickingFunction* fn) {
            this->fn = fn;
        }
        BlockPos getPosition() {
            return pos;
        }
        // returns a tickresult
        TickingFunction::TickResult tick() {
            if (this->block->isTicking) {
                if (this->block->randomtick > 0) {
                    if (rand() % this->block->randomtick != 0) {
                        return TickingFunction::TickResult::DoNothing;
                    }
                }
                if (this->fn == nullptr) {
                    this->fn = GlobalTickingFunctionRegistry.getById(block->function);
                    if (this->fn == nullptr) {
                        this->block->isTicking = false;
                        return TickingFunction::TickResult::DoNothing;
                    }
                }
            } else {
                return TickingFunction::TickResult::DoNothing;
            }
            return _tick();
        }

        TickingFunction::TickResult _tick() {
            long long values[8];
            return fn->run(this->block, this->pos, values);
        }
    };
}

#endif