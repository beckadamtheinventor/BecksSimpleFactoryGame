#ifndef __SCRIPT_ASSEMBLY_COMPILER_HPP__
#define __SCRIPT_ASSEMBLY_COMPILER_HPP__

#include "../../json/json.hpp"
#include "DynamicArray.hpp"
#include <string.h>

namespace Components {
    class ScriptAssemblyCompiler {
        JSON::HashedSymList<size_t> vars;
        typedef struct {
            size_t offset;
            size_t lno;
            const char *label;
        } labelusage_t;
        JSON::HashedSymList<size_t> labels;
        DynamicArray<labelusage_t, 128> labelusages;
        DynamicArray<unsigned char, 512> outbuf;
        long long token_int;
        char *token_str;
        size_t lno;
        
        enum Token {
            Nop,
            Return, ReturnDoNothing, ReturnFail, ReturnDestroy, ReturnPlace, ReturnKeep, ReturnUpdate, ReturnReverseUpdate,
            End, Frameset, ReadArg, Random, LoadVar, StoreVar, Exchange,
            Immediate8, Immediate16, Immediate32, Immediate8U, Immediate16U, Immediate32U,
            Immediate8B, Immediate16B, Immediate32B, Immediate8UB, Immediate16UB, Immediate32UB,
            Add, Sub, Mul, Div, Mod, And, Or, Xor, Lor, Land, Inc, Dec,
            AddF, SubF, MulF, DivF, ModF, PowF, NanF, InfF,
            Push, Pop, PushB, PopB, BA, BZ, BNZ, JSR, RTS, JSRZ, JSRNZ, RTSZ, RTSNZ,
            EQ, NEQ, GT, LT, GTEQ, LTEQ, EQF, NEQF, GTF, LTF, GTEQF, LTEQF,
            BZSet32, BNZSet32, PushArg, PushVar, Abs, AbsF, Sqrt, SqrtF, Itof, Ftoi,
            GetBlock = 0x80, SetBlock, BreakBlock,
            Check3x3x3, Check3x3, Check5x5x5, Check5x5,
            isSolid, isOpaque, isTicking,
            ReplaceBlocks3x3x3, ReplaceBlocks3x3,
            ReplaceBlocks5x5x5, ReplaceBlocks5x5,
            SpreadBlocks,
            None=0xF0, Integer, Label, LabelUsage,
        };
        public:
        ScriptAssemblyCompiler();
        char peek(const char *data, size_t datalen, size_t i);
        size_t compile(const char *data, size_t datalen, unsigned char **out);
        Token next(const char *data, size_t datalen, size_t &i);
    };
}

#endif