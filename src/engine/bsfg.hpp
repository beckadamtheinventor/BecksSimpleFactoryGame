#ifndef __BSFG_H__
#define __BSFG_H__

#include <string>
#include <time.h>

#include "../../lib/raylib/raylib.h"

#include "components/ScriptInterface.hpp"
#include "components/World.hpp"
#include "components/MeshGenerator.hpp"
#include "components/Queue.hpp"

using namespace Components;

#define PLAYER_WIDTH 0.1f
#define PLAYER_SPEED 10.0f
#define PLAYER_JUMP 10.0f
#define PLAYER_HEIGHT 1.5f
#define PLAYER_RUN_MODIFIER 2.5f
#define PLAYER_REACH_DISTANCE 6.0f
#define MOUSE_SENSITIVITY 0.1f
#define CHUNKS_PER_FRAME 32
#define MESH_CHUNKS_PER_FRAME 64
#define CHUNK_LOAD_QUEUE_SIZE (1024*1024)

#define FRUSTUM_FOV_PADDING (PI/64.0f)

namespace BSFG {

    class Engine {
        std::string title;
        const char *loadedContentPath;
        Camera3D camera;
        Vector3 playerVelocity;
        bool taking_screenshot;
        bool playerIsGrounded;
        bool gravityEnabled;
        bool collisionEnabled;
        World *world;
        ScriptInterface scriptInterface;
        MeshGenerator meshGenerator;
        Texture2D atlas;
        int numChunks;
        char targetedFace;
        unsigned int **chunkVBOs;
        unsigned int *chunkNumFaces;
        Vector3 *chunkPositions;
        Model skyboxModel;
        long long renderDistance;
        long long generatorArea;
        long long generatorAreaSquare;
        long long generatorAreaCube;
        size_t generatorIndex;
        float chunkLoadTimer;
        BlockPos targetedBlock;
        Material chunkMaterial;
        BlockPos meshVBOAreaStart;
        BlockPos meshVBOAreaMiddle;
        blockid_t currentBlockId;
        float playerSpeed;
        float playerReachDistance;
        float playerJump;
        float playerHeight;
        float mouseSensitivity;
        float playerRunModifier;
        float targetAngle;
        float fovx;
        float aspectRatio;
        Queue<BlockPos, CHUNK_LOAD_QUEUE_SIZE> *chunkGenQueue;
        Queue<BlockPos, CHUNK_LOAD_QUEUE_SIZE> *chunkMeshQueue;
        public:

        enum GameMenu {
            Main,
            Settings,
            Game,
        };

        Engine(std::string title, long long seed);

        ~Engine() {
            if (IsWindowFullscreen()) {
                ToggleFullscreen();
            }
            EnableCursor();
            CloseAudioDevice();
            CloseWindow();
        }

        void ReloadRenderDistance(size_t newRenderDistance);
        bool Generate();
        void Load();
        void Spawn();
        void SetCameraPosition(Vector3 pos);
        void LoadJson(const char *path);
        void UnloadContent();
        void ReloadJson();
        void ReloadAtlas();
        void Draw();
        void UnloadAllChunks();
        void ReloadAllChunks();
        void DrawSkybox();
        void DrawMenu(GameMenu menu);
        void HandleInputs();
        bool LoadCameraPositions(JSON::JSONArray a);
        JSON::JSONArray *SaveCameraPositions();

        bool isRunning();
        void onWindowResized();
        long long GetChunkVBOOffset(BlockPos pos);
        long long GetChunkVBOOffset(long long x, long long y, long long z);
        void GenChunkMeshAndNeighbors(BlockPos pos, bool all=false);
        void GenChunkMesh(BlockPos pos);
        void GenQueuedChunks();
        static void GenChunkThread(World *world, BlockPos *positions, size_t num);
        bool GenChunkMeshNoQueue(BlockPos pos);
        void _GenChunkMesh(BlockPos pos, bool opaque);
        void Update();
        void LateUpdate();
        bool HandleVelocity(float dt);
        void ComputeFrustum();
        bool IsChunkInView(Vector3 pos);
    };

}

#endif