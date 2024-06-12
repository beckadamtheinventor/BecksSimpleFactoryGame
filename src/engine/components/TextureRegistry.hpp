#ifndef __TEXTURE_MANAGER_H__
#define __TEXTURE_MANAGER_H__

#include "../../../lib/raylib/raylib.h"
#include "../../json/json.hpp"
#include "Types.hpp"
#include "Registry.hpp"

#define TEXTURE_REGISTRY_DATA_VERSION 1

namespace Components {
    const int atlasSize = 8192;
    const int atlasTileSize = 32;
    const int atlasTileWidth = atlasSize / atlasTileSize;
    const float invAtlasTileWidth = (double)atlasTileSize / (double)atlasSize;

    class TextureId {
        public:
        const char *path = NULL;
        textureid_t id = 0;
        TextureId() : TextureId(NULL, 0) {}
        TextureId(const char *path, textureid_t id) {
            this->path = path;
            this->id = id;
        }
    };
    class TextureRegistry : public Registry<TextureId> {
        textureid_t next_id = 1;
        Image atlas;
        Texture2D atlasTex;
        public:

        Texture2D getAtlas() {
            return atlasTex;
        }
        void clear() {
            items.Clear();
            InitFirstItem();
            this->next_id = 1;
        }

        bool Load(JSON::JSONObject& json) {
            std::string ns = "default";
            if (json.Contains("version")) {
                JSON::JSON ver = *json["version"];
                if (ver.getType() == JSON::Type::Integer) {
                    if (ver.getInteger() != TEXTURE_REGISTRY_DATA_VERSION) {
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
                JSON::JSONArray& textures = json["defines"]->getArray();
                for (size_t i=0; i<textures.length; i++) {
                    if (textures[i]->getType() == JSON::Type::String) {
                        const char *path = (new std::string("assets/" + ns + "/textures/" + textures[i]->getString() + ".png"))->c_str();
                        Identifier id = Identifier(ns.c_str(), textures[i]->getCString());
                        Image img = LoadImage(path);
                        if (IsImageReady(img)) {
                            stitchTexture(img, next_id);
                            UnloadImage(img);
                        } else {
                            printf("Failed to load image %s\n", path);
                        }
                        this->Register(id.getName(), new TextureId(path, next_id));
                        printf("Registered texture %s (ID %u) (%s)\n", id.getName(), next_id, path);
                        next_id++;
                    }
                }
            }
            genAtlas();
            return true;
        }
        Texture2D genAtlas() {
            return (atlasTex = LoadTextureFromImage(atlas));
        }
        bool exportAtlas() {
            if (!ExportImage(atlas, "atlas.png")) {
                printf("Warning: Failed to export atlas. Might be corrupted?\n");
                return false;
            }
            return true;
        }
        void stitchTexture(Image img, textureid_t id) {
            if (!IsImageReady(atlas)) {
                atlas = GenImageColor(atlasSize, atlasSize, {0, 0, 0, 0});
            }
            id--;
            int x = (id % atlasTileWidth) * atlasTileSize;
            int y = (id / atlasTileWidth) * atlasTileSize;
            // printf("Stitching texture ID %u to %u, %u\n", id, x, y);
            ImageDraw(&atlas, img,
                {0,0,(float)img.width,(float)img.height},
                {(float)x, (float)y, atlasTileSize, atlasTileSize},
                WHITE);
        }
    };
}
#endif