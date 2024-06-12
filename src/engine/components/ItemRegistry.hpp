#ifndef __ITEM_REGISTRY_H__
#define __ITEM_REGISTRY_H__

#include "Registry.hpp"
#include "TextureRegistry.hpp"
#include "Item.hpp"

#define ITEM_REGISTRY_DATA_VERSION 1

extern Components::TextureRegistry GlobalTextureRegistry;

namespace Components {
    class ItemRegistry : public Registry<Item> {
        public:
        bool Load(JSON::JSONObject& json) {
            std::string ns = "default";
            if (json.Contains("version")) {
                JSON::JSON ver = *json["version"];
                if (ver.getType() == JSON::Type::Integer) {
                    if (ver.getInteger() != ITEM_REGISTRY_DATA_VERSION) {
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
                                Item* item = new Item(this->getNextId());
                                if (obj.Contains("texture") && obj["texture"]->getType() == JSON::Type::String) {
                                    TextureId* tex = GlobalTextureRegistry.get(obj["texture"]->getCString());
                                    if (tex->path != NULL) {
                                        item->texture(tex->id);
                                    }
                                } else {
                                    printf("Warning: missing textures for item %s (ID %u)\n", id.getName(), item->getId());
                                }
                                this->Register(id, item);
                                printf("Registered item %s (ID %u)\n", id.getName(), item->getId());
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
                        printf("ItemRegistry: error loading entry %llu\n", i);
                    }
                }
                return true;
            }
            return false;
        }
    };
}

#endif