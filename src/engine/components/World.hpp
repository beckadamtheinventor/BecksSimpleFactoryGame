#ifndef __WORLD_H__
#define __WORLD_H__

#include "Region.hpp"
#include "BlockPos.hpp"
#include "Block.hpp"
#include "CoordinateKeyedMap.hpp"
#include "NoiseSettings.hpp"
#include <time.h>

namespace Components {
    class World {
        BlockPos lastregpos;
        Region *lastregion = nullptr;
        BlockPos lastpos;
        Chunk *lastchunk = nullptr;
        CoordinateKeyedMap<Region*, 8> regions;
        DynamicArray<Region*, 256> regionArray;

        public:
        NoiseSettings *noise;

        World() : World(time(nullptr)) {}
        World(long long seed);
        void LoadNoiseSettings(JSON::JSONObject& JSON);
        void UnloadChunk(BlockPos pos);
        void UnloadAllChunks();
        void generateChunk(BlockPos pos);
        bool generate(BlockPos p1, BlockPos p2);
        inline BlockPos getRegionPosition(BlockPos pos);
        inline std::string getRegionKey(BlockPos pos);
        Region* getRegion(BlockPos pos);
        bool tickBlocks(BlockPos pos, long long range);
        bool setBlock(BlockPos pos, char x, char y, char z, Block *block);
        bool setBlock(long long x, long long y, long long z, Block *block);
        bool setBlock(BlockPos pos, Block *block);
        Chunk *getChunk(BlockPos pos);
        Block* getBlock(BlockPos pos, char x, char y, char z);
        Block* getBlock(long long x, long long y, long long z);
        Block* getBlock(BlockPos pos);
        BlockPos placeOffset(char face);
        bool placeBlock(BlockPos pos, Block *block);
        bool placeBlock(BlockPos pos, char face, Block *block);
        void save(std::string name);
        void load(std::string name);
    };
}

#endif