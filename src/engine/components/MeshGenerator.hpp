#ifndef __MESH_GENERATOR_H__
#define __MESH_GENERATOR_H__

#include <stdint.h>
#include <string.h>
#include "../../../lib/raylib/raylib.h"
#include "../../../lib/raylib/rlgl.h"
#include "../../../lib/raylib/external/glad.h"
#include "Types.hpp"
#include "BlockRegistry.hpp"
#include "World.hpp"
#include "Block.hpp"

namespace Components {
    // extern BlockRegistry GlobalBlockRegistry;
    class MeshGenerator {

        public:
        MeshGenerator() {}

        // returns number of faces (triangle pairs) generated
        size_t gen(uint32_t **meshVertices, unsigned short** meshIndices, BlockPos pos, World *world, bool opaque=true) {
            static const char vertices[] = {
                // +y
                1, 1, 0,
                0, 1, 0,
                0, 1, 1,
                1, 1, 1,
                // -y
                0, 0, 1,
                0, 0, 0,
                1, 0, 0,
                1, 0, 1,
                // +x
                1, 0, 0,
                1, 1, 0,
                1, 1, 1,
                1, 0, 1,
                // -x
                0, 0, 1,
                0, 1, 1,
                0, 1, 0,
                0, 0, 0,
                // +z
                1, 0, 1,
                1, 1, 1,
                0, 1, 1,
                0, 0, 1,
                // -z
                0, 0, 0,
                0, 1, 0,
                1, 1, 0,
                1, 0, 0,
            };

            static const char vertexNumbers[] = {
                2, 0, 1, 3,
            };

            static const char dr[] {
                0, 1, 0,
                0, -1, 0,
                1, 0, 0,
                -1, 0, 0,
                0, 0, 1,
                0, 0, -1,
            };

/*
            if (_vertices != NULL) {
                RL_FREE(_vertices);
            }
            if (_indices != NULL) {
                RL_FREE(_indices);
            }
            if (_texcoords != NULL) {
                RL_FREE(_texcoords);
            }
            if (_normals != NULL) {
                RL_FREE(_normals);
            }
*/

            unsigned char *facesToBeDrawn = new unsigned char[CHUNK_W*CHUNK_W*CHUNK_W] {0};
            size_t numVisibleFaces = 0;
            size_t i = 0;
            Chunk* chunk = world->getChunk(pos);
            if (chunk != NULL) {
                for (char y=0; y<CHUNK_W; y++) {
                    for (char z=0; z<CHUNK_W; z++) {
                        for (char x=0; x<CHUNK_W; x++) {
                            blockid_t id = chunk->get(x, y, z);
                            if (id > 0) {
                                Block* block = GlobalBlockRegistry.getById(id);
                                char j=0;
                                for (char f=0; f<6; f++) {
                                    if (block->getTexture(f) != 0) {
                                        unsigned int id2 = chunk->get(x+dr[j+0], y+dr[j+1], z+dr[j+2], -1);
                                        Block *block2;
                                        if (id2 == -1) {
                                            // block2 = GlobalBlockRegistry.getById(0);
                                            block2 = world->getBlock(pos + BlockPos(x+dr[j+0], y+dr[j+1], z+dr[j+2]));
                                            id2 = block2->getId();
                                        } else {
                                            block2 = GlobalBlockRegistry.getById(id2);
                                        }
                                        if (block->isOpaque && !block2->isOpaque && opaque ||
                                            !block->isOpaque && id != id2 && !opaque) {
                                            facesToBeDrawn[i] |= 1 << f;
                                            numVisibleFaces++;
                                        }
                                    }
                                    j += 3;
                                }
                            }
                            i++;
                        }
                    }
                }
            } else {
                printf("Warning: Chunk at %lld,%lld,%lld was not found!\n", pos.x, pos.y, pos.z);
            }
            if (numVisibleFaces > 0) {

                uint32_t* vertexData = (uint32_t *)RL_MALLOC(numVisibleFaces*8*sizeof(uint32_t));
                unsigned short* indexData = (unsigned short *)RL_MALLOC(numVisibleFaces*6*sizeof(unsigned short));

                i = 0;
                size_t mi = 0;
                Chunk *chunk = world->getChunk(pos);
                if (chunk != NULL) {
                    for (char y=0; y<CHUNK_W; y++) {
                        for (char z=0; z<CHUNK_W; z++) {
                            for (char x=0; x<CHUNK_W; x++) {
                                unsigned char drawface = facesToBeDrawn[i];
                                if (drawface > 0) {
                                    Block* block = GlobalBlockRegistry.getById(chunk->get(x, y, z));
                                    for (char fi=0; fi<6; fi++) {
                                        if (drawface & (1 << fi)) {
                                            textureid_t tid = block->getTexture(fi);
                                            if (tid > 0) {
                                                size_t mo = mi*8;
                                                size_t fo = fi*3*4;
                                                tid--;
                                                // Vertices, Texcoords
                                                for (int j=0; j<4; j++) {
                                                    vertexData[mo + j*2 + 0] =
                                                        (15 - 2*(fi&6)) << 28 | // TODO: proper light levels
                                                        vertexNumbers[j] << 26 | // vertex number
                                                        (vertices[fo + j*3 + 0] + x)<<12  | // X
                                                        (vertices[fo + j*3 + 1] + y)<< 6  | // Y
                                                        (vertices[fo + j*3 + 2] + z)<< 0;   // Z
                                                    vertexData[mo + j*2 + 1] =
                                                        (tid & 0xFFFF) | // texture number
                                                        (0xFFF) << 16;   // vertex color
                                                }
                                                // Indices
                                                fo = mi * 6;
                                                mo >>= 1;
                                                indexData[fo + 0] = mo;
                                                indexData[fo + 1] = mo + 1;
                                                indexData[fo + 2] = mo + 2;
                                                indexData[fo + 3] = mo;
                                                indexData[fo + 4] = mo + 2;
                                                indexData[fo + 5] = mo + 3;
                                                mi++;
                                                // printf("Face %u\n", f);
                                                // for (char i=0; i<6; i++) {
                                                //     printf("%f, %f, %f\n", _normals[i*3+0], _normals[i*3+1], _normals[i*3+2]);
                                                // }
                                            }
                                        }
                                    }
                                }
                                i++;
                            }
                        }
                    }
                }
                if (mi != numVisibleFaces) {
                    printf("Warning: Chunk mesh at %lld,%lld,%lld has not fully initialized!\n%d != %d\n", pos.x, pos.y, pos.z, mi, numVisibleFaces);
                }
                *meshVertices = vertexData;
                *meshIndices = indexData;
            // } else {
               // printf("Warning: Chunk Mesh at %lld,%lld,%lld has no visible faces!\n", pos.x, pos.y, pos.z);
            }
            delete [] facesToBeDrawn;
            return numVisibleFaces;
        }
    };
}

#endif