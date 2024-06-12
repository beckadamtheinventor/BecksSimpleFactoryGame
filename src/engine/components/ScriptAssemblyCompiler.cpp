#include "../../json/json.hpp"
#include "DynamicArray.hpp"
#include "BlockRegistry.hpp"
#include "ItemRegistry.hpp"
#include <string.h>

extern Components::BlockRegistry GlobalBlockRegistry;
extern Components::ItemRegistry GlobalItemRegistry;

namespace Components {
        static constexpr const char *opcodes[] {
            "nop", "rv", "returnDoNothing", "returnFail", "returnDestroy",
            "returnPlace", "returnKeep", "returnUpdate", "returnReverseUpdate",
            "end", "frameset", "arg", "random", "v", "sv", "ex",
            "i8", "i16", "i32", "u8", "u16", "u32",
            "i8b", "i16b", "i32b", "u8b", "u16b", "u32b",
            "add", "sub", "mul", "div", "mod", "and", "or", "xor", "lor", "land", "inc", "dec",
            "addf", "subf", "mulf", "divf", "modf", "powf", "nanf", "inff",
            "push", "pop", "pushb", "popb", "ba", "bz", "bnz", "jsr", "rts", "jsrz", "jsrnz", "rtsz", "rtsnz",
            "eq", "neq", "gt", "lt", "gteq", "lteq", "eqf", "neqf", "gtf", "ltf", "gteqf", "lteqf",
            "bzset32", "bnzset32", "pusharg", "pushvar", "abs", "absf", "sqrt", "sqrtf", "itof", "ftoi",
            nullptr,
        };
        static constexpr const char *opcodes80[] {
            "getBlock", "setBlock", "breakBlock",
            "check3x3x3", "check3x3", "check5x5x5", "check5x5",
            "isSolid", "isOpaque", "isTicking",
            "replaceBlocks3x3x3", "replaceBlocks3x3",
            "replaceBlocks5x5x5", "replaceBlocks5x5",
            "spreadBlocks",
            nullptr,
        };

        static const char *subcstr(const char *data, size_t datalen, size_t start, size_t len) {
            if (start >= datalen) {
                return nullptr;
            }
            if (start+len > datalen) {
                len = datalen - start;
            }
            char *s = new char[len+1];
            memcpy(s, &data[start], len);
            s[len] = 0;
            return s;
        }

        ScriptAssemblyCompiler::ScriptAssemblyCompiler() {
            lno = 1;
        }

        size_t ScriptAssemblyCompiler::compile(const char *data, size_t datalen, unsigned char **out) {
            Token tk;
            size_t inoffset = 0;
            do {
                tk = next(data, datalen, inoffset);
                if (tk >= Nop && tk < None) {
                    outbuf.append(tk);
                    switch (tk) {
                        case LoadVar:
                        case StoreVar:
                        case Frameset:
                        case ReadArg:
                        case PushArg:
                        case PushVar:
                            tk = next(data, datalen, inoffset);
                            outbuf.append(token_int);
                            break;
                        case Return:
                            tk = next(data, datalen, inoffset);
                            outbuf.append(token_int);
                            tk = next(data, datalen, inoffset);
                            outbuf.append(token_int);
                            break;
                        case ReturnDoNothing:
                            break;
                        case Immediate8:
                        case Immediate8U:
                        case Immediate8B:
                        case Immediate8UB:
                            tk = next(data, datalen, inoffset);
                            outbuf.append(token_int);
                            break;
                        case Immediate16:
                        case Immediate16U:
                        case Immediate16B:
                        case Immediate16UB:
                            tk = next(data, datalen, inoffset);
                            outbuf.append(token_int);
                            outbuf.append(token_int >> 8);
                            break;
                        case Immediate32:
                        case Immediate32U:
                        case Immediate32B:
                        case Immediate32UB:
                            tk = next(data, datalen, inoffset);
                            outbuf.append(token_int);
                            outbuf.append(token_int >> 8);
                            outbuf.append(token_int >> 16);
                            outbuf.append(token_int >> 24);
                            break;
                        case BA:
                        case BZ:
                        case BNZ:
                        case JSR:
                        case JSRZ:
                        case JSRNZ:
                            tk = next(data, datalen, inoffset);
                            if (tk == LabelUsage) {
                                outbuf.append(0);
                                outbuf.append(0);
                            } else {
                                outbuf.append(token_int);
                                outbuf.append(token_int >> 8);
                            }
                            break;
                        case BZSet32:
                        case BNZSet32:
                            tk = next(data, datalen, inoffset);
                            outbuf.append(token_int);
                            outbuf.append(token_int >> 8);
                            outbuf.append(token_int >> 16);
                            outbuf.append(token_int >> 24);
                            tk = next(data, datalen, inoffset);
                            if (tk == LabelUsage) {
                                outbuf.append(0);
                                outbuf.append(0);
                            } else {
                                outbuf.append(token_int);
                                outbuf.append(token_int >> 8);
                            }
                        default:
                            break;
                    }
                }
            } while (inoffset < datalen);

            size_t *vno = new size_t[1] {outbuf.Length()};
            labels.Add("eof", vno);

            for (size_t i=0; i<labelusages.Length(); i++) {
                labelusage_t *lbl = labelusages[i];
                const char *name = lbl->label;
                size_t value = -1;
                if (labels.Contains(name)) {
                    // resolved label address
                    value = *labels[lbl->label];
                } else {
                    // label not found / not resolved
                    printf("Warning: error loading script: Unknown label name \"%s\" (line %llu)\n", name, lbl->lno);
                }
                // set resolved label address
                *outbuf[lbl->offset+0] = value;
                *outbuf[lbl->offset+1] = value >> 8;
            }

            outbuf.append(End);
            *out = outbuf.collapse();
            return outbuf.Length();
        }
    
    char ScriptAssemblyCompiler::peek(const char *data, size_t datalen, size_t i) {
        if (i < datalen) {
            return data[i];
        }
        return 0;
    }

    ScriptAssemblyCompiler::Token ScriptAssemblyCompiler::next(const char *data, size_t datalen, size_t &i) {
        char c = peek(data, datalen, i);
        Token tk = None;
        if (c == 0) {
            return tk;
        }
        // skip whitespace
        while (c > 0 && c <= ' ') {
            if (c == '\n') {
                lno++;
            }
            i++;
            c = peek(data, datalen, i);
        }
        if (c == 0) {
            return tk;
        }
        if (c == ';') {
            // comment
            i++;
            do {
                c = peek(data, datalen, i); i++;
            } while (c >= ' ');
            i--;
        } else if (c == '#') {
            // block/item ID
            bool isanitem = false;
            i++;
            if (peek(data, datalen, i) == '#') {
                i++;
                isanitem = true;
            }
            size_t j = i;
            do {
                c = peek(data, datalen, i); i++;
            } while (c > ' ');
            i--;
            if (isanitem) {
                token_int = GlobalItemRegistry.get(subcstr(data, datalen, j, i-j))->getId();
            } else {
                token_int = GlobalBlockRegistry.get(subcstr(data, datalen, j, i-j))->getId();
            }
            tk = Integer;
        } else if (c == '$' || c == '-' || c >= '0' && c <= '9') {
            // number
            bool neg = false;
            char base = 10;
            long long num = 0;
            if (c == '-') {
                i++;
                c = peek(data, datalen, i);
                neg = true;
            }
            if (c == '$') {
                i++;
                base = 16;
            }
            while (1) {
                c = peek(data, datalen, i); i++;
                if (c >= '0' && c <= '9') {
                    num = num * base + c - '0';
                } else if (base > 10) {
                    if (c >= 'A' && c <= 'F') {
                        num = num * base + c + 10 - 'A';
                    } else if (c >= 'a' && c <= 'f') {
                        num = num * base + c + 10 - 'a';
                    }
                } else {
                    break;
                }
            }
            if (neg) {
                num = -num;
            }
            token_int = num;
            tk = Integer;
        } else if (c == '_') {
            // argument/variable
            i++;
            c = peek(data, datalen, i); i++;
            if (c == 'I' && peek(data, datalen, i) == 'D') {
                i++;
                token_int = 0;
            } if (c == 'X') {
                token_int = 1;
            } else if (c == 'Y') {
                token_int = 2;
            } else if (c == 'Z') {
                token_int = 3;
            } else {
                size_t j = i;
                do {
                    c = peek(data, datalen, i); i++;
                } while (c > ' ');
                const char *name = subcstr(data, datalen, j, i-j);
                if (vars.Contains(name)) {
                    token_int = *vars.Get(name);
                } else {
                    size_t *vno = new size_t[1] {vars.Length()+1};
                    vars.Add(name, vno);
                    token_int = *vno;
                }
            }
            tk = Integer;
        } else if (c == ':') {
            // label define
            i++;
            size_t j = i;
            do {
                c = peek(data, datalen, i); i++;
            } while (c > ' ');
            i--;
            const char *name = subcstr(data, datalen, j, i-j);
            if (labels.Contains(name)) {
                *labels.Get(name) = outbuf.Length();
            } else {
                size_t *vno = new size_t[1] {outbuf.Length()};
                labels.Add(name, vno);
                token_int = *vno;
            }
            tk = Label;
        } else if (c == '@') {
            // label usage
            i++;
            size_t j = i;
            do {
                c = peek(data, datalen, i); i++;
            } while (c > ' ');
            i--;
            const char *name = subcstr(data, datalen, j, i-j);
            labelusages.append({outbuf.Length(), lno, name});
            tk = LabelUsage;
        } else if (c >= 'a' && c <= 'z') {
            size_t j = i;
            do {
                c = peek(data, datalen, i); i++;
            } while (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c >= '0' && c <= '9' || c == '_');
            i--;
            const char *name = subcstr(data, datalen, j, i-j);
            token_int = -1;
            for (size_t k=0; opcodes[k]!=nullptr; k++) {
                if (!strcmp(name, opcodes[k])) {
                    token_int = k;
                    break;
                }
            }
            if (token_int == -1) {
                for (size_t k=0; opcodes80[k]!=nullptr; k++) {
                    if (!strcmp(name, opcodes80[k])) {
                        token_int = k+0x80;
                        break;
                    }
                }
            }
            if (token_int == -1) {
                printf("Warning: error loading script: Unknown opcode '%s' (line %llu)\n", name, lno);
            } else {
                tk = (Token)token_int;
            }
        } else {
            printf("Warning: error loading script: Unexpected character '%c' (line %llu)\n", c, lno);
            i++;
        }
        return tk;
    }
}
