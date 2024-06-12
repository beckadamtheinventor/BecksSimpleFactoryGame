#ifndef __REGISTRY_H__
#define __REGISTRY_H__

#include "../../json/json.hpp"
#include "Identifier.hpp"

namespace Components {
    template<class T>
    class Registry {
        protected:
        JSON::HashedSymList<T> items;
        size_t next_id;
        public:
        Registry() {
            InitFirstItem();
        }
        /* Override this method for subclasses */
        bool Load(JSON::JSONObject json) {
            return false;
        }
        void InitFirstItem() {
            next_id = 1;
            T *t = new T();
            items.Add(Identifier("null"), t);
        }
        void clear() {
            items.Clear();
            InitFirstItem();
        }
        size_t getNextId() {
            return next_id++;
        }
        size_t getLastId() {
            return next_id-1;
        }
        bool Register(Identifier id, T* item) {
            if (items.Contains(id)) {
                return false;
            }
            items.Add(id, item);
            return true;
        }
        Identifier* getIdentifier(size_t i) {
            return new Identifier(items.Keys(i));
        }
        T* get(const char *id) {
            return get(Identifier(id));
        }
        T* get(Identifier id) {
            if (!items.Contains(id)) {
                return items.Values(0);
            }
            return items[id];
        }
        T* getById(size_t id) {
            if (id >= next_id) {
                return items.Values(0);
            }
            return items[id];
        }
    };
}

#endif