#ifndef __ITEM_STACK_H__
#define __ITEM_STACK_H__

#include "Item.hpp"
#include "ItemComponent.hpp"
#include "../../json/json.hpp"

namespace Components {
    class ItemStack {
        Item item;
        itemcount_t count;
        itemdamage_t damage;
        unsigned short num_components;
        JSON::HashedSymList<ItemComponent> *components;
        public:
        ItemStack(Item item) : ItemStack(item, 1) {}
        ItemStack(Item item, itemcount_t count) {
            this->item = item;
            this->count = count;
        }
        itemid_t getId() {
            return item.getId();
        }
        itemcount_t getCount() {
            return count;
        }
        void setCount(itemcount_t count) {
            this->count = count;
        }
    };
}

#endif