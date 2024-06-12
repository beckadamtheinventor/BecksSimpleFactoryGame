#ifndef __CHUNK_H__
#define __CHUNK_H__

#include <cstring>
#include "../../../lib/zlib-1.3.1/zlib.h"
#include "BlockPos.hpp"
#include "Types.hpp"

/* chunk width cubed times the block ID size times 2. */
#define Z_CHUNK_SIZE (CHUNK_SZ*sizeof(blockid_t))

#define DEFLATE_LEVEL 7


namespace Components {
    class Chunk {
        enum Compression {
            NONE = 0,
            DEFLATE = 'D',
        };
        blockid_t blocks[CHUNK_W*CHUNK_W*CHUNK_W];
        int deserializeDeflate(unsigned char *data, size_t len) {
            int ret;
            unsigned int have;
            z_stream stream;
            unsigned char out[Z_CHUNK_SIZE];
            stream.zalloc = Z_NULL;
            stream.zfree = Z_NULL;
            stream.opaque = Z_NULL;
            stream.avail_in = len;
            stream.next_in = data;
            ret = inflateInit(&stream);
            if (ret != Z_OK) {
                return ret;
            }

            stream.avail_out = Z_CHUNK_SIZE;
            stream.next_out = out;
            ret = inflate(&stream, Z_NO_FLUSH);
            if (ret != Z_OK) {
                return ret;
            }
            return deserializeUncompressed(out, Z_CHUNK_SIZE - stream.avail_out);
        }
        int deserializeUncompressed(unsigned char *data, size_t len) {
            if (len > CHUNK_SZ) {
                len = CHUNK_SZ;
            }
            memcpy(blocks, data, len);
            size_t j = len / sizeof(blockid_t);
            while (j < CHUNK_SZ) {
                blocks[j++] = 0;
            }
            return CHUNK_SZ - len;
        }

        public:
        Chunk() {
            memset(blocks, 0, sizeof(blocks));
        }
        bool isEmpty() {
            for (size_t i=0; i<CHUNK_SZ; i++) {
                if (blocks[i] > 0) {
                    return false;
                }
            }
            return true;
        }
        void set(char x, char y, char z, unsigned int block=0) {
            if (x<0 || x>=CHUNK_W || y<0 || y>=CHUNK_W || z<0 || z>=CHUNK_W) {
                return;
            }
            blocks[x + z*CHUNK_W + y*CHUNK_W*CHUNK_W] = block;
        }
        unsigned int get(char x, char y, char z, unsigned int defaultId=0) {
            if (x<0 || x>=CHUNK_W || y<0 || y>=CHUNK_W || z<0 || z>=CHUNK_W) {
                return defaultId;
            }
            return blocks[x + z*CHUNK_W + y*CHUNK_W*CHUNK_W];
        }
        void deserialize(unsigned char *data, size_t len) {
            switch (data[0]) {
                case DEFLATE:
                    deserializeDeflate(&data[1], len-1);
                    break;
                default:
                    deserializeUncompressed(&data[1], len-1);
                    break;
            }
        }
        size_t serialize(unsigned char *buffer) {
            int ret;
            size_t have;
            z_stream stream;
            stream.zalloc = Z_NULL;
            stream.zfree = Z_NULL;
            stream.opaque = Z_NULL;
            ret = deflateInit(&stream, DEFLATE_LEVEL);
            if (ret != Z_OK) {
                return 0;
            }
            stream.avail_in = Z_CHUNK_SIZE;
            stream.next_in = (unsigned char*)blocks;
            stream.avail_out = Z_CHUNK_SIZE;
            stream.next_out = &buffer[1];
            ret = deflate(&stream, Z_NO_FLUSH);
            if (ret != Z_OK) {
                return 0;
            }
            have = Z_CHUNK_SIZE - stream.avail_out;
            deflateEnd(&stream);
            if (have >= Z_CHUNK_SIZE) {
                // write uncompressed data if compression would be equal size or larger
                // should be unlikely in practice
                buffer[0] = NONE;
                memcpy(&buffer[1], blocks, Z_CHUNK_SIZE);
            } else {
                // mark data as DEFLATE compressed
                buffer[0] = DEFLATE;
            }
            return have;
        }
    };
}

#endif