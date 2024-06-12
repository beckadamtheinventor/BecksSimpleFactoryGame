#ifndef __GENERATOR_FUNCTION_HPP__
#define __GENERATOR_FUNCTION_HPP__

#include "ScriptBytecode.hpp"
#include "ScriptAssemblyCompiler.hpp"
#include "BlockPos.hpp"

namespace Components {
    class GeneratorFunction {
        public:
        ScriptBytecode *script;
        unsigned int id;
        unsigned int prob;
        unsigned int runs;
        public:
        enum Type {
            None = 0,
            Base,
            Feature,
            Structure,
        };
        Type type;
        GeneratorFunction() : GeneratorFunction(0) {}
        ~GeneratorFunction() {
            if (this->script != nullptr) {
                delete [] this->script;
            }
        }
        GeneratorFunction(unsigned int id) {
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

        // Run the Script Bytecode, passing along the ground height value at this XZ position and a block position.
        // Returns up to 8 values in rvalues.
        // # Base generator script outputs
        // 0th is the topsoil block, 1st is the soil block, 2nd is the soil depth, 3rd is the underground block, 4th is the bedrock block,
        // 5th is the above topsoil block, 6th is the above topsoil depth
        // 7th is used to determine how many times the generator needs to be called.
        // if the 7th is 0 (default) run this function for every block column.
        // if the 7th is less than 0, run this function once per *chunk* column.
        // otherwise, the 7th specifies how many generator steps to skip.
        // Other generator script outputs are currently unused.
        bool run(long long height, BlockPos pos, long long *rvalues) {
            long long *argv = new long long[]{height, pos.x, pos.y, pos.z};
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
                    printf("%s error generating block at position %lld,%lld,%lld in generator script ID %u\n", errstr.c_str(), pos.x, pos.y, pos.z, this->id);
                    exit(1);
                }
                return true;
            }
            return false;
        }
        unsigned int getId() {
            return id;
        }
    };
}
#endif