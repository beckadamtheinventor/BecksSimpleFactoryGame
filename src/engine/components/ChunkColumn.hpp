#ifndef __CHUNK_COLUMN_H__
#define __CHUNK_COLUMN_H__

#include "Chunk.hpp"
#include "BlockRegistry.hpp"
#include "GeneratorRegistry.hpp"
#include "NoiseSettings.hpp"

#include <stdio.h>
#include <string>

#define CHUNK_COLUMN_HEIGHT 8
#define WORLD_HEIGHT (CHUNK_COLUMN_HEIGHT*CHUNK_W)
#define WORLD_BOTTOM 0

#define GROUND_LEVEL 64

extern Components::BlockRegistry GlobalBlockRegistry;
extern Components::GeneratorFunctionRegistry GlobalGeneratorRegistry;

namespace Components {

    class ChunkColumn {
        Chunk *chunks[CHUNK_COLUMN_HEIGHT];
        public:
        ChunkColumn() {
            for (size_t i = 0; i < CHUNK_COLUMN_HEIGHT; i++) {
                chunks[i] = NULL;
            }
        }
        ~ChunkColumn() {
            for (size_t i = 0; i < CHUNK_COLUMN_HEIGHT; i++) {
                if (chunks[i] != nullptr) {
                    delete chunks[i];
                }
            }
        }
        bool has(int height) {
            if (height >= 0 && height < CHUNK_COLUMN_HEIGHT) {
                return chunks[height] != NULL;
            }
            return false;
        }
        Chunk *get(int height) {
            if (height >= 0 && height < CHUNK_COLUMN_HEIGHT) {
                return chunks[height];
            }
            return NULL;
        }
        void set(int height, Chunk *chunk) {
            if (height >= 0 && height < CHUNK_COLUMN_HEIGHT) {
                chunks[height] = chunk;
            }
        }
        bool generate(BlockPos pos, NoiseSettings *noise) {
            long long values[8];
            GeneratorFunction *fn = GlobalGeneratorRegistry.getById(1);
            for (size_t i=0; i<CHUNK_COLUMN_HEIGHT; i++) {
                if (chunks[i] == nullptr) {
                    chunks[i] = new Chunk();
                }
            }
            long long execskip = 0;
            for (char z=0; z<CHUNK_W; z++) {
                for (char x=0; x<CHUNK_W; x++) {
                    size_t noiseheight = noise->get(pos.x+x, 64, pos.z+z);
                    // allow for generator function to skip steps
                    if (execskip < 0) {
                        // don't re-run the generator function for the remainder of the block columns
                    } else if (execskip == 0) {
                        // run the generator function for this block column
                        if (fn->run(noiseheight, BlockPos(pos.x+x, 0, pos.z+z), values)) {
                            // if the generator function succeeded
                            execskip = values[7];
                        } else {
                            // if the generator function fails, do basic default generation.
                            values[0] = 1; // topsoil of block ID 1
                            values[1] = 2; // soil of block ID 2
                            values[2] = 4; // soil depth
                            values[3] = 3; // underground of block ID 3
                            values[4] = 4; // bedrock of block ID 4
                            values[5] = 0; // above topsoil of block ID 0
                            values[6] = 0; // above topsoil depth 0
                            execskip = -1; // don't try to re-run a broken generator function
                        }
                    } else {
                        // otherwise skip however many chunk columns
                        execskip--;
                    }
                    for (size_t y=0; y<CHUNK_COLUMN_HEIGHT; y++) {
                        Chunk *chunk = chunks[y];
                        for (size_t yy=0; yy<CHUNK_W; yy++) {
                            size_t h = y*CHUNK_W+yy;
                            if (h <= (rand() & 3)) {
                                // bedrock
                                chunk->set(x, yy, z, values[4]);
                            } else if (h > noiseheight) {
                                if (values[6] > 0 && h < noiseheight + values[6]) {
                                    chunk->set(x, yy, z, values[5]);
                                }
                            } else if (h == noiseheight) {
                                // topsoil
                                chunk->set(x, yy, z, values[0]);
                            } else if (h >= noiseheight-values[2]) {
                                // soil
                                chunk->set(x, yy, z, values[1]);
                            } else {
                                // underground
                                chunk->set(x, yy, z, values[3]);
                            }
                        }
                    }
                }
            }
            for (unsigned int i=1; i<=GlobalGeneratorRegistry.getLastId(); i++) {
                fn = GlobalGeneratorRegistry.getById(i);
                for (unsigned int r=0; r<fn->runs; r++) {
                    if (fn->prob == 0 || (rand() % fn->prob) == 0) {
                        fn->run(0, pos, values);
                    }
                }
            }
            return true;
        }
    };
}

#endif