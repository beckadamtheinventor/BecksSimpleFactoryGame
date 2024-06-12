#ifndef __BLOCKPOS_H__
#define __BLOCKPOS_H__
#include <math.h>
#include <string>
#include <string.h>
#include "Vec3.hpp"


#define REGION_W 16
#define CHUNK_W 32
#define CHUNK_SZ (CHUNK_W*CHUNK_W*CHUNK_W)
#define CHUNK_W_MASK (CHUNK_W-1)

namespace Components {
    class BlockPos : public Vec3i {
        public:
        BlockPos() {
            this->x = this->y = this->z = 0;
        }
        BlockPos(const char *s) {
            this->x = atoi(s);
            s = strchr(s, ',');
            this->y = atoi(s);
            s = strchr(s, ',');
            this->z = atoi(s);
        }
        BlockPos(BlockPos &other) {
            this->x = other.x;
            this->y = other.y;
            this->z = other.z;
        }
        BlockPos(Vec3i other) {
            this->x = other.x;
            this->y = other.y;
            this->z = other.z;
        }
        BlockPos(Vec3d pos, long long x, long long y, long long z) {
            this->x = x + pos.x;
            this->y = y + pos.y;
            this->z = z + pos.z;
        }
        BlockPos(BlockPos pos, long long x, long long y, long long z) {
            this->x = x + pos.x;
            this->y = y + pos.y;
            this->z = z + pos.z;
        }
        BlockPos(long long x, long long y, long long z) {
            this->x = x;
            this->y = y;
            this->z = z;
        }
        BlockPos(Vector3 pos) {
            this->x = pos.x;
            this->y = pos.y;
            this->z = pos.z;
        }
        inline std::string serialize() {
            return std::to_string(this->x)+","+std::to_string(this->y)+","+std::to_string(this->z);
        }
        inline BlockPos chunkPos() {
            return BlockPos(this->x & ~CHUNK_W_MASK, this->y & ~CHUNK_W_MASK, this->z & ~CHUNK_W_MASK);
        }
        inline BlockPos regionPos() {
            return BlockPos(this->x & ~(CHUNK_W*REGION_W-1), this->y & ~(CHUNK_W*REGION_W-1), this->z & ~(CHUNK_W*REGION_W-1));
        }
        inline BlockPos operator/(double v) {
            return BlockPos(this->x/v, this->y/v, this->z/v);
        }
        inline BlockPos operator&(size_t v) {
            return BlockPos(this->x&v, this->y&v, this->z&v);
        }
        inline BlockPos operator*(double v) {
            return BlockPos(this->x*v, this->y*v, this->z*v);
        }
        inline bool operator==(BlockPos other) {
            return this->x == other.x && this->y == other.y && this->z == other.z;
        }
        inline bool operator!=(BlockPos other) {
            return !(this->x == other.x && this->y == other.y && this->z == other.z);
        }
        inline operator Vector3() {
            return Vector3{(float)this->x, (float)this->y, (float)this->z};
        }
    };
}
#endif