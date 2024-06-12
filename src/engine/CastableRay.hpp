#ifndef __RAYCAST_HPP__
#define __RAYCAST_HPP__

#include "../../lib/raylib/raylib.h"
#include "../../lib/raylib/raymath.h"

#include <math.h>

#include "components/BlockPos.hpp"
#include "components/World.hpp"

using namespace Components;

namespace BSFG {
    class CastableRay {
        public:
        Vector3 position;
        Vector3 direction;
        char face;

        CastableRay() : CastableRay({0}, {0}) {}

        CastableRay(Vector3 position, Vector3 direction) {
            this->position = position;
            this->direction = direction;
            this->face = -1;
        }

        CastableRay cast(BlockPos start, float maximum, bool (*checkHit)(BlockPos position, World *world), World *world, size_t iterations=25) {
            CastableRay ray = *this;
            float dirX = ray.direction.x;
            float dirY = ray.direction.y;
            float dirZ = ray.direction.z;
            float ddx = dirX==0?1e30:fabs(1.0f / dirX);
            float ddy = dirY==0?1e30:fabs(1.0f / dirY);
            float ddz = dirZ==0?1e30:fabs(1.0f / dirZ);
            float sdx, sdy, sdz;
            long long mx = ray.position.x;
            long long my = ray.position.y;
            long long mz = ray.position.z;
            int stepX, stepY, stepZ;
            char side;
            bool hit = false;
            if (dirX < 0) {
                stepX = -1;
                sdx = (ray.position.x - mx) * ddx;
            } else {
                stepX = 1;
                sdx = (mx + 1.0f - ray.position.x) * ddx;
            }
            if (dirY < 0) {
                stepY = -1;
                sdy = (ray.position.y - my) * ddy;
            } else {
                stepY = 1;
                sdy = (my + 1.0f - ray.position.y) * ddy;
            }
            if (dirZ < 0) {
                stepZ = -1;
                sdz = (ray.position.z - mz) * ddz;
            } else {
                stepZ = 1;
                sdz = (mz + 1.0f - ray.position.z) * ddz;
            }
            mx += start.x;
            my += start.y;
            mz += start.z;
            BlockPos pos = BlockPos(mx, my, mz);
            size_t cycles = 0;
            do {
                if (sdx < sdz) {
                    if (sdy <= sdx) {
                        sdy += ddy;
                        my += stepY;
                        side = 0;
                    } else {
                        sdx += ddx;
                        mx += stepX;
                        side = 2;
                    }
                } else {
                    if (sdy <= sdz) {
                        sdy += ddy;
                        my += stepY;
                        side = 0;
                    } else {
                        sdz += ddz;
                        mz += stepZ;
                        side = 4;
                    }
                }
                pos.x = mx;
                pos.y = my;
                pos.z = mz;
                if (checkHit(pos, world)) {
                    hit = true;
                }
            } while (!hit && cycles++ < iterations);
            if (cycles >= iterations) {
                ray.face = -1;
                ray.position = pos;
                return ray;
            }
            switch (side) {
                case 4:
                    side += (stepZ <= 0);
                    break;
                case 2:
                    side += (stepX <= 0);
                    break;
                case 0:
                    side += (stepY <= 0);
                    break;
                default:
                    break;
            }
            ray.face = side;
            if (Vector3Distance(position, pos) >= maximum) {
                return CastableRay();
            }
            ray.position = pos;
            return ray;
        }
    };
}

#endif