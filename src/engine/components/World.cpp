#include "Types.hpp"
#include "Region.hpp"
#include "BlockPos.hpp"
#include "Block.hpp"
#include "TickingBlock.hpp"
#include "World.hpp"
#include "BlockRegistry.hpp"
#include "ItemRegistry.hpp"
// #include "Queue.hpp"
#include "../../json/json.hpp"
#include <thread>

// #define CHUNK_GEN_THREADS 16
extern Components::BlockRegistry GlobalBlockRegistry;
extern Components::ItemRegistry GlobalItemRegistry;

namespace Components {
    World::World(long long seed) {
        noise = new NoiseSettings(seed);
    }
    void World::LoadNoiseSettings(JSON::JSONObject& json) {
        noise->load(json);
    }
    void World::UnloadAllChunks() {
        regions.clear();
    }
    void World::UnloadChunk(BlockPos pos) {
        Region *reg = getRegion(pos);
        Chunk *chunk = reg->get(pos);
        if (chunk != nullptr) {
            delete chunk;
            reg->set(pos, nullptr);
        }
    }
    static void generateChunkThread(World *world, BlockPos pos) {
        Region *reg = world->getRegion(pos);
        reg->generate(pos, world->noise);
    }
    void World::generateChunk(BlockPos pos) {
        Region *reg = getRegion(pos);
        if (reg == nullptr) {
            reg = regions.set(pos, new Region(pos));
            regionArray.append(reg);
        }
        reg->generate(pos, noise);
    }

    void DrawGenerationScreen(size_t done, size_t remain) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Generating Terrain...", 2, 2, 40, BLACK);
        char buffer[64] {0};
        sprintf(buffer, "%llu", done);
        DrawText(buffer, 2, 44, 20, GRAY);
        sprintf(buffer, "/%llu complete", remain);
        DrawText(buffer, 100, 44, 20, GRAY);
        EndDrawing();
    }

    // Generate chunks from position p1 to p2.
    // Displays progress screen.
    // Returns false if window should be closed.
    bool World::generate(BlockPos p1, BlockPos p2) {
        long long x1 = p1.x < p2.x ? p1.x : p2.x;
        long long y1 = p1.y < p2.y ? p1.y : p2.y;
        long long z1 = p1.z < p2.z ? p1.z : p2.z;
        long long x2 = p1.x > p2.x ? p1.x : p2.x;
        long long y2 = p1.y > p2.y ? p1.y : p2.y;
        long long z2 = p1.z > p2.z ? p1.z : p2.z;
        size_t area = (x2-x1)*(y2-y1)*(z2-z1)/(CHUNK_W*CHUNK_W*CHUNK_W);
        size_t complete = 0;
        SetTargetFPS(-1);
        DrawGenerationScreen(complete, area);
        // preallocate regions so we can multithread
        for (long long z3=z1; z3<z2; z3+=REGION_W) {
            for (long long x3=x1; x3<x2; x3+=REGION_W) {
                BlockPos pos = BlockPos(x3, 0, z3);
                if (getRegion(pos) == nullptr) {
                    Region *reg = regions.set(pos, new Region(pos));
                    regionArray.append(reg);
               }
            }
        }
        // Queue<std::thread*, CHUNK_GEN_THREADS> threadQueue;
        for (long long z3=z1; z3<z2; z3+=CHUNK_W) {
            for (long long x3=x1; x3<x2; x3+=CHUNK_W) {
                for (long long y3=y1; y3<y2; y3+=CHUNK_W) {
                    /*
                    if (threadQueue.remaining() == 0) {
                        if (threadQueue.peek()->joinable()) {
                            threadQueue.pop()->join();
                            complete++;
                        }
                        DrawGenerationScreen(complete, area);
                        if (WindowShouldClose()) {
                            return false;
                        }
                    }
                    threadQueue.append(new std::thread(generateChunkThread, this, BlockPos(x3, y3, z3)));
                    */
                    generateChunkThread(this, BlockPos(x3, y3, z3));
                    complete++;
                }
                DrawGenerationScreen(complete, area);
                if (WindowShouldClose()) {
                    return false;
                }
            }
        }
        /*
        while (threadQueue.available() > 0) {
            std::thread *th = threadQueue.pop();
            if (th->joinable()) {
                th->join();
            }
        }
        */
        DrawGenerationScreen(area, area);
        return true;
    }

    inline BlockPos World::getRegionPosition(BlockPos pos) {
        return pos.regionPos() / (REGION_W*CHUNK_W);
    }

    inline std::string World::getRegionKey(BlockPos pos) {
        return getRegionPosition(pos).serialize();
    }
    Region* World::getRegion(BlockPos pos) {
        pos = pos.regionPos();
        if (lastregion != nullptr && pos == lastregpos) {
            return lastregion;
        }
        Region *reg;
        if (regions.get(pos, &reg)) {
            lastregpos = pos;
            return (lastregion = reg);
        }
        return NULL;
    }
    bool World::tickBlocks(BlockPos pos, long long range) {
        bool updatemesh = false;
        for (long long z=pos.z-range; z<pos.z+range; z++) {
            for (long long x=pos.x-range; x<pos.x+range; x++) {
                for (long long y=pos.y-range; y<pos.y+range; y++) {
                    Block *block = getBlock(x, y, z);
                    if (block->isTicking) {
                        TickingBlock state = TickingBlock(block, x, y, z);
                        TickingFunction::TickResult result = state.tick();
                        switch (result) {
                            case TickingFunction::TickResult::Destroy:
                            case TickingFunction::TickResult::Place:
                            case TickingFunction::TickResult::Update:
                            case TickingFunction::TickResult::ReverseUpdate:
                                updatemesh = true;
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
        }
        return updatemesh;
    }
    bool World::setBlock(long long x, long long y, long long z, Block *block) {
        return setBlock(BlockPos(x, y, z), block);
    }
    bool World::setBlock(BlockPos pos, char x, char y, char z, Block *block) {
        return setBlock(pos + BlockPos(x, y, z), block);
    }
    bool World::setBlock(BlockPos pos, Block *block) {
        Chunk *chunk = getChunk(pos);
        if (chunk != NULL) {
            char x = pos.x & CHUNK_W_MASK;
            char y = pos.y & CHUNK_W_MASK;
            char z = pos.z & CHUNK_W_MASK;
            chunk->set(x, y, z, block->getId());
            return true;
        }
        return false;
    }
    Chunk* World::getChunk(BlockPos pos) {
        BlockPos cpos = pos.chunkPos();
        if (lastchunk != nullptr && cpos == lastpos) {
            return lastchunk;
        }
        Region *reg = getRegion(pos);
        if (reg != NULL) {
            Chunk *chunk = reg->get(pos & (CHUNK_W*REGION_W-1));
            if (chunk != NULL) {
                lastpos = cpos;
                lastchunk = chunk;
                return chunk;
            }
        }
        return NULL;
    }

    Block* World::getBlock(BlockPos pos, char x, char y, char z) {
        return getBlock(pos + BlockPos(x, y, z));
    }
    Block* World::getBlock(long long x, long long y, long long z) {
        return getBlock(BlockPos(x, y, z));
    }
    Block* World::getBlock(BlockPos pos) {
        Chunk *chunk = getChunk(pos);
        if (chunk != NULL) {
            char x = pos.x & CHUNK_W_MASK;
            char y = pos.y & CHUNK_W_MASK;
            char z = pos.z & CHUNK_W_MASK;
            return GlobalBlockRegistry.getById(chunk->get(x, y, z));
        }
        return GlobalBlockRegistry.getById(0);
    }
    BlockPos World::placeOffset(char face) {
        BlockPos pos;
        switch (face) {
            case 0:
                pos.y--;
                break;
            case 1:
                pos.y++;
                break;
            case 2:
                pos.x--;
                break;
            case 3:
                pos.x++;
                break;
            case 4:
                pos.z--;
                break;
            case 5:
                pos.z++;
                break;
            default:
                break;
        }
        return pos;
    }
    bool World::placeBlock(BlockPos pos, Block *block) {
        return setBlock(pos, block);
    }
    bool World::placeBlock(BlockPos pos, char face, Block *block) {
        return setBlock(pos + placeOffset(face), block);
    }
    void World::save(std::string name) {
        FILE *fd;
        size_t tmp;

        // save block IDs so we can remap them if needed when loading
        fopen_s(&fd, (name+"/blocks.dat").c_str(), "wb");
        tmp = GlobalBlockRegistry.getLastId();
        fwrite(&tmp, 4, 1, fd);
        for (blockid_t i=0; i<GlobalBlockRegistry.getLastId(); i++) {
            const char *s = GlobalBlockRegistry.getIdentifier(i)->getString();
            size_t tmp = strlen(s);
            if (tmp > 65535) {
                printf("Content String Identifier longer than 65535 characters. How did we get here?\n");
                exit(69);
            }
            fwrite(&tmp, 2, 1, fd);
            fwrite(s, 1, tmp, fd);
            fwrite("\0", 1, 1, fd);
        }
        fclose(fd);

        // save item IDs so we can remap them if needed when loading
        fopen_s(&fd, (name+"/items.dat").c_str(), "wb");
        tmp = GlobalItemRegistry.getLastId();
        fwrite(&tmp, 4, 1, fd);
        for (itemid_t i=0; i<GlobalItemRegistry.getLastId(); i++) {
            const char *s = GlobalItemRegistry.getIdentifier(i)->getString();
            tmp = strlen(s);
            if (tmp > 65535) {
                printf("Content String Identifier longer than 65535 characters. How did we get here?\n");
                exit(69);
            }
            fwrite(&tmp, 2, 1, fd);
            fwrite(s, 1, tmp, fd);
            fwrite("\0", 1, 1, fd);
        }
        fclose(fd);

        // save an index of all regions to be saved and write region data
        fopen_s(&fd, (name+"/index.dat").c_str(), "wb");
        tmp = regionArray.Length();
        fwrite(&tmp, 8, 1, fd);
        for (size_t i=0; i<regionArray.Length(); i++) {
            Region *reg = *regionArray[i];
            std::string s = reg->pos.serialize();
            size_t tmp = s.length();
            if (tmp > 255) {
                printf("Region BlockPos string is longer than 255 characters. How did we get here?\n");
                exit(69);
            }
            fwrite(&tmp, 1, 1, fd);
            fwrite(s.c_str(), 1, s.length(), fd);
            fwrite("\0", 1, 1, fd);
            reg->save((name+"/r"+reg->pos.serialize()+".dat").c_str());
        }
        fclose(fd);
    }
    void World::load(std::string name) {
        FILE *fd;
        size_t tmp;
        bool remap = false;
        // load block IDs so we can remap them if needed
        fopen_s(&fd, (name+"/blocks.dat").c_str(), "wb");
        fread(&tmp, 4, 1, fd);
        blockid_t *remapblocks = new blockid_t[tmp] {0};
        for (size_t i=0; i<tmp; i++) {
            size_t namelen = 0;
            fread(&namelen, 2, 1, fd);
            char *buffer = new char[namelen+1];
            fread(buffer, 1, namelen+1, fd);
            // try to find a matching block ID
            Block *block = GlobalBlockRegistry.get(buffer);
            if (block == nullptr) {
                // no matching block ID, remove it.
                remapblocks[i] = 0;
            } else {
                // remap to new block ID
                remapblocks[i] = block->getId();
                // only run remapper if a block/item ID changed.
                if (block->getId() != i) {
                    remap = true;
                }
            }
        }
        fclose(fd);

        // load item IDs so we can remap them if needed
        fopen_s(&fd, (name+"/items.dat").c_str(), "wb");
        fread(&tmp, 4, 1, fd);
        itemid_t *remapitems = new itemid_t[tmp] {0};
        for (size_t i=0; i<tmp; i++) {
            size_t namelen = 0;
            fread(&namelen, 2, 1, fd);
            char *buffer = new char[namelen+1];
            fread(buffer, 1, namelen+1, fd);
            // try to find a matching block ID
            Item *item = GlobalItemRegistry.get(buffer);
            if (item == nullptr) {
                // no matching item ID, remove it.
                remapitems[i] = 0;
            } else {
                // remap to new item ID
                remapitems[i] = item->getId();
                // only run remapper if a block/item ID changed.
                if (item->getId() != i) {
                    remap = true;
                }
            }
            delete [] buffer;
        }
        fclose(fd);

        // load regions from index
        fopen_s(&fd, (name+"/index.dat").c_str(), "rb");
        fread(&tmp, 8, 1, fd);
        for (size_t i=0; i<tmp; i++) {
            size_t namelen = 0;
            fread(&namelen, 1, 1, fd);
            char *buffer = new char[namelen+1];
            fread(buffer, 1, namelen+1, fd);
            Region *reg = new Region();
            reg->pos = BlockPos(buffer);
            if (reg->load((name+"/r"+reg->pos.serialize()+".dat").c_str())) {
                regions.set(reg->pos, reg);
            } else {
                delete reg;
            }
        }
        fclose(fd);

        // free remapping tables
        delete [] remapitems;
        delete [] remapblocks;
    }
}
