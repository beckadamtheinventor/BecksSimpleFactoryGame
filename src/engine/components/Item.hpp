#ifndef __ITEM_H__
#define __ITEM_H__

#include "Types.hpp"

namespace Components {
    class Item {
        itemid_t id;
        textureid_t tex;
        public:
        Item() : Item(0) {}
        Item(itemid_t id) {
            this->id = id;
            this->tex = 0;
        }
        itemid_t getId() {
            return id;
        }
        void texture(textureid_t tex) {
            this->tex = tex;
        }
    };
}

#endif