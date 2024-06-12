
#include "ScriptInterface.hpp"
#include "World.hpp"

extern Components::World *GlobalWorldAccess;

namespace Components {
    ScriptInterface::ScriptInterface() {}
    size_t ScriptInterface::getBlock(long long x, long long y, long long z) {
        return GlobalWorldAccess->getBlock(x, y, z)->getId();
    }
    void ScriptInterface::setBlock(long long x, long long y, long long z, size_t b) {
        GlobalWorldAccess->setBlock(x, y, z, GlobalBlockRegistry.getById(b));
    }
    void ScriptInterface::breakBlock(long long x, long long y, long long z) {
        GlobalWorldAccess->setBlock(x, y, z, GlobalBlockRegistry.getById(0));
    }
    size_t ScriptInterface::check3x3(long long x, long long y, long long z, size_t b) {
        size_t n = 0;
        for (char zz=-1; zz<2; zz++) {
            for (char xx=-1; xx<2; xx++) {
                if (GlobalWorldAccess->getBlock(x+xx, y, z+zz)->getId() == b) {
                    n++;
                }
            }
        }
        return n;
    }
    size_t ScriptInterface::check3x3x3(long long x, long long y, long long z, size_t b) {
        size_t n = 0;
        for (char zz=-1; zz<2; zz++) {
            for (char yy=-1; yy<2; yy++) {
                for (char xx=-1; xx<2; xx++) {
                    if (GlobalWorldAccess->getBlock(x+xx, y+yy, z+zz)->getId() == b) {
                        n++;
                    }
                }
            }
        }
        return n;
    }
    size_t ScriptInterface::check5x5(long long x, long long y, long long z, size_t b) {
        size_t n = 0;
        for (char zz=-2; zz<3; zz++) {
            for (char xx=-2; xx<3; xx++) {
                if (GlobalWorldAccess->getBlock(x+xx, y, z+zz)->getId() == b) {
                    n++;
                }
            }
        }
        return n;
    }
    size_t ScriptInterface::check5x5x5(long long x, long long y, long long z, size_t b) {
        size_t n = 0;
        for (char zz=-2; zz<3; zz++) {
            for (char yy=-2; yy<3; yy++) {
                for (char xx=-2; xx<3; xx++) {
                    if (GlobalWorldAccess->getBlock(x+xx, y+yy, z+zz)->getId() == b) {
                        n++;
                    }
                }
            }
        }
        return n;
    }
    void ScriptInterface::replaceBlocks3x3(long long x, long long y, long long z, size_t r, size_t b) {
        for (char zz=-1; zz<2; zz++) {
            for (char xx=-1; xx<2; xx++) {
                if (GlobalWorldAccess->getBlock(x+xx, y, z+zz)->getId() == r) {
                    GlobalWorldAccess->setBlock(x+xx, y, z+zz, GlobalBlockRegistry.getById(b));
                }
            }
        }
    }
    void ScriptInterface::replaceBlocks3x3x3(long long x, long long y, long long z, size_t r, size_t b) {
        for (char zz=-1; zz<2; zz++) {
            for (char yy=-1; yy<2; yy++) {
                for (char xx=-1; xx<2; xx++) {
                    if (GlobalWorldAccess->getBlock(x+xx, y+yy, z+zz)->getId() == r) {
                        GlobalWorldAccess->setBlock(x+xx, y+yy, z+zz, GlobalBlockRegistry.getById(b));
                    }
                }
            }
        }
    }
    void ScriptInterface::replaceBlocks5x5(long long x, long long y, long long z, size_t r, size_t b) {
        for (char zz=-2; zz<3; zz++) {
            for (char xx=-2; xx<3; xx++) {
                if (GlobalWorldAccess->getBlock(x+xx, y, z+zz)->getId() == r) {
                    GlobalWorldAccess->setBlock(x+xx, y, z+zz, GlobalBlockRegistry.getById(b));
                }
            }
        }
    }
    void ScriptInterface::replaceBlocks5x5x5(long long x, long long y, long long z, size_t r, size_t b) {
        for (char zz=-2; zz<3; zz++) {
            for (char yy=-2; yy<3; yy++) {
                for (char xx=-2; xx<3; xx++) {
                    if (GlobalWorldAccess->getBlock(x+xx, y+yy, z+zz)->getId() == r) {
                        GlobalWorldAccess->setBlock(x+xx, y+yy, z+zz, GlobalBlockRegistry.getById(b));
                    }
                }
            }
        }
    }
    void ScriptInterface::spreadBlocks(long long x, long long y, long long z, size_t r, size_t b) {
        const char dr[5*3] = {
            0, -1, 0,
            1, 0, 0,
            -1, 0, 0,
            0, 0, 1,
            0, 0, -1,
        };
        for (char i=0; i<5*3; i+=3) {
            if (GlobalWorldAccess->getBlock(x+dr[i+0], y+dr[i+1], z+dr[i+2])->getId() == r) {
                GlobalWorldAccess->setBlock(x+dr[i+0], y+dr[i+1], z+dr[i+2], GlobalBlockRegistry.getById(b));
            }
        }
    }
    bool ScriptInterface::isSolid(size_t b) {
        return GlobalBlockRegistry.getById(b)->isSolid;
    }
    bool ScriptInterface::isOpaque(size_t b) {
        return GlobalBlockRegistry.getById(b)->isOpaque;
    }
    bool ScriptInterface::isTicking(size_t b) {
        return GlobalBlockRegistry.getById(b)->isTicking;
    }
}
