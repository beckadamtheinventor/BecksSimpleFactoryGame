#ifndef __SCRIPTBYTECODE_H__
#define __SCRIPTBYTECODE_H__

#include <cmath>
#include <stdio.h>
#include "ScriptInterface.hpp"

namespace Components {
    class ScriptBytecode {
        typedef union { long long i; double f; } i64;
        enum Opcode {
            Nop = 0,
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
        };
        static constexpr const unsigned char DO_NOTHING_BYTECODE[] = {Opcode::Return, 0, Opcode::End};
        static constexpr const size_t STACK_SIZE = 64;
        static constexpr const size_t MAX_CYCLES = 1024;
        static constexpr const size_t MAX_VARS = 32;
        static constexpr const i64 i64Zero = {.i = 0};
        const unsigned char *bytecode;
        size_t len;
        size_t nvars = 0;
        size_t max_cycles = 0x800000;
        i64 *vars = nullptr;
        i64 *stack = nullptr;
        ScriptInterface *interface;
        public:
        enum Result {
            Success = 0,
            UnknownOpcode,
            OutOfBoundsRead,
            OutOfBoundsWrite,
            OutOfBoundsExec,
            StackOverflow,
            StackUnderflow,
            Timeout,
        };
        Result result;
        ScriptBytecode() {
            bytecode = DO_NOTHING_BYTECODE;
            len = sizeof(DO_NOTHING_BYTECODE);
        }
        ScriptBytecode(const unsigned char *bytecode, size_t len) : ScriptBytecode(bytecode, len, MAX_CYCLES) {}

        ScriptBytecode(const unsigned char *bytecode, size_t len, size_t max_cycles) {
            this->bytecode = bytecode;
            this->len = len;
            this->max_cycles = max_cycles;
            if (this->len > 0) {
                stack = new i64[STACK_SIZE];
                vars = new i64[MAX_VARS];
            } else {
                stack = vars = nullptr;
            }
        }
        void setInterface(ScriptInterface *interface) {
            this->interface = interface;
        }
        void setMaxCycles(size_t cycles) {
            max_cycles = cycles;
        }
        size_t dump(FILE *fd) {
            return fwrite(bytecode, 1, len, fd);
        }
        int run(size_t argc, long long *argv, long long *retval) {
            size_t cycles = 0;
            size_t pc = 0;
            size_t sp = STACK_SIZE;
            i64 acc, bcc;
            long long tmp, tmp2, tmp3, tmp4, tmp5;
            char tmpC;
            short tmpS;
            int tmpI;
            for (char i=0; i<8; i++) {
                retval[i] = 0;
            }
            acc.i = bcc.i = 0;
            nvars = MAX_VARS;
            result = Result::Success;
            while (pc < len) {
                if (cycles++ >= max_cycles) {
                    result = Result::Timeout;
                    break;
                }
                switch (next(pc)) {
                    case Nop:
                        break;
                    case Return:
                        tmp = next(pc);
                        if (tmp < 0 || tmp >= 8) {
                            result = Result::OutOfBoundsWrite;
                        } else {
                            tmp2 = next(pc);
                            retval[tmp] = getvar(tmp2).i;
                        }
                        break;
                    // note: these return functions need to be updated if Components::TickingFunction::TickResult changes.
                    case ReturnDoNothing:
                        retval[0] = 0;
                        break;
                    case ReturnFail:
                        retval[0] = 1;
                        break;
                    case ReturnDestroy:
                        retval[0] = 2;
                        break;
                    case ReturnPlace:
                        retval[0] = 3;
                        break;
                    case ReturnKeep:
                        retval[0] = 4;
                        break;
                    case ReturnUpdate:
                        retval[0] = 5;
                        break;
                    case ReturnReverseUpdate:
                        retval[0] = 6;
                        break;
                    case Random:
                        acc.i = rand();
                        break;
                    case End:
                        return Result::Success;
                    case Frameset:
                        tmpC = next(pc);
                        sp += tmpC;
                        break;
                    case ReadArg:
                        tmp = next(pc);
                        if (tmp < 0 || tmp >= argc) {
                            result = Result::OutOfBoundsRead;
                        } else {
                            acc.i = argv[tmp];
                        }
                        break;
                    case Immediate8:
                        tmpC = next(pc);
                        acc.i = tmpC;
                        break;
                    case Immediate16:
                        tmpS = nextw(pc);
                        acc.i = tmpS;
                        break;
                    case Immediate32:
                        tmpI = nexti(pc);
                        acc.i = tmpI;
                        break;
                    case Immediate8U:
                        acc.i = next(pc);
                        break;
                    case Immediate16U:
                        acc.i = nextw(pc);
                        break;
                    case Immediate32U:
                        acc.i = nexti(pc);
                        break;
                    case Immediate8B:
                        tmpC = next(pc);
                        bcc.i = tmpC;
                        break;
                    case Immediate16B:
                        tmpS = nextw(pc);
                        bcc.i = tmpS;
                        break;
                    case Immediate32B:
                        tmpI = nexti(pc);
                        bcc.i = tmpI;
                        break;
                    case Immediate8UB:
                        bcc.i = next(pc);
                        break;
                    case Immediate16UB:
                        bcc.i = nextw(pc);
                        break;
                    case Immediate32UB:
                        bcc.i = nexti(pc);
                        break;
                    case LoadVar:
                        tmp = next(pc);
                        bcc = getvar(tmp);
                        break;
                    case StoreVar:
                        tmp = next(pc);
                        setvar(tmp, acc);
                        break;
                    case Exchange:
                        tmp = acc.i;
                        acc.i = bcc.i;
                        bcc.i = tmp;
                        break;
                    case Add:
                        acc.i += bcc.i;
                        break;
                    case Sub:
                        acc.i -= bcc.i;
                        break;
                    case Mul:
                        acc.i *= bcc.i;
                        break;
                    case Div:
                        tmp = bcc.i;
                        if (tmp == 0) {
                            acc.i = -1;
                        } else {
                            acc.i /= tmp;
                        }
                        break;
                    case Mod:
                        tmp = bcc.i;
                        if (tmp == 0) {
                            acc.i = -1;
                        } else {
                            acc.i %= tmp;
                        }
                        break;
                    case And:
                        acc.i &= bcc.i;
                        break;
                    case Or:
                        acc.i |= bcc.i;
                        break;
                    case Xor:
                        acc.i ^= bcc.i;
                        break;
                    case Lor:
                        acc.i = acc.i || bcc.i;
                        break;
                    case Land:
                        acc.i = acc.i && bcc.i;
                        break;
                    case Inc:
                        acc.i++;
                        break;
                    case Dec:
                        acc.i--;
                        break;
                    case AddF:
                        acc.f += bcc.f;
                        break;
                    case SubF:
                        acc.f -= bcc.f;
                        break;
                    case MulF:
                        acc.f *= bcc.f;
                        break;
                    case DivF:
                        acc.f /= bcc.f;
                        break;
                    case ModF:
                        acc.f = fmod(acc.f, bcc.f);
                        break;
                    case PowF:
                        acc.f = pow(acc.f, bcc.f);
                        break;
                    case NanF:
                        acc.i = acc.f == NAN;
                        break;
                    case InfF:
                        acc.i = acc.f == INFINITY;
                        break;
                    case Push:
                        push(sp, acc);
                        break;
                    case Pop:
                        acc = pop(sp);
                        break;
                    case PushB:
                        push(sp, bcc);
                        break;
                    case PopB:
                        bcc = pop(sp);
                        break;
                    case BA:
                        pc = nextw(pc);
                        break;
                    case BZ:
                        tmp = nextw(pc);
                        if (acc.i == 0) {
                            pc = tmp;
                        }
                        break;
                    case BNZ:
                        tmp = nextw(pc);
                        if (acc.i != 0) {
                            pc = tmp;
                        }
                        break;
                    case JSR:
                        tmp = nextw(pc);
                        push(sp, {.i = (signed)pc});
                        pc = tmp;
                        break;
                    case JSRZ:
                        tmp = nextw(pc);
                        if (acc.i == 0) {
                            push(sp, {.i = (signed)pc});
                            pc = tmp;
                        }
                        break;
                    case JSRNZ:
                        tmp = nextw(pc);
                        if (acc.i != 0) {
                            push(sp, {.i = (signed)pc});
                            pc = tmp;
                        }
                        break;
                    case RTS:
                        pc = pop(sp).i;
                        break;
                    case RTSZ:
                        if (acc.i == 0) {
                            pc = pop(sp).i;
                        }
                        break;
                    case RTSNZ:
                        if (acc.i != 0) {
                            pc = pop(sp).i;
                        }
                        break;
                    case EQ:
                        acc.i = acc.i == bcc.i;
                        break;
                    case NEQ:
                        acc.i = acc.i != bcc.i;
                        break;
                    case GT:
                        acc.i = acc.i > bcc.i;
                        break;
                    case LT:
                        acc.i = acc.i < bcc.i;
                        break;
                    case GTEQ:
                        acc.i = acc.i >= bcc.i;
                        break;
                    case LTEQ:
                        acc.i = acc.i <= bcc.i;
                        break;
                    case EQF:
                        acc.i = acc.f == bcc.f;
                        break;
                    case NEQF:
                        acc.i = acc.f != bcc.f;
                        break;
                    case GTF:
                        acc.i = acc.f > bcc.f;
                        break;
                    case LTF:
                        acc.i = acc.f < bcc.f;
                        break;
                    case GTEQF:
                        acc.i = acc.f >= bcc.f;
                        break;
                    case LTEQF:
                        acc.i = acc.f <= bcc.f;
                        break;
                    case BZSet32:
                        tmpI = nextl(pc);
                        tmp = nextw(pc);
                        if (acc.i == 0) {
                            acc.i = tmpI;
                            pc = tmp;
                        }
                        break;
                    case BNZSet32:
                        tmpI = nextl(pc);
                        tmp = nextw(pc);
                        if (acc.i == 0) {
                            acc.i = tmpI;
                            pc = tmp;
                        }
                        break;
                    case PushArg:
                        tmp = next(pc);
                        if (tmp < 0 || tmp >= argc) {
                            result = Result::OutOfBoundsRead;
                        } else {
                            push(sp, i64 { .i = argv[tmp] });
                        }
                        break;
                    case PushVar:
                        tmp = next(pc);
                        push(sp, getvar(tmp));
                        break;
                    case Abs:
                        acc.i = abs(acc.i);
                        break;
                    case AbsF:
                        acc.f = fabs(acc.f);
                        break;
                    case Sqrt:
                        acc.i = sqrt(acc.i);
                        break;
                    case SqrtF:
                        acc.f = sqrt(acc.f);
                        break;
                    case Itof:
                        acc.f = acc.i;
                        break;
                    case Ftoi:
                        acc.i = acc.f;
                        break;
                    case GetBlock:
                        tmp  = pop(sp).i + argv[1];
                        tmp2 = pop(sp).i + argv[2];
                        tmp3 = pop(sp).i + argv[3];
                        acc.i = interface->getBlock(tmp, tmp2, tmp3);
                        break;
                    case SetBlock:
                        tmp  = pop(sp).i + argv[1];
                        tmp2 = pop(sp).i + argv[2];
                        tmp3 = pop(sp).i + argv[3];
                        tmp4 = pop(sp).i;
                        interface->setBlock(tmp, tmp2, tmp3, tmp4);
                        break;
                    case BreakBlock:
                        tmp  = pop(sp).i + argv[1];
                        tmp2 = pop(sp).i + argv[2];
                        tmp3 = pop(sp).i + argv[3];
                        interface->breakBlock(tmp, tmp2, tmp3);
                        break;
                    case Check3x3x3:
                        cycles += 3*3*3/4;
                        if (cycles >= max_cycles) {
                            result = Result::Timeout;
                        } else {
                            tmp  = pop(sp).i + argv[1];
                            tmp2 = pop(sp).i + argv[2];
                            tmp3 = pop(sp).i + argv[3];
                            tmp4 = pop(sp).i;
                            acc.i = interface->check3x3x3(tmp, tmp2, tmp3, tmp4);
                        }
                        break;
                    case Check3x3:
                        cycles += 3*3/4;
                        if (cycles >= max_cycles) {
                            result = Result::Timeout;
                        } else {
                            tmp  = pop(sp).i + argv[1];
                            tmp2 = pop(sp).i + argv[2];
                            tmp3 = pop(sp).i + argv[3];
                            tmp4 = pop(sp).i;
                            acc.i = interface->check3x3(tmp, tmp2, tmp3, tmp4);
                        }
                        break;
                    case Check5x5x5:
                        cycles += 5*5*5/4;
                        if (cycles >= max_cycles) {
                            result = Result::Timeout;
                        } else {
                            tmp  = pop(sp).i + argv[1];
                            tmp2 = pop(sp).i + argv[2];
                            tmp3 = pop(sp).i + argv[3];
                            tmp4 = pop(sp).i;
                            acc.i = interface->check5x5x5(tmp, tmp2, tmp3, tmp4);
                        }
                        break;
                    case Check5x5:
                        cycles += 5*5/4;
                        if (cycles >= max_cycles) {
                            result = Result::Timeout;
                        } else {
                            tmp  = pop(sp).i + argv[1];
                            tmp2 = pop(sp).i + argv[2];
                            tmp3 = pop(sp).i + argv[3];
                            tmp4 = pop(sp).i;
                            acc.i = interface->check5x5(tmp, tmp2, tmp3, tmp4);
                        }
                        break;
                    case ReplaceBlocks3x3:
                        cycles += 3*3/2;
                        if (cycles >= max_cycles) {
                            result = Result::Timeout;
                        } else {
                            tmp  = pop(sp).i + argv[1];
                            tmp2 = pop(sp).i + argv[2];
                            tmp3 = pop(sp).i + argv[3];
                            tmp4 = pop(sp).i;
                            tmp5 = pop(sp).i;
                            interface->replaceBlocks3x3(tmp, tmp2, tmp3, tmp4, tmp5);
                        }
                        break;
                    case ReplaceBlocks3x3x3:
                        cycles += 3*3*3/2;
                        if (cycles >= max_cycles) {
                            result = Result::Timeout;
                        } else {
                            tmp  = pop(sp).i + argv[1];
                            tmp2 = pop(sp).i + argv[2];
                            tmp3 = pop(sp).i + argv[3];
                            tmp4 = pop(sp).i;
                            tmp5 = pop(sp).i;
                            interface->replaceBlocks3x3x3(tmp, tmp2, tmp3, tmp4, tmp5);
                        }
                        break;
                    case ReplaceBlocks5x5:
                        cycles += 5*5/2;
                        if (cycles >= max_cycles) {
                            result = Result::Timeout;
                        } else {
                            tmp  = pop(sp).i + argv[1];
                            tmp2 = pop(sp).i + argv[2];
                            tmp3 = pop(sp).i + argv[3];
                            tmp4 = pop(sp).i;
                            tmp5 = pop(sp).i;
                            interface->replaceBlocks5x5(tmp, tmp2, tmp3, tmp4, tmp5);
                        }
                        break;
                    case ReplaceBlocks5x5x5:
                        cycles += 5*5*5/2;
                        if (cycles >= max_cycles) {
                            result = Result::Timeout;
                        } else {
                            tmp  = pop(sp).i + argv[1];
                            tmp2 = pop(sp).i + argv[2];
                            tmp3 = pop(sp).i + argv[3];
                            tmp4 = pop(sp).i;
                            tmp5 = pop(sp).i;
                            interface->replaceBlocks5x5x5(tmp, tmp2, tmp3, tmp4, tmp5);
                        }
                        break;
                    case SpreadBlocks:
                        cycles += 3*3*3;
                        if (cycles >= max_cycles) {
                            result = Result::Timeout;
                        } else {
                            tmp  = pop(sp).i + argv[1];
                            tmp2 = pop(sp).i + argv[2];
                            tmp3 = pop(sp).i + argv[3];
                            tmp4 = pop(sp).i;
                            tmp5 = pop(sp).i;
                            interface->spreadBlocks(tmp, tmp2, tmp3, tmp4, tmp5);
                        }
                        break;
                    case isSolid:
                        acc.i = interface->isSolid(acc.i) ? 1 : 0;
                        break;
                    case isOpaque:
                        acc.i = interface->isOpaque(acc.i) ? 1 : 0;
                        break;
                    case isTicking:
                        acc.i = interface->isTicking(acc.i) ? 1 : 0;
                        break;
                    default:
                        result = Result::UnknownOpcode;
                        printf("Opcode: 0x%02X\n", bytecode[pc-1]);
                        break;
                }
                if (result != Result::Success) {
                    break;
                }
            }
            if (result != Result::Success) {
                printf("Program counter: 0x%04llX\n", pc-1);
                printf("Stack pointer: 0x%04llX\n", sp);
                printf("Accumulator: 0x%016llX\n", acc.i);
            }
            return result;
        }
        private:
        i64 pop(size_t& sp) {
            if (sp == STACK_SIZE) {
                result = Result::StackUnderflow;
                return i64Zero;
            } else {
                return stack[sp++];
            }
        }
        void push(size_t& sp, i64 val) {
            if (sp == 0) {
                result = Result::StackOverflow;
            } else {
                stack[--sp] = val;
            }
        }
        i64 getvar(unsigned char n) {
            if (n == 0) {
                return i64Zero;
            }
            if (vars != nullptr && n < nvars) {
                return vars[n];
            }
            result = Result::OutOfBoundsRead;
            return i64Zero;
        }
        void setvar(unsigned char n, i64 val) {
            if (vars != nullptr && n < nvars) {
                if (n > 0) {
                    vars[n] = val;
                }
            } else {
                result = Result::OutOfBoundsRead;
            }
        }
        long long nextl(size_t &i) {
            unsigned int tmp = nexti(i);
            unsigned int tmp2 = nexti(i);
            return (tmp & 0xffffffff) | ((unsigned long long)tmp2 << 32);
        }
        int nexti(size_t &i) {
            unsigned short tmp = nextw(i);
            unsigned short tmp2 = nextw(i);
            return (tmp & 0xffff) | ((unsigned int)tmp2 << 16);
        }
        short nextw(size_t &i) {
            unsigned char tmp = next(i);
            unsigned char tmp2 = next(i);
            return (tmp & 0xff) | ((unsigned short)tmp2<<8);
        }
        unsigned char next(size_t &i) {
            if (i < len)
                return bytecode[i++];
            result = Result::OutOfBoundsRead;
            return 0;
        }
    };
}

#endif