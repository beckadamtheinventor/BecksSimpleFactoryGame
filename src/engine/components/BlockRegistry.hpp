#ifndef __BLOCK_REGISTRY_H__
#define __BLOCK_REGISTRY_H__

#include "Block.hpp"
#include "Types.hpp"
#include "Registry.hpp"
#include "TextureRegistry.hpp"
#include "TickingFunctionRegistry.hpp"

#define BLOCK_REGISTRY_DATA_VERSION 1

extern Components::TextureRegistry GlobalTextureRegistry;
extern Components::TickingFunctionRegistry GlobalTickingFunctionRegistry;

namespace Components {
    class BlockRegistry : public Registry<Block> {
        public:
        bool Load(JSON::JSONObject& json) {
            Block* air = getById(0);
            air->flags = 0;
            air->textureAll(0);
            air->functionstr = nullptr;
            air->name = nullptr;
            air->title = nullptr;
            air->function = 0;
            air->randomtick = 0;
            
            std::string ns = "default";
            if (json.Contains("version")) {
                JSON::JSON ver = *json["version"];
                if (ver.getType() == JSON::Type::Integer) {
                    if (ver.getInteger() != BLOCK_REGISTRY_DATA_VERSION) {
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
                    JSON::JSON j = *defines.get(i);
                    if (j.getType() == JSON::Type::Object) {
                        JSON::JSONObject& obj = j.getObject();
                        if (obj.Contains("id") &&
                            obj.Contains("textures")) {
                            JSON::JSON* j_id = obj["id"];
                            JSON::JSON* j_textures = obj["textures"];
                            if (j_id->getType() == JSON::Type::String) {
                                std::string *s_id = new std::string(ns + ":" + obj["id"]->getString());
                                Identifier id = Identifier(s_id->c_str());
                                Block* block = new Block(this->getNextId());
                                if (j_textures->getType() == JSON::Type::Array) {
                                    JSON::JSONArray& textures = j_textures->getArray();
                                    for (char f=0; f<6; f++) {
                                        if (textures[f]->getType() == JSON::Type::String) {
                                            TextureId* tex = GlobalTextureRegistry.get(textures[f]->getCString());
                                            block->texture(f, tex->id);
                                            // printf("texture id %u\n", tex.id);
                                        } else {
                                            block->texture(f, 1);
                                        }
                                    }
                                } else if (j_textures->getType() == JSON::Type::String) {
                                    TextureId* tex = GlobalTextureRegistry.get(j_textures->getCString());
                                    block->textureAll(tex->id);
                                } else {
                                    printf("Warning: missing textures for block %s (ID %u)\n", id.getName(), block->getId());
                                }
                                block->flags = 0;
                                block->function = block->randomtick = 0;
                                block->isOpaque = true;
                                block->isSolid = true;
                                block->isTicking = false;
                                block->functionstr = nullptr;
                                block->name = id.getString();
                                block->title = nullptr;
                                if (j.contains("opaque") && j["opaque"]->getType() == JSON::Type::Boolean) {
                                    block->isOpaque = j["opaque"]->getBoolean();
                                }
                                if (j.contains("solid") && j["solid"]->getType() == JSON::Type::Boolean) {
                                    block->isSolid = j["solid"]->getBoolean();
                                }
                                if (j.contains("tickfunction") && j["tickfunction"]->getType() == JSON::Type::String) {
                                    block->functionstr = JSON::dupcstr(j["tickfunction"]->getString());
                                    if (j.contains("randomtick") && j["randomtick"]->getType() == JSON::Type::Integer) {
                                        block->randomtick = j["randomtick"]->getInteger();
                                    }
                                }
                                if (j.contains("title") && j["title"]->getType() == JSON::Type::String) {
                                    block->title = JSON::dupcstr(j["title"]->getString());
                                }
                                this->Register(id, block);
                                printf("Registered block %s (ID %u)\n", id.getString(), block->getId());
                                delete s_id;
                                // printf("Textures: %u, %u, %u, %u, %u, %u\n", block->getTexture(0), block->getTexture(1), block->getTexture(2), block->getTexture(3), block->getTexture(4), block->getTexture(5));
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
                        printf("BlockRegistry: error loading entry %llu\n", i);
                    }
                }
                return true;
            }
            return false;
        }
    };
}

#endif