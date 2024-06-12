#ifndef __REGION_H__
#define __REGION_H__

#include "BlockPos.hpp"
#include "ChunkColumn.hpp"
#include <stdio.h>
#include <string.h>

#define REGION_HEADER "BSFGREG\0"
#define REGION_DATA_VERSION 1

namespace Components {
    class Region {
        public:
        BlockPos pos;
        private:
        ChunkColumn *chunks[REGION_W*REGION_W] = {nullptr};
        public:
        Region() : Region(BlockPos()) {}
        Region(BlockPos pos) {
            this->pos = pos.regionPos();
        }
        ~Region() {
            for (size_t z=0; z<REGION_W; z++) {
                for (size_t x=0; x<REGION_W; x++) {
                    if (ChunkColumn *chunk = chunks[x + z*REGION_W]) {
                        delete chunk;
                    }
                }
            }
        }
        // get the chunk at pos
        Chunk *get(BlockPos pos) {
            pos = pos.chunkPos();
            ChunkColumn *col = chunks[(pos.x/CHUNK_W)+(pos.z/CHUNK_W)*REGION_W];
            if (col != nullptr) {
                return col->get(pos.y / CHUNK_W);
            }
            return nullptr;
        }
        // Set the chunk at pos. Make sure you free the old one first if it exists.
        void set(BlockPos pos, Chunk *chunk) {
            pos = pos.chunkPos();
            size_t i = (pos.x/CHUNK_W)+(pos.z/CHUNK_W)*REGION_W;
            ChunkColumn *col = chunks[i];
            if (col == nullptr) {
                col = chunks[i] = new ChunkColumn();
            }
            col->set(pos.y / CHUNK_W, chunk);
        }
        bool generate(BlockPos pos, NoiseSettings *noise) {
            BlockPos p = pos - pos.regionPos();
            size_t i = (p.x/CHUNK_W) + (p.z/CHUNK_W)*REGION_W;
            ChunkColumn *col = chunks[i];
            if (col == NULL) {
                col = chunks[i] = new ChunkColumn();
            }
            col->generate(pos.chunkPos(), noise);
            return true;
        }
        bool load(const char *fname) {
            unsigned char buffer[Z_CHUNK_SIZE];
            size_t columnsize = 0;
            FILE *fd;
            fopen_s(&fd, fname, "rb");
            fread(buffer, 1, 8, fd);
            // fail if invalid region file
            if (strcmp((char*)buffer, REGION_HEADER)) {
                fclose(fd);
                return false;
            }
            size_t versionno = 0;
            fread(&versionno, 4, 1, fd);
            // don't load if the data version is newer than the current one
            if (versionno > REGION_DATA_VERSION) {
                fclose(fd);
                return false;
            }
            while (!feof(fd)) {
                fread(buffer, 1, 4, fd);
                // return if invalid format
                if (memcmp(buffer, "CO", 2)) {
                    fclose(fd);
                    return false;
                }
                size_t x=0, z=0;
                fread(&x, 1, 1, fd);
                fread(&z, 1, 1, fd);
                fread(&columnsize, 4, 1, fd);
                size_t offset = 0;
                while (offset < columnsize) {
                    offset += fread(buffer, 1, 2, fd);
                    if (buffer[0] != 'C') {
                        fclose(fd);
                        return false;
                    }
                    size_t y = buffer[1];
                    size_t chunksize = 0;
                    offset += fread(&chunksize, 2, 1, fd);
                    offset += fread(buffer, 1, chunksize, fd);
                    Chunk *chunk = new Chunk();
                    chunk->deserialize(buffer, chunksize);
                }
            }
            return true;
        }
        size_t save(const char *fname) {
            unsigned char buffer[Z_CHUNK_SIZE];
            size_t sz = 0;
            FILE *fd;
            fopen_s(&fd, fname, "wb");
            sz += fwrite(REGION_HEADER, 1, 8, fd);
            size_t tmp = REGION_DATA_VERSION;
            sz += fwrite(&tmp, 1, 4, fd);
            for (size_t z=0; z<REGION_W; z++) {
                for (size_t x=0; x<REGION_W; x++) {
                    ChunkColumn *col = chunks[x + z * REGION_W];
                    // only write chunk columns that exist
                    if (col != nullptr) {
                        sz += fwrite("CO", 1, 2, fd);
                        sz += fwrite(&x, 1, 1, fd);
                        sz += fwrite(&z, 1, 1, fd);
                        // this will later be overwritten by the length of the chunk column data
                        size_t columnsizeoffset = sz;
                        sz += fwrite("\0\0\0\0", 1, 4, fd);
                        size_t columnsize = sz;
                        for (size_t y=0; y<CHUNK_COLUMN_HEIGHT; y++) {
                            // only write chunks that exist
                            if (col->has(y)) {
                                Chunk *chunk = col->get(y);
                                // only write non-empty chunks
                                if (!chunk->isEmpty()) {
                                    sz += fwrite("C", 1, 1, fd);
                                    sz += fwrite(&y, 1, 1, fd);
                                    size_t chunksize = chunk->serialize(buffer);
                                    sz += fwrite(&chunksize, 1, 2, fd);
                                    sz += fwrite(buffer, 1, chunksize, fd);
                                }
                            }
                        }
                        columnsize = sz - columnsize;
                        fseek(fd, columnsizeoffset, SEEK_SET);
                        fwrite(&columnsize, 1, 4, fd);
                        fseek(fd, sz, SEEK_SET);
                    }
                }
            }
            fclose(fd);
            return sz;
        }
    };
}

#endif