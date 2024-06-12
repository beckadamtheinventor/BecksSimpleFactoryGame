#ifndef __TICKING_FUNCTION_H__
#define __TICKING_FUNCTION_H__

#include "Block.hpp"
#include "BlockPos.hpp"
#include "ScriptBytecode.hpp"
#include "ScriptAssemblyCompiler.hpp"

namespace Components {
    class TickingFunction {
        ScriptBytecode *script;
        unsigned int id;
        public:
        enum TickResult {
            DoNothing = 0,
            Fail,
            Destroy,
            Place,
            Keep,
            Update,
            ReverseUpdate,
        };
        TickingFunction() : TickingFunction(0) {}
        ~TickingFunction() {
            if (this->script != nullptr) {
                delete [] this->script;
            }
        }
        TickingFunction(unsigned int id) {
            this->id = id;
            this->script = nullptr;
        }
        // Load Script Bytecode from binary data.
        bool loadScript(const unsigned char *data, size_t len) {
            if (len == 0) {
                return false;
            }
            script = new ScriptBytecode(data, len);
            return true;
        }
        // Load Script assembly from text data.
        bool loadScriptSource(const char *data, size_t len) {
            ScriptAssemblyCompiler compiler = ScriptAssemblyCompiler();
            unsigned char *outdata;
            size_t outlen = compiler.compile(data, len, &outdata);
            return loadScript(outdata, outlen);
        }

        // Run the Script Bytecode, passing along the attached Block ID and BlockPos to it.
        // Returns up to 8 values in rvalues, currently only the 0th is used to specify a TickResult.
        // Returns true if a mesh update is needed. (tick resulted in a block update)
        TickResult run(Block *block, BlockPos pos, long long *rvalues) {
            long long *argv = new long long[]{block->getId(), pos.x, pos.y, pos.z};
            if (script != nullptr) {
                int result = script->run(4, argv, rvalues);
                std::string errstr = "";
                switch (result) {
                    case ScriptBytecode::UnknownOpcode:
                        errstr = "Unknown Opcode";
                        break;
                    case ScriptBytecode::Timeout:
                        errstr = "Timeout";
                        break;
                    case ScriptBytecode::StackUnderflow:
                        errstr = "Stack Underflow";
                        break;
                    case ScriptBytecode::StackOverflow:
                        errstr = "Stack Overflow";
                        break;
                    case ScriptBytecode::OutOfBoundsRead:
                        errstr = "Out of Bounds Read";
                        break;
                    case ScriptBytecode::OutOfBoundsWrite:
                        errstr = "Out of Bounds Write";
                        break;
                    case ScriptBytecode::OutOfBoundsExec:
                        errstr = "Out of Bounds Execution";
                        break;
                    case ScriptBytecode::Success:
                        break;
                    default:
                        errstr = "Unspecified";
                        break;
                }
                if (errstr.length() > 0) {
                    FILE *fd = fopen("dump.bin", "wb");
                    script->dump(fd);
                    fclose(fd);
                    printf("%s error ticking block at position %lld,%lld,%lld ID %u script ID %u\n", errstr.c_str(), pos.x, pos.y, pos.z, block->getId(), this->id);
                    exit(1);
                }
                return (TickResult)rvalues[0];
            }
            return TickResult::Fail;
        }
        unsigned int getId() {
            return id;
        }
    };
}

#endif