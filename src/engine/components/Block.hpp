#ifndef __BLOCK_H__
#define __BLOCK_H__

#include "Types.hpp"

namespace Components {
    class Block {
        blockid_t id;
        textureid_t textures[6];
        public:
        union {
            unsigned int flags;
            struct {
                bool isOpaque : 1;
                bool isSolid : 1;
                bool isTicking : 1;
            };
        };
        unsigned int function;
        unsigned int randomtick;
        const char *functionstr;
        const char *title;
        const char *name;
        Block() : Block(0) {}
        Block(blockid_t id) {
            this->id = id;
            this->function = 0;
            this->flags = 0;
        }
        blockid_t getId() {
            return id;
        }
        textureid_t getTexture(char face) {
            return textures[face];
        }
        void texture(char face, textureid_t texture) {
            if (face >= 0 && face < 6) {
                textures[face] = texture;
            }
        }
        void textureAll(textureid_t texture) {
            for (char i=0; i<6; i++) {
                textures[i] = texture;
            }
        }
    };
}

#endif