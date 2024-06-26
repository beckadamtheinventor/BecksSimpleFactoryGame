#ifndef __JSON_H__
#define __JSON_H__

#include <csignal>
#include <cstring>
#include <exception>
#include <string>

#define JSONMap HashedSymList<JSON>

namespace JSON {
    typedef double jdouble_t;
    typedef size_t jsize_t;
    typedef long long jlong_t;
    typedef int jint_t;
    static char *dupcstr(std::string s) {
        char *r = (char*) malloc(s.length()+1);
        if (r == NULL) {
            printf("JSON::dupcstr() out of memory");
            throw std::exception();
        }
        size_t i;
        for (i=0; i<s.length(); i++) {
            r[i] = s[i];
        }
        r[i] = 0;
        return r;
    }
    template<class V>
    class HashedSymList {
        constexpr static const jsize_t MIN_NUM_ENTRIES = 10;
		static jsize_t Hash(const char *s) {
            if (s == NULL) {
                return 0;
            }
            jsize_t h = 0;
            for (jsize_t i=0; s[i]; i++) {
                h = h*129 ^ ~s[i];
            }
            return h;
		}
        public:
        class Symbol {
            public:
            const char *key;
            jsize_t hash;
            V *value;
            Symbol(jsize_t hash, char *key) {
                this->key = key;
                this->hash = hash;
            }
            Symbol(char *key) : Symbol(Hash(key), key) {};
            Symbol() : Symbol(0, nullptr) {};

            Symbol& operator=(const Symbol& sym) {
                hash = sym.hash;
                key = sym.key;
                value = sym.value;
                return *this;
            }
            V* operator=(const V* val) {
                value = val;
            }

            operator V() const {return value;}
        };

        private:
        Symbol *entries = nullptr;
        jsize_t max_entries;
        jsize_t length = 0;

        public:

        HashedSymList() : HashedSymList(MIN_NUM_ENTRIES) {}
        HashedSymList(jsize_t max) {
            max_entries = max;
            if (max > 0) {
                entries = new Symbol[max];
            } else {
                entries = nullptr;
            }
        }
        HashedSymList(HashedSymList *v) : HashedSymList(v->max_entries) {
            length = v->length;
            for (jsize_t i = 0; i < max_entries; i++) {
                entries[i] = v->entries[i];
            }
        }

        void Clear() {
            delete entries;
            entries = new Symbol[MIN_NUM_ENTRIES] {nullptr};
            length = 0;
            max_entries = MIN_NUM_ENTRIES;
        }

        void Resize(jsize_t size) {
            if (size < max_entries) {
                length = -1;
            }
            Symbol *newmembers = new Symbol[size];
            jsize_t j = 0;
            for (jsize_t i = 0; i < max_entries; i++) {
                if (entries[i].key != nullptr) {
                    newmembers[j++] = entries[i];
                    if (j >= size) {
                        break;
                    }
                }
            }
            while (j < size) {
                newmembers[j++].key = nullptr;
            }
            max_entries = size;
            delete entries;
            entries = newmembers;
            Length();
        }

        V* Add(const char *key, V *value) {
            for (jsize_t i = 0; i < max_entries; i++) {
                if (entries[i].key == nullptr) {
                    entries[i].hash = Hash(key);
                    entries[i].key = key;
                    entries[i].value = value;
                    length++;
                    return entries[i].value;
                }
            }
            Resize(max_entries + MIN_NUM_ENTRIES);
            jsize_t j = FindFirstEmptyIndex();
            entries[j].hash = Hash(key);
            entries[j].key = key;
            entries[j].value = value;
            length++;
            return entries[j].value;
        }

        void Remove(const char *key) {
            Symbol& sym = FindSym(key);
            sym.name = nullptr;
            length--;
        }

        jlong_t FindSymIndex(const char *key) {
            jsize_t hash = Hash(key);
            for (jsize_t i = 0; i < max_entries; i++) {
                if (entries[i].hash == hash) {
                    if (entries[i].key != nullptr) {
                        if (!strcmp(entries[i].key, key)) {
                            return i;
                        }
                    }
                }
            }
            return -1;
        }

        bool Contains(const char *key) {
            return (FindSymIndex(key) != -1);
        }

        jsize_t Length() {
            jsize_t l = 0;
            for (jsize_t i = 0; i < max_entries; i++) {
                if (entries[i].key != nullptr) {
                    l++;
                }
            }
            length = l;
            return l;
        }

        jsize_t FindFirstEmptyIndex() {
            for (jsize_t i = 0; i < max_entries; i++) {
                if (entries[i].key == nullptr) {
                    return i;
                }
            }
            jsize_t n = max_entries;
            Resize(max_entries + MIN_NUM_ENTRIES);
            return n;
        }

        Symbol& FindSym(jsize_t i) {
            for (jsize_t j = 0; j < max_entries; j++) {
                if (entries[j].key != nullptr) {
                    if (i == 0) {
                        return entries[j];
                    }
                    i--;
                }
            }
            return entries[FindFirstEmptyIndex()];
        }

        Symbol& FindSym(const char *key) {
            jlong_t i = FindSymIndex(key);
            if (i == -1) {
                throw std::exception();
            }
            return entries[i];
        }

        const char *Keys(jsize_t i) {
            return FindSym(i).key;
        }

        V& Get(jsize_t i) {
            return FindSym(i).value;
        }

        V* Values(jsize_t i) {
            return FindSym(i).value;
        }

		V* Get(const char *key) {
			return FindSym(key).value;
		}
		
        Symbol& NextSym(const char *key) {
            jlong_t i = FindSymIndex(key);
            if (i == -1) {
                return -1;
            }
            return NextSym(i);
        }

        Symbol& NextSym(jsize_t i) {
            if (i+1 >= max_entries) {
                return new Symbol;
            }
            return entries[i+1];
        }

        V* operator[](const char *key) {
            return Get(key);
        }

        V* operator[](jsize_t i) {
            return Values(i);
        }
    };
    enum Type {
        Empty = 0,
        Object,
        Array,
        Null,
        Boolean,
        Integer,
        Float,
        String,
    };
    class JSON {

        public:
        class JSONArray {
            static const jsize_t MIN_ALLOC = 10;
            public:
            jsize_t length;
            jsize_t allocated;
            JSON **members;
            JSONArray() : JSONArray(MIN_ALLOC) {}
            JSONArray(jsize_t size) {
                allocated = size;
                length = 0;
                members = new JSON*[size]();
            }
            JSONArray(JSON **members, jsize_t size) {
                this->allocated = size;
                this->length = size;
                this->members = members;
            }
            JSONArray(JSONArray& a) : JSONArray(a.length) {
                for (jsize_t i=0; i<a.length; i++) {
                    members[i] = a.members[i];
                }
            }
            ~JSONArray() {
                if (members != NULL && allocated > 0) {
                    delete members;
                }
            }
            jsize_t trim() {
                resize(length);
                return length;
            }
            void resize(jsize_t size) {
                if (size < length) {
                    size = length;
                }
                JSON **newmembers = new JSON*[size]();
                for (jsize_t i=0; i<length; i++) {
                    newmembers[i] = members[i];
                }
                delete [] members;
                members = newmembers;
                allocated = size;
            }
            JSON* append(JSON *object) {
                if (length + 1 >= allocated) {
                    resize(allocated + MIN_ALLOC);
                }
                members[length] = object;
				return members[length++];
            }
            void remove(jsize_t i) {
                if (i < length) {
                    if (i == length - 1) {
                        members[length] = new JSON();
                    } else for (; i<length-1; i++)
                        members[i] = members[i+1];
                    length--;
                }
            }
            JSON* get(jsize_t i) {
                if (i >= allocated) {
                    resize(i + MIN_ALLOC);
                }
                if (i >= length) {
                    length = i+1;
                }
                return members[i];
            }
            JSON* operator[](jsize_t i) {
                return get(i);
            }
            JSON* operator+=(JSON *object) {
                return append(object);
            }
        };
        private:
        jsize_t type = Type::Empty;
        union {
            const char *s;
            jdouble_t d;
            jlong_t i;
            JSONArray *a;
            JSONMap *o;
            void *p;
        } value;
        void type_error() {
            printf("Wrong type (0x%llX) for operation\n", type);
            throw std::exception();
        }
        public:
        JSON() {}
        JSON(JSONMap *o) {
            this->type = Type::Object;
            this->value.o = o;
        }
        JSON(JSONArray *a) {
            this->type = Type::Array;
            this->value.a = a;
        }
        JSON(jlong_t i) {
            this->type = Type::Integer;
            this->value.i = i;
        }
        JSON(jint_t i) {
            this->type = Type::Integer;
            this->value.i = i;
        }
        JSON(jdouble_t d) {
            this->type = Type::Float;
            this->value.d = d;
        }
        JSON(char *s) {
            this->type = Type::String;
            this->value.s = s;
        }
        JSON(std::string s) {
            this->type = Type::String;
            this->value.s = (char*) s.c_str();
        }

        JSONMap *set(JSONMap& o) { return setObject(o); }
        JSONMap *set(JSONMap* o) { return setObject(o); }
        JSONArray *set(JSONArray& a) { return setArray(a); }
        JSONArray *set(JSONArray *a) { return setArray(a); }
        jdouble_t set(jdouble_t v) { return setFloat(v); }
        jlong_t set(jlong_t v) { return setInteger(v); }
        jint_t set(jint_t v) { return setInteger(v); }
        const char *set(const char *s) { return setString(s); }
        std::string set(std::string s) { return setString(s); }
        bool set(bool v) { return setBoolean(v); }
        void set() { return setNull(); }
        void *set(void *p, jsize_t t) { return setCustom(p, t); }

        static JSON *fromDouble(double value) {
            JSON *j = new JSON();
            j->setFloat((jdouble_t)value);
            return j;
        }

        Type getType() {
            return static_cast<Type>(type);
        }

        jsize_t getCustomType() {
            return type;
        }

        void setCustomType(jsize_t t) {
            this->type = t;
        }

        void *getCustomValue() {
            return this->value.p;
        }

        void setCustomValue(void *p) {
            this->value.p = p;
        }

        JSON* operator[](jsize_t i) {
            if (type == Type::Array) {
                return value.a->get(i);
            }
            value.a->append(new JSON());
            return value.a->get(value.a->length);
        }
        JSON* operator[](std::string key) {
            const char *s = key.c_str();
            if (type == Type::Object) {
                if (value.o->Contains(s)) {
                    return value.o->FindSym(s).value;
                }
                return value.o->Add(key.c_str(), new JSON());
            }
            printf("Cannot index non-object with key string\n");
            throw std::exception();
        }
        JSON* operator[](char *key) {
            if (type == Type::Object) {
                if (value.o->Contains(key)) {
                    return value.o->FindSym(key).value;
                }
                return value.o->Add(key, new JSON());
            }
            printf("Cannot index non-object with key string\n");
            throw std::exception();
        }
        bool contains(std::string key) {
            if (type == Type::Object) {
                return value.o->Contains(key.c_str());
            }
            return false;
        }
        bool contains(const char *key) {
            if (type == Type::Object) {
                return value.o->Contains(key);
            }
            return false;
        }
        const char* getCString() {
            if (type != Type::String) {
                type_error();
            }
            return value.s;
        }
        std::string getString() {
            if (type != Type::String) {
                type_error();
            }
            return std::string(value.s);
        }
        jdouble_t& getFloat() {
            if (type != Type::Float) {
                type_error();
            }
            return value.d;
        }
        jlong_t& getInteger() {
            if (type != Type::Integer) {
                type_error();
            }
            return value.i;
        }
        jsize_t getUnsigned() {
            if (type != Type::Integer) {
                type_error();
            }
            return value.i;
        }
        bool getBoolean() {
            if (type != Type::Boolean) {
                type_error();
            }
            return value.i > 0;
        }
        bool isNull() {
            return type == Type::Null;
        }
        jsize_t getArrayLength() {
            if (type != Type::Array) {
                type_error();
            }
            return value.a->length;
        }
        jsize_t getObjectLength() {
            if (type != Type::Object) {
                type_error();
            }
            return value.o->Length();
        }
        JSONMap& getObject() {
            if (type != Type::Object) {
                type_error();
            }
            return *value.o;
        }
        JSONArray& getArray() {
            if (type != Type::Array) {
                type_error();
            }
            return *value.a;
        }

        JSONMap *setObject(JSONMap& o) {
            this->type = Type::Object;
            this->value.o = new JSONMap(o);
			return this->value.o;
        }
        JSONMap *setObject(JSONMap *o) {
            this->type = Type::Object;
            this->value.o = o;
			return this->value.o;
        }
        JSONArray *setArray(JSONArray& a) {
            this->type = Type::Array;
            this->value.a = new JSONArray(a);
			return this->value.a;
        }
        JSONArray *setArray(JSONArray *a) {
            this->type = Type::Array;
            this->value.a = a;
			return this->value.a;
        }
        jdouble_t setFloat(jdouble_t d) {
            this->type = Type::Float;
            this->value.d = d;
			return d;
        }
        jlong_t setInteger(jlong_t i) {
            this->type = Type::Integer;
            this->value.i = i;
			return i;
        }
        const char *setString(const char *s) {
            this->type = Type::String;
            this->value.s = s;
			return s;
        }
        std::string setString(std::string s) {
            this->type = Type::String;
            this->value.s = (char*)s.c_str();
			return s;
        }
        bool setBoolean(bool v) {
            this->type = Type::Boolean;
            this->value.i = v ? 1 : 0;
			return v;
        }
        void setNull() {
            this->type = Type::Null;
            this->value.i = 0;
        }
        void *setCustom(void *p, jsize_t t) {
            this->type = t;
            this->value.p = p;
            return p;
        }

        const char *serialize() {
            jsize_t len;
            std::string o = std::string();
            switch (type) {
                case Type::Empty:
                    break;
                case Type::Null:
                    o = "null";
                    break;
                case Type::Boolean:
                    o = value.i?"true":"false";
                    break;
                case Type::Integer:
                    o = std::to_string(value.i);
                    break;
                case Type::Float:
                    o = std::to_string(value.d);
                    break;
                case Type::String:
                    if (value.s != NULL) {
                        o.reserve(strlen(value.s));
                        o.append("\"");
                        o.append(value.s);
                        o.append("\"");
                    } else {
                        o.append("\"\"");
                    }
                    break;
                case Type::Array:
                    o.reserve(value.a->length*4);
                    o.append("[");
                    for (jsize_t i=0; i<value.a->length; i++) {
                        o.append(value.a->get(i)->serialize());
                        if (i+1<value.a->length) {
                            o.append(",");
                            if (value.a->get(i)->getType() == Type::Array || value.a->get(i)->getType() == Type::Object) {
                                o.append("\n");
                            }
                        }
                    }
                    o.append("]");
                    break;
                case Type::Object:
                    len = value.o->Length();
                    o.reserve(len*6);
                    o.append("{");
                    for (jsize_t i=0; i<len; i++) {
                        o.append("\"");
                        o.append(value.o->Keys(i));
                        o.append("\": ");
                        o.append(value.o->Values(i)->serialize());
                        if (i+1 < len) {
                            o.append(",\n");
                        }
                    }
                    o.append("}");
                    break;
                default:
                    printf("Cannot serialize custom type with default method.\n\
                        Override the serialize() method for your custom type class if you need it.");
                    throw std::exception();
                    break;
            }
            o.shrink_to_fit();
            return dupcstr(o);
        }

        static JSON *deserialize(const char *data) {
            jsize_t i = 0;
            return deserialize(data, i);
        }

        private:
        static char nibble(char c) {
            if (c >= '0' && c <= '9') {
                return c - '0';
            } else if (c >= 'A' && c <= 'F') {
                return c + 10 - 'A';
            } else if (c >= 'a' && c <= 'f') {
                return c + 10 - 'a';
            } else {
                return 0;
            }
        }
        static void skipspace(const char *data, jsize_t &i) {
            while (data[i] == ' ' || data[i] == '\t' || data[i] == '\n' || data[i] == ',') {
                i++;
            }
        }
        static char nextchar(const char *data, jsize_t &i) {
            skipspace(data, i);
            char c = data[i++];
            skipspace(data, i);
            return c;
        }

        static JSON* deserialize(const char* data, jsize_t& i) {
            JSON* o = new JSON();
            skipspace(data, i);
            char c = data[i];
            if (c >= 'a' && c <= 'z') {
                if (!strncmp(&data[i], "null", 4)) {
                    o->setNull();
                    i += 4;
                } else if (!strncmp(&data[i], "false", 5)) {
                    o->setBoolean(false);
                    i += 5;
                } else if (!strncmp(&data[i], "true", 4)) {
                    o->setBoolean(false);
                    i += 4;
                } else {
                    o->setNull();
                }
            } else if (c >= '0' && c <= '9' || c == '-') {
                jsize_t j;
                bool flt = false;
                bool neg = false;
                bool hex = false;
                if (c == '-') {
                    neg = true;
                    c = data[++i];
                }
                if (c == '0' && data[i+1] == 'x') {
                    hex = true;
                    i+=2;
                }
                j = i;
                while (c != ',' && c != ']' && c != '}') {
                    c = data[j++];
                    if (c == '.') {
                        flt = true;
                    }
                }
                if (hex) {
                    jlong_t num = 0;
                    for (; i<j; j++) {
                        num = (num<<4) + nibble(data[i]);
                    }
                    if (neg) {
                        num = -num;
                    }
                    o->setInteger(num);
                } else if (flt) {
                    jdouble_t num = 0;
                    jdouble_t f = 1.0;
                    flt = false;
                    for (; i<j; i++) {
                        if (data[i] >= '0' && data[i] <= '9') {
                            if (flt) {
                                f *= 0.1;
                                num += f*(data[i] - '0');
                            } else {
                                num = num * 10 + data[i] - '0';
                            }
                        } else if (data[i] == '.') {
                            flt = true;
                        }
                    }
                    if (neg) {
                        num = -num;
                    }
                    o->setFloat(num);
                } else {
                    jlong_t num = 0;
                    for (; i<j; i++) {
                        if (data[i] >= '0' && data[i] <= '9') {
                            num = num * 10 + data[i] - '0';
                        }
                    }
                    if (neg) {
                        num = -num;
                    }
                    o->setInteger(num);
                }
                i--;
            } else if (c == '"') {
                std::string s;
                i++;
                do {
                    c = data[i++];
                    if (c == '\\') {
                        if (data[i] == 'n') {
                            s += '\n';
                        } else if (data[i] == 't') {
                            s += '\t';
                        } else if (data[i] == 'r') {
                            s += '\r';
                        } else if (data[i] == '0') {
                            s += '\0';
                        } else if (data[i] == 'x') {
                            i++;
                            s += (nibble(data[i]) << 4) | nibble(data[i+1]);
                            i += 2;
                        } else {
                            s += c;
                            s += data[i++];
                        }
                    } else if (c != '"') {
                        s += c;
                    }
                } while (c != '"');
                o->setString(dupcstr(s));
            } else if (c == '[') {
                JSONArray* a = new JSONArray();
                i++;
                skipspace(data, i);
                c = data[i];
                while (c != ']') {
                    skipspace(data, i);
                    JSON* j = deserialize(data, i);
                    a->append(j);
                    skipspace(data, i);
                    c = data[i];
                }
                i++;
                o->setArray(a);
            } else if (c == '{') {
                JSONMap* a = new JSONMap();
                i++;
                skipspace(data, i);
                c = data[i];
                while (c != '}') {
                    skipspace(data, i);
                    const char* key = deserialize(data, i)->getCString();
                    if (data[i] == ':') {
                        i++;
                    }
                    skipspace(data, i);
                    if (data[i] == ':') {
                        i++;
                    }
                    a->Add(dupcstr(std::string(key)), deserialize(data, i));
                    skipspace(data, i);
                    c = data[i];
                }
                i++;
                o->setObject(a);
            }
            return o;
        }

    };
	typedef JSON::JSON::JSONArray JSONArray;
	typedef HashedSymList<JSON> JSONObject;

    static JSON *deserialize(const char *s) {
        return JSON::deserialize(s);
    }

    static JSON *deserialize(std::string s) {
        return JSON::deserialize(s.c_str());
    }

}


#endif