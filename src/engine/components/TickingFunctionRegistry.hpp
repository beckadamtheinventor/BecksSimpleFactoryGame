#ifndef __TICKING_FUNCTION_REGISTRY_HPP__
#define __TICKING_FUNCTION_REGISTRY_HPP__

#include "Registry.hpp"
#include "TickingFunction.hpp"
#include <stdio.h>

#define TICKING_FUNCTION_REGISTRY_DATA_VERSION 1

namespace Components {
    class TickingFunctionRegistry : public Registry<TickingFunction> {
        static char nibble(char c) {
            if (c >= '0' && c <= '9') {
                return c - '0';
            } else if (c >= 'A' && c <= 'F') {
                return c + 10 - 'A';
            } else if (c >= 'a' && c <= 'f') {
                return c + 10 - 'a';
            }
            return 0;
        }
        public:
        bool Load(JSON::JSONObject& json, ScriptInterface *interface) {
            std::string ns = "default";
            if (json.Contains("version")) {
                JSON::JSON ver = *json["version"];
                if (ver.getType() == JSON::Type::Integer) {
                    if (ver.getInteger() != TICKING_FUNCTION_REGISTRY_DATA_VERSION) {
                        return false;
                    }
                } else {
                    return false;
                }
            }
            if (json.Contains("namespace") && json["namespace"]->getType() == JSON::Type::String) {
                ns = json["namespace"]->getString();
            }
            if (json.Contains("defines") && json["defines"]->getType() == JSON::Type::Array) {
                JSON::JSONArray& defines = json["defines"]->getArray();
                for (size_t i = 0; i < defines.length; i++) {
                    bool err = false;
                    JSON::JSON j = *defines[i];
                    if (j.getType() == JSON::Type::Object) {
                        JSON::JSONObject& obj = j.getObject();
                        if (obj.Contains("id")) {
                            JSON::JSON j_id = *obj["id"];
                            if (j_id.getType() == JSON::Type::String) {
                                Identifier id = Identifier((new std::string(ns + ":" + obj["id"]->getString()))->c_str());
                                TickingFunction *fn = new TickingFunction(this->getNextId());
                                if (obj.Contains("bytecode") && obj["bytecode"]->getType() == JSON::Type::String) {
                                    const char *s = obj["bytecode"]->getCString();
                                    size_t len = strlen(s)/2+1;
                                    unsigned char *code = new unsigned char[len];
                                    for (size_t i=0; s[i*2]>0 && s[i*2+1]>0; i++) {
                                        code[i] = (nibble(s[i*2])<<4) | nibble(s[i*2+1]);
                                    }
                                    fn->loadScript(code, len);
                                } else if (obj.Contains("source") && obj["source"]->getType() == JSON::Type::String) {
                                    std::string s = "assets/" + obj["source"]->getString();
                                    FILE *fd = fopen(s.c_str(), "rb");
                                    if (fd != NULL) {
                                        fseek(fd, 0, SEEK_END);
                                        size_t len = ftell(fd);
                                        fseek(fd, 0, SEEK_SET);
                                        char *fdata = new char[len];
                                        fread(fdata, 1, len, fd);
                                        fclose(fd);
                                        // load assembly
                                        if (!fn->loadScriptSource(fdata, len)) {
                                            printf("Failed to load script from file %s\n", s.c_str());
                                            err = true;
                                        }
                                        delete [] fdata;
                                    } else {
                                        printf("Failed to locate script file %s\n", s.c_str());
                                    }
                                }
                                this->Register(id, fn);
                                printf("Registered Ticking Function %s (ID %u)\n", id.getString(), fn->getId());
                            } else {
                                err = true;
                            }
                        } else {
                            err = true;
                        }
                    } else {
                        err = true;
                    }
                    if (err) {
                        printf("TickingFunctionRegistry: error loading entry %llu\n", i);
                    }
                }
                return true;
            }
            return false;
        }
    };
}

#endif