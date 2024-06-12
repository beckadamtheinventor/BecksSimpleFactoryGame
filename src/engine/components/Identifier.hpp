#ifndef __IDENTIFIER_H__
#define __IDENTIFIER_H__

#include <malloc.h>
#include <stdio.h>
#include <string.h>

namespace Components {
    /* Stores a "namespace:name" identifier string */
    class Identifier {
        char *ns, *name, *str;
        void make(const char *ns, const char *name) {
            size_t len0 = strlen(ns)+1;
            size_t len1 = strlen(name)+1;
            char *tmp0 = (char*)malloc(len0);
            char *tmp1 = (char*)malloc(len0+len1+1);
            memcpy(tmp0, ns, len0);
            this->ns = tmp0;
            memcpy(tmp1, ns, len0);
            tmp1[len0-1] = ':';
            memcpy(&tmp1[len0], name, len1);
            this->str = tmp1;
            this->name = &tmp1[len0];
        }
        public:
    /* stores namespace in its own string,
        full name in its own string,
        and name as an offset of the full name string */

        /* Initialize an Identifier object from a partial or complete Identifier string. */
        Identifier(const char *s) {
            // printf("Initializing Identifier(\"%s\")\n", s);
            const char *i = strchr(s, ':');
            if (i != NULL) {
                size_t l = i + 1 - s;
                char *tmp = (char*)malloc(l);
                memcpy(tmp, s, l);
                tmp[l-1] = 0;
                make(tmp, &i[1]);
            } else {
                make("default", s);
            }
        }
        Identifier(const char *ns, const char *name) {
            make(ns, name);
        }
        operator const char *() {
            return str;
        }

        const char *getNamespace() {
            return ns;
        }
        const char *getName() {
            return name;
        }
        const char *getString() {
            return str;
        }
    };
}

#endif