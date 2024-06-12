
#include "../../lib/raylib/raylib.h"
#include "../../lib/raylib/rlgl.h"
#include "../../lib/raylib/raymath.h"
#include "../../lib/raylib/rcamera.h"
#include "../../lib/raylib/external/glad.h"

#include "bsfg.hpp"

#include "components/TickingBlock.hpp"
#include "components/TickingFunction.hpp"

#include "components/TextureRegistry.hpp"
#include "components/BlockRegistry.hpp"
#include "components/ItemRegistry.hpp"
#include "components/TickingFunctionRegistry.hpp"

#include "components/MeshGenerator.hpp"
#include "components/ScriptInterface.hpp"
#include "CastableRay.hpp"
#include <math.h>
#include <stdint.h>
#include <thread>

using namespace Components;
TextureRegistry GlobalTextureRegistry;
BlockRegistry GlobalBlockRegistry;
ItemRegistry GlobalItemRegistry;
TickingFunctionRegistry GlobalTickingFunctionRegistry;
GeneratorFunctionRegistry GlobalGeneratorRegistry;
World *GlobalWorldAccess;

#define GLSL_VERSION 330
#define OpenGLDebug(s) if (GLenum e = glGetError()) printf("%s: OpenGL Error: %u\n", s, e)

namespace BSFG {
    Engine::Engine(std::string title, long long seed) {
        this->world = new World(seed);
        this->title = title;
        targetedBlock = BlockPos(0, 0, 0);
        targetedFace = -1;
        chunkGenQueue = new Queue<BlockPos, CHUNK_LOAD_QUEUE_SIZE>;
        chunkMeshQueue = new Queue<BlockPos, CHUNK_LOAD_QUEUE_SIZE>;
        taking_screenshot = false;
        SetTraceLogLevel(LOG_WARNING);
        InitWindow(640, 480, title.c_str());
        InitAudioDevice();
        SetWindowMinSize(320, 240);
        SetWindowState(FLAG_WINDOW_RESIZABLE);
        SetExitKey(-1);
        SetTargetFPS(60);
        GlobalWorldAccess = world;
        playerSpeed = PLAYER_SPEED;
        playerHeight = PLAYER_HEIGHT;
        playerReachDistance = PLAYER_REACH_DISTANCE;
        playerJump = PLAYER_JUMP;
        playerRunModifier = PLAYER_RUN_MODIFIER;
        playerVelocity = {0, 0, 0};
        playerIsGrounded = false;
        mouseSensitivity = MOUSE_SENSITIVITY;
        gravityEnabled = true;
        collisionEnabled = true;
        currentBlockId = 1;
        camera.position = { 0.0f, 0.0f, 0.0f };
        camera.target   = { 1.6f, 0.0f, 0.0f };
        camera.up       = { 0.0f, 1.0f, 0.0f };
        camera.fovy = 60.0f;
        camera.projection = CAMERA_PERSPECTIVE;

        scriptInterface = ScriptInterface();

        DisableCursor();
        skyboxModel = LoadModelFromMesh(GenMeshCube(1, 1, 1));
        // Load skybox shader and set required locations
        // NOTE: Some locations are automatically set at shader loading
        skyboxModel.materials[0].shader = LoadShader(TextFormat("assets/shaders/glsl%i/skybox.vs", GLSL_VERSION),
                                                TextFormat("assets/shaders/glsl%i/skybox.fs", GLSL_VERSION));

        SetShaderValue(skyboxModel.materials[0].shader,
            GetShaderLocation(skyboxModel.materials[0].shader, "environmentMap"), new int[1] { MATERIAL_MAP_CUBEMAP }, SHADER_UNIFORM_INT);
        
        SetShaderValue(skyboxModel.materials[0].shader,
            GetShaderLocation(skyboxModel.materials[0].shader, "doGamma"), new int[1] {0}, SHADER_UNIFORM_INT);
        
        SetShaderValue(skyboxModel.materials[0].shader,
            GetShaderLocation(skyboxModel.materials[0].shader, "vflipped"), new int[1] { 0 }, SHADER_UNIFORM_INT);

        Image skybox = LoadImage("assets/textures/skybox.png");
        skyboxModel.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = LoadTextureCubemap(skybox, CUBEMAP_LAYOUT_AUTO_DETECT);

        Shader voxelShader =  LoadShader(TextFormat("assets/shaders/glsl%i/voxel.vs", GLSL_VERSION),
                                                TextFormat("assets/shaders/glsl%i/voxel.fs", GLSL_VERSION));

        glBindAttribLocation(voxelShader.id, 0, "vertexInfo1");
        glBindAttribLocation(voxelShader.id, 1, "vertexInfo2");

        chunkMaterial = LoadMaterialDefault();
        chunkMaterial.shader = voxelShader;
        renderDistance = 10;
        generatorAreaCube = 0;
        numChunks = 0;
        chunkVBOs = nullptr;
        chunkNumFaces = nullptr;
        chunkPositions = nullptr;
        ReloadRenderDistance(renderDistance);
        chunkLoadTimer = 1.0f;
    }

    void Engine::ReloadRenderDistance(size_t newRenderDistance) {
        printf("Reloading render distance...\n");
        long long rdx = newRenderDistance - renderDistance;
        bool increasing = rdx > 0;
        bool first = false;
        camera.position.x += newRenderDistance - renderDistance;
        camera.position.z += newRenderDistance - renderDistance;
        generatorAreaSquare = 4 * (newRenderDistance+1) * (newRenderDistance+1);
        size_t oldGeneratorAreaCube = generatorAreaCube;
        generatorAreaCube = WORLD_HEIGHT / CHUNK_W * generatorAreaSquare;
        generatorArea = ceil(3.142 * (newRenderDistance+1) * (newRenderDistance+1));

        unsigned int **newChunkVBOs = new unsigned int*[generatorAreaCube*2] {nullptr};
        unsigned int *newChunkNumFaces = new unsigned int[generatorAreaCube*2] {0};
        Vector3 *newChunkPositions = new Vector3[generatorAreaCube*2];


        if (oldGeneratorAreaCube == 0) {
            // generate VAOs/VBOs for the first time
            printf("Generating VBOs/VAOs for the first time...\n");
            first = true;
            chunkVBOs = newChunkVBOs;
            chunkNumFaces = newChunkNumFaces;
            chunkPositions = newChunkPositions;
            for (size_t i=0; i<generatorAreaCube*2; i++) {
                if (chunkVBOs[i] == nullptr) {
                    chunkVBOs[i] = new unsigned int[3];
                }
                glGenBuffers(2, chunkVBOs[i]);
                glGenVertexArrays(1, &chunkVBOs[i][2]);
            }
        } else if (increasing) {
            // upsize
            printf("Increasing render distance...\n");
            size_t i = 0;
            for (int z=rdx; z<newRenderDistance*2-rdx; z++) {
                for (int x=rdx; x<newRenderDistance*2-rdx; x++) {
                    size_t j = x+z*2*newRenderDistance;
                    newChunkVBOs[j*2] = chunkVBOs[i];
                    newChunkNumFaces[j*2] = chunkNumFaces[i];
                    newChunkPositions[j*2] = chunkPositions[i];
                    i++;
                    newChunkVBOs[j*2+1] = chunkVBOs[i];
                    newChunkNumFaces[j*2+1] = chunkNumFaces[i];
                    newChunkPositions[j*2+1] = chunkPositions[i];
                    i++;
                }
            }
        } else {
            // downsize
            printf("Decreasing render distance...\n");
            size_t i=0;
            rdx = -rdx;
            for (int z=0; z<renderDistance*2; z++) {
                for (int x=0; x<renderDistance*2; x++) {
                    size_t j = x+z*2*renderDistance;
                    if (x<rdx && x>=renderDistance*2-rdx && z<rdx && z>=renderDistance*2-rdx) {
                        // free unneeded meshes
                        if (chunkVBOs[j*2] != nullptr) {
                            glDeleteBuffers(2, chunkVBOs[j*2]);
                            glDeleteVertexArrays(1, &chunkVBOs[j*2][2]);
                            delete chunkVBOs[j*2];
                        }
                        if (chunkVBOs[j*2+1] != nullptr) {
                            glDeleteBuffers(2, chunkVBOs[j*2+1]);
                            glDeleteVertexArrays(1, &chunkVBOs[j*2+1][2]);
                            delete chunkVBOs[j*2+1];
                        }
                        newChunkNumFaces[i] = 0;
                    } else {
                        // copy over needed meshes
                        newChunkVBOs[i] = chunkVBOs[j*2];
                        newChunkNumFaces[i] = chunkNumFaces[j*2];
                        newChunkPositions[i] = chunkPositions[j*2];
                        i++;
                        newChunkVBOs[i] = chunkVBOs[j*2+1];
                        newChunkNumFaces[i] = chunkNumFaces[j*2+1];
                        newChunkPositions[i] = chunkPositions[j*2+1];
                        i++;
                    }
                }
            }
        }

        if (!first) {
            // if not the first time we need to free and reassign the mesh data arrays
            printf("Reassigning mesh data arrays...\n");
            delete chunkVBOs;
            delete chunkNumFaces;
            delete chunkPositions;
            newChunkVBOs = chunkVBOs;
            chunkNumFaces = newChunkNumFaces;
            chunkPositions = newChunkPositions;
        }

        generatorIndex = 0;
        renderDistance = newRenderDistance;
        numChunks = generatorAreaCube;
        meshVBOAreaStart = BlockPos(-renderDistance, WORLD_BOTTOM, -renderDistance);
        meshVBOAreaMiddle = BlockPos(0, WORLD_BOTTOM, 0);
    }

    void Engine::UnloadAllChunks() {
        world->UnloadAllChunks();
        ReloadAllChunks();
    }

    void Engine::ReloadAllChunks() {
        for (long long y=WORLD_BOTTOM; y<WORLD_BOTTOM+WORLD_HEIGHT; y+=CHUNK_W) {
            for (long long z=meshVBOAreaStart.z; z<=(meshVBOAreaStart.z+(signed)renderDistance*2); z++) {
                for (long long x=meshVBOAreaStart.x; x<=(meshVBOAreaStart.x+(signed)renderDistance*2); x++) {
                    GenChunkMesh(BlockPos(x*CHUNK_W, y, z*CHUNK_W));
                }
            }
        }
    }

    void Engine::DrawSkybox() {
        rlDisableBackfaceCulling();
        rlDisableDepthMask();
            DrawModel(skyboxModel, (Vector3){0, 0, 0}, 1.0f, WHITE);
        rlEnableBackfaceCulling();
        rlEnableDepthMask();
        glGetError();
    }

    void Engine::DrawMenu(GameMenu menu) {
        ;
    }

    void Engine::Load() {
        LoadJson("assets/default.json");
        atlas = GlobalTextureRegistry.getAtlas();
    }

    bool Engine::Generate() {
        /*
        printf("Generating chunks...\n");
        long long D = (signed)renderDistance*CHUNK_W;
        bool success = world->generate(BlockPos(-D, WORLD_BOTTOM, -D),
            BlockPos(D, WORLD_BOTTOM+WORLD_HEIGHT, D));
        printf("Done!\n");
        SetTargetFPS(60);
        return success;
        */
        return true;
    }

    void Engine::Draw() {
        static bool first_frame = true;
        RenderTexture2D screenshotRT;
        if (IsWindowResized()) {
            onWindowResized();
        }
        BeginDrawing();
        if (taking_screenshot) {
            screenshotRT = LoadRenderTexture(GetRenderWidth(), GetRenderHeight());
            BeginTextureMode(screenshotRT);
        }
        ClearBackground(GRAY);
        BeginMode3D(this->camera);
        DrawSkybox();

        // glDisable(GL_CULL_FACE);
        glUseProgram(chunkMaterial.shader.id);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlas.id);
        glUniform1i(chunkMaterial.shader.locs[SHADER_LOC_MAP_DIFFUSE], 0);
        unsigned int chunkposloc = GetShaderLocation(chunkMaterial.shader, "chunkPosition");

        ComputeFrustum();

        for (int i=0; i<numChunks; i++) {
            if (chunkNumFaces[i] > 0 && chunkNumFaces[i] != 0xffffffff && chunkVBOs[i] != nullptr) {
                if (IsChunkInView(chunkPositions[i])) {
                    glUniform3f(chunkposloc, chunkPositions[i].x, chunkPositions[i].y, chunkPositions[i].z);
                    Matrix matModel, matModelView;
                    Matrix matView = rlGetMatrixModelview();
                    Matrix matProjection = rlGetMatrixProjection();
                    matModel = rlGetMatrixTransform();
                    matModelView = MatrixMultiply(matModel, matView);
                    Matrix matModelViewProjection = MatrixMultiply(matModelView, matProjection);
                    rlSetUniformMatrix(chunkMaterial.shader.locs[SHADER_LOC_MATRIX_MVP], matModelViewProjection);

                    glBindVertexArray(chunkVBOs[i][2]);
                    glDrawElements(GL_TRIANGLES, chunkNumFaces[i]*6, GL_UNSIGNED_SHORT, 0);
                    glBindVertexArray(0);
                }
            }
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        if (targetedFace != -1) {
            DrawCubeWires(Vector3{targetedBlock.x+0.5f, targetedBlock.y+0.5f, targetedBlock.z+0.5f},
                 1, 1, 1, BLACK);
        }
        EndMode3D();
        char buffer[64];
        DrawRectangle(1, 2, GetRenderWidth()-2, 44, {0, 0, 0, 64});
        DrawFPS(2, 3);
        sprintf(buffer, "%f,%f,%f", camera.position.x, camera.position.y, camera.position.z);
        DrawText(buffer, 1, 23, 20, WHITE);
        sprintf(buffer, "Queued Gen %llu Mesh %llu", chunkGenQueue->available(), chunkMeshQueue->available());
        DrawText(buffer, 100, 2, 20, WHITE);
        sprintf(buffer, "ViewAngle: %f", targetAngle*180/PI);
        DrawText(buffer, 350, 2, 20, WHITE);
        if (taking_screenshot) {
            EndTextureMode();
            char file_name[64];
            time_t t = time(NULL);
            strftime(file_name, 64, "BSFG_%Y_%B_%d_%I_%M_%S_%p.png", localtime(&t));
            printf("Saving screenshot %s.\n", file_name);

            Image img = LoadImageFromTexture(screenshotRT.texture);
            ImageFlipVertical(&img);
            ExportImage(img, file_name);
            UnloadImage(img);
            UnloadRenderTexture(screenshotRT);
            taking_screenshot = false;
        }
        EndDrawing();
    }

    void Engine::Spawn() {
        long long y = 40;
        while (world->getBlock(0, y++, 0)->isSolid) {
            if (y >= WORLD_BOTTOM+WORLD_HEIGHT) {
                break;
            }
        }
        SetCameraPosition({0.5f, (float)y, 0.5f});
    }

    void Engine::SetCameraPosition(Vector3 pos) {
        Vector3 delta = Vector3Subtract(camera.target, camera.position);
        camera.position = pos;
        camera.target = Vector3Add(pos, delta);
    }

    void Engine::LoadJson(const char *path) {
        loadedContentPath = path;
        Engine::ReloadJson();
    }

    void Engine::UnloadContent() {
        printf("Info: Unloading all content...");
        GlobalBlockRegistry.clear();
        GlobalItemRegistry.clear();
        GlobalTickingFunctionRegistry.clear();
        GlobalGeneratorRegistry.clear();
        GlobalTextureRegistry.clear();
    }

    void Engine::ReloadAtlas() {
        UnloadTexture(atlas);
        atlas = GlobalTextureRegistry.getAtlas();
        // GlobalTextureRegistry.exportAtlas();
    }

    void Engine::ReloadJson() {
        FILE *fd = fopen(loadedContentPath, "r");
        if (fd == NULL) {
            printf("Failed to load open JSON file %s\n", loadedContentPath);
            return;
        }
        printf("Loading Content from JSON file %s\n", loadedContentPath);
        fseek(fd, 0, SEEK_END);
        size_t len = ftell(fd);
        fseek(fd, 0, SEEK_SET);
        void *data = malloc(len);
        if (data != NULL) {
            fread(data, len, 1, fd);
        }
        fclose(fd);
        JSON::JSON json = *JSON::JSON::deserialize((const char*)data);
        free(data);
        // printf("%s\n", json.serialize());
        if (json.getType() == JSON::Type::Object) {
            if (json.contains("skybox") && json["skybox"]->getType() == JSON::Type::String) {
                // unload old skybox
                UnloadTexture(skyboxModel.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture);
                // load new skybox
                Image skybox = LoadImage(("assets/" + json["skybox"]->getString()).c_str());
                skyboxModel.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = LoadTextureCubemap(skybox, CUBEMAP_LAYOUT_PANORAMA);
            }
            if (json.contains("textures") && json["textures"]->getType() == JSON::Type::Object) {
                printf("Loading Textures...\n");
                GlobalTextureRegistry.Load(json["textures"]->getObject());
            }
            if (json.contains("blocks") && json["blocks"]->getType() == JSON::Type::Object) {
                printf("Loading Blocks...\n");
                GlobalBlockRegistry.Load(json["blocks"]->getObject());
            }
            if (json.contains("items") && json["items"]->getType() == JSON::Type::Object) {
                printf("Loading Items...\n");
                GlobalItemRegistry.Load(json["items"]->getObject());
            }
            if (json.contains("tickingfunctions") && json["tickingfunctions"]->getType() == JSON::Type::Object) {
                printf("Loading Ticking Functions...\n");
                GlobalTickingFunctionRegistry.Load(json["tickingfunctions"]->getObject(), &scriptInterface);
            }
            if (json.contains("generators") && json["generators"]->getType() == JSON::Type::Object) {
                printf("Loading Generator Functions...\n");
                GlobalGeneratorRegistry.Load(json["generators"]->getObject(), &scriptInterface);
            }
            if (json.contains("noise") && json["noise"]->getType() == JSON::Type::Object) {
                printf("Loading Noise Settings...\n");
                world->LoadNoiseSettings(json["noise"]->getObject());
            }
            for (size_t i=1; i<=GlobalBlockRegistry.getLastId(); i++) {
                Block *block = GlobalBlockRegistry.getById(i);
                if (block->functionstr != nullptr) {
                    block->function = GlobalTickingFunctionRegistry.get(block->functionstr)->getId();
                    if (block->function != 0) {
                        block->isTicking = true;
                    } else {
                        printf("Missing tick function %s for block %s\n", block->functionstr, block->name); 
                    }
                }
            }
        }
    }

    long long Engine::GetChunkVBOOffset(BlockPos pos) {
        BlockPos p = pos.chunkPos() / CHUNK_W;
        return GetChunkVBOOffset(p.x, p.y, p.z);
    }

    long long Engine::GetChunkVBOOffset(long long x, long long y, long long z) {
        return 2*((x+1-meshVBOAreaStart.x) + (z+1-meshVBOAreaStart.z)*2*renderDistance + (y-meshVBOAreaStart.y)*4*renderDistance*renderDistance);
    }

    void Engine::HandleInputs() {
        float dt = GetFrameTime();
        if (IsKeyPressed(KEY_F11)) {
            int monitor = GetCurrentMonitor();
            if (IsWindowFullscreen()) {
                ToggleFullscreen();
                SetWindowSize((int)GetMonitorWidth(monitor)*0.98f, (int)GetMonitorHeight(monitor)*0.98f);
                SetWindowPosition(20, 20);
            } else {
                ToggleFullscreen();
                // SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
            }
        }

        if (IsKeyPressed(KEY_F2)) {
            taking_screenshot = true;
        }

        // UpdateCamera(&camera, CAMERA_FIRST_PERSON);
        float angle = atan2(camera.target.z - camera.position.z, camera.target.x - camera.position.x);
        float dx = dt*cos(angle);
        float dz = dt*sin(angle);
        if (IsKeyDown(KEY_LEFT_CONTROL)) {
            dx *= playerRunModifier;
            dz *= playerRunModifier;
        }
        if (IsKeyDown(KEY_W)) {
            playerVelocity.x += dx * playerSpeed;
            playerVelocity.z += dz * playerSpeed;
        }
        if (IsKeyDown(KEY_S)) {
            Vector2 r = Vector2Rotate({dx, dz}, PI);
            playerVelocity.x += r.x * playerSpeed;
            playerVelocity.z += r.y * playerSpeed;
        }
        if (IsKeyDown(KEY_A)) {
            Vector2 r = Vector2Rotate({dx, dz}, -PI*0.5f);
            playerVelocity.x += r.x * playerSpeed;
            playerVelocity.z += r.y * playerSpeed;
        }
        if (IsKeyDown(KEY_D)) {
            Vector2 r = Vector2Rotate({dx, dz}, PI*0.5f);
            playerVelocity.x += r.x * playerSpeed;
            playerVelocity.z += r.y * playerSpeed;
        }
        Vector2 mouseDelta = GetMouseDelta();
        CameraYaw(&camera, -mouseDelta.x*dt*mouseSensitivity, false);
        CameraPitch(&camera, -mouseDelta.y*dt*mouseSensitivity, true, false, false);

        if (IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) {
            if (targetedFace != -1) {
                currentBlockId = world->getBlock(targetedBlock)->getId();
            }
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (targetedFace != -1) {
                world->setBlock(targetedBlock, GlobalBlockRegistry.getById(0));
                GenChunkMeshAndNeighbors(targetedBlock);
            }
        }

        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            if (targetedFace != -1) {
                world->placeBlock(targetedBlock, targetedFace, GlobalBlockRegistry.getById(currentBlockId));
                GenChunkMeshAndNeighbors(targetedBlock + world->placeOffset(targetedFace));
            }
        }
        float mouseWheel = GetMouseWheelMove();
        if (mouseWheel < 0) {
            if (currentBlockId <= 1) {
                currentBlockId = GlobalBlockRegistry.getLastId();
            } else {
                currentBlockId--;
            }
        }
        if (mouseWheel > 0) {
            if (currentBlockId >= GlobalBlockRegistry.getLastId()) {
                currentBlockId = 1;
            } else {
                currentBlockId++;
            }
        }

        if (IsKeyDown(KEY_HOME)) {
            SetCameraPosition({0, 66+playerHeight, 0});
        }

        if (IsKeyDown(KEY_SPACE)) {
            if (!gravityEnabled) {
                playerVelocity.y += playerJump*1.5;
            } else if (playerIsGrounded) {
                playerVelocity.y += playerJump;
            }
        }
        
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
            if (!gravityEnabled) {
                playerVelocity.y -= playerJump*1.5f;
            }
        }

        if (IsKeyPressed(KEY_EQUAL)) {
            printf("Changing render distance! Old: %llu New: %llu\n", renderDistance, renderDistance+1);
            ReloadRenderDistance(renderDistance+2);
        }

        if (IsKeyPressed(KEY_MINUS)) {
            if (renderDistance > 1) {
                printf("Changing render distance! Old: %llu New: %llu\n", renderDistance, renderDistance-1);
                ReloadRenderDistance(renderDistance-2);
            }
        }

        if (IsKeyPressed(KEY_F4)) {
            world->save("saves/world/");
        }

        if (IsKeyPressed(KEY_G)) {
            gravityEnabled = !gravityEnabled;
            if (gravityEnabled) {
                collisionEnabled = true;
            }
        }
        if (IsKeyPressed(KEY_C)) {
            collisionEnabled = !collisionEnabled;
            if (!collisionEnabled) {
                gravityEnabled = false;
            }
        }

        if (IsKeyPressed(KEY_F5)) {
            Engine::UnloadContent();
            Engine::ReloadJson();
            Engine::ReloadAtlas();
        }

        if (IsKeyPressed(KEY_F6)) {
            UnloadAllChunks();
        }

    }
    
    bool Engine::LoadCameraPositions(JSON::JSONArray a) {
        if (a.length < 9) {
            return false;
        }
        camera.position = Vector3 {(float)a[0]->getFloat(), (float)a[1]->getFloat(), (float)a[2]->getFloat()};
        camera.target   = Vector3 {(float)a[3]->getFloat(), (float)a[4]->getFloat(), (float)a[5]->getFloat()};
        camera.up       = Vector3 {(float)a[6]->getFloat(), (float)a[7]->getFloat(), (float)a[8]->getFloat()};
        return true;
    }

    JSON::JSONArray *Engine::SaveCameraPositions() {
        JSON::JSONArray *a = new JSON::JSONArray(9);
        a->append(JSON::JSON::fromDouble(camera.position.x));
        a->append(JSON::JSON::fromDouble(camera.position.y));
        a->append(JSON::JSON::fromDouble(camera.position.z));
        a->append(JSON::JSON::fromDouble(camera.target.x));
        a->append(JSON::JSON::fromDouble(camera.target.y));
        a->append(JSON::JSON::fromDouble(camera.target.z));
        a->append(JSON::JSON::fromDouble(camera.up.x));
        a->append(JSON::JSON::fromDouble(camera.up.y));
        a->append(JSON::JSON::fromDouble(camera.up.z));
        return a;
    }

    bool Engine::isRunning(void) {
        return !WindowShouldClose();
    }

    void Engine::onWindowResized(void) {
        
    }

    void Engine::GenChunkMeshAndNeighbors(BlockPos pos, bool all) {
        // 6 cardinal directions
        static const char dr[] {
            0, 0, 0,
            0, CHUNK_W, 0,
            0, -CHUNK_W, 0,
            CHUNK_W, 0, 0,
            -CHUNK_W, 0, 0,
            0, 0, CHUNK_W,
            0, 0, -CHUNK_W,
        };
        // determine which directions are directly beside BlockPos pos
        BlockPos p2 = pos & CHUNK_W_MASK;
        bool m[] {
            1,
            p2.y==CHUNK_W_MASK,
            p2.y==0,
            p2.x==CHUNK_W_MASK,
            p2.x==0,
            p2.z==CHUNK_W_MASK,
            p2.z==0,
        };
        char ii = 0;
        for (char i=0; i<7; i++) {
            if (m[i] || all) {
                BlockPos tmp = pos + BlockPos(dr[ii+0], dr[ii+1], dr[ii+2]);
                if (tmp.y >= WORLD_BOTTOM && tmp.y < WORLD_BOTTOM+WORLD_HEIGHT) {
                    GenChunkMesh(tmp);
                }
            }
            ii += 3;
        }
    }

    void Engine::GenChunkMesh(BlockPos pos) {
        if (world->getChunk(pos)==nullptr) {
            chunkGenQueue->append(pos);
        } else {
            chunkMeshQueue->append(pos);
        }
    }

    void Engine::GenQueuedChunks() {
        size_t generated = 0;
        // generate world data
        while (chunkGenQueue->available() > 0) {
            if (generated < CHUNKS_PER_FRAME) {
                BlockPos pos = chunkGenQueue->pop();
                world->generateChunk(pos);
                chunkMeshQueue->append(pos);
                generated++;
            } else {
                break;
            }
        }
        generated = 0;
        while (chunkMeshQueue->available() > 0) {
            if (generated < MESH_CHUNKS_PER_FRAME) {
                BlockPos pos = chunkMeshQueue->pop();
                if (GenChunkMeshNoQueue(BlockPos(pos))) {
                    // successfuly generated the mesh
                    generated++;
                } else {
                    // failed to generate, chunk needs to be generated first
                    chunkMeshQueue->append(pos);
                }
            } else {
                break;
            }
        }
    }

    void Engine::GenChunkThread(World *world, BlockPos *positions, size_t num) {
        for (size_t i=0; i<num; i++) {
            world->generateChunk(positions[i]);
        }
    }

    bool Engine::GenChunkMeshNoQueue(BlockPos pos) {
        if (world->getChunk(pos) == NULL) {
            // don't generate mesh for missing chunk
            // printf("Not generating mesh yet, requeueing not-yet-generated chunk at position %lld,%lld,%lld\n", pos.x, pos.y, pos.z);
            return false;
        }
        // generate mesh for opaque blocks
        _GenChunkMesh(pos, true);
        // generate mesh for transparent blocks
        // _GenChunkMesh(pos, false);
        return true;
    }

    void Engine::_GenChunkMesh(BlockPos pos, bool opaque) {
        BlockPos chunkOffset = pos.chunkPos();
        size_t i = GetChunkVBOOffset(pos) + (opaque?0:1);
        chunkPositions[i] = chunkOffset;
        uint32_t *meshVertices = nullptr;
        unsigned short *meshIndices = nullptr;
        unsigned int tmp = chunkNumFaces[i] = meshGenerator.gen(&meshVertices, &meshIndices, chunkOffset, world, opaque);

        if (chunkNumFaces[i] > 0) {
            if (chunkVBOs[i] == nullptr) {
                chunkVBOs[i] = new unsigned int[3] {0};
            }
            if (chunkVBOs[i][2] == 0) {
                glGenVertexArrays(1, &chunkVBOs[i][2]);
            }
            if (chunkVBOs[i][1] == 0) {
                glGenBuffers(1, &chunkVBOs[i][1]);
            }
            if (chunkVBOs[i][0] == 0) {
                glGenBuffers(1, &chunkVBOs[i][0]);
            }
            glBindVertexArray(chunkVBOs[i][2]);
            glBindBuffer(GL_ARRAY_BUFFER, chunkVBOs[i][0]);
            glBufferData(GL_ARRAY_BUFFER, chunkNumFaces[i]*8*sizeof(uint32_t), meshVertices, GL_STATIC_DRAW);
            glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(uint32_t)*2, nullptr);
            glEnableVertexAttribArray(0);
            glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(uint32_t)*2, (void*)sizeof(uint32_t));
            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunkVBOs[i][1]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, chunkNumFaces[i]*6*sizeof(unsigned short), meshIndices, GL_STATIC_DRAW);
            glBindVertexArray(0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            if (meshVertices != nullptr)
                delete [] meshVertices;
            if (meshIndices != nullptr)
                delete [] meshIndices;
        } else if (chunkVBOs[i] != nullptr) {
            if (chunkVBOs[i][2] != 0) {
                glDeleteVertexArrays(1, &chunkVBOs[i][2]);
            }
            if (chunkVBOs[i][1] != 0) {
                glDeleteBuffers(1, &chunkVBOs[i][1]);
            }
            if (chunkVBOs[i][0] != 0) {
                glDeleteBuffers(1, &chunkVBOs[i][0]);
            }
            chunkVBOs[i][0] = 0;
            chunkVBOs[i][1] = 0;
            chunkVBOs[i][2] = 0;
        }
    }

    void Engine::Update(void) {
        float dt = GetFrameTime();

        // only tick if we're done generating chunks to avoid additional lag
        if (chunkGenQueue->available() < CHUNKS_PER_FRAME) {
            static float ticktimer = 0;
            ticktimer += dt;
            if (ticktimer >= 0.1f) {
                size_t range = 16;
                BlockPos pos = meshVBOAreaMiddle + BlockPos(camera.position);
                if (world->tickBlocks(pos, range)) {
                    GenChunkMeshAndNeighbors(pos, true);
                }
            }
        }

        GenQueuedChunks();
        size_t ci = 0;
        for (int z=-renderDistance*CHUNK_W; z<renderDistance*CHUNK_W; z+=CHUNK_W) {
            for (int x=-renderDistance*CHUNK_W; x<renderDistance*CHUNK_W; x+=CHUNK_W) {
                for (int y=WORLD_BOTTOM; y<WORLD_BOTTOM+WORLD_HEIGHT; y+=CHUNK_W) {
                    if (chunkNumFaces[ci] > 0 && chunkNumFaces[ci] != 0xffffffff) {
                        chunkGenQueue->append(BlockPos(x, y, z));
                        chunkNumFaces[ci] = 0xffffffff;
                    }
                    ci++;
                }
            }
        }

        for (size_t i=0; i<MESH_CHUNKS_PER_FRAME; i++) {
            if (chunkMeshQueue->available() > 0) {
                BlockPos pos = chunkMeshQueue->pop();
                size_t i = GetChunkVBOOffset(pos);
                chunkPositions[i+1] = chunkPositions[i] = Vector3{(float)pos.x, (float)pos.y, (float)pos.z};
                GenChunkMesh(BlockPos(pos.x, pos.y, pos.z));
            } else {
                break;
            }
        }
    }

    static bool CheckHit(BlockPos pos, World *world) {
        return world->getBlock(pos)->isSolid;
    }

    void Engine::LateUpdate(void) {
        float dt = GetFrameTime();
        Vector3 delta = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        CastableRay targetRay = CastableRay(camera.position, delta);
        targetRay = targetRay.cast(meshVBOAreaMiddle, playerReachDistance, CheckHit, world);
        targetedBlock = targetRay.position;
        targetedFace = targetRay.face;

        // apply gravity
        if (gravityEnabled) {
            playerVelocity.y += -0.981*dt;
        }
        // cap gravity
        if (playerVelocity.y < -0.5f) {
            playerVelocity.y = -0.5f;
        }
        // cap vertical movement
        if (playerVelocity.y > 0.5f) {
            playerVelocity.y = 0.5f;
        }
        // handle gravity and player movements
        HandleVelocity(dt);
        playerVelocity.x = playerVelocity.z = 0;
    }

    bool Engine::HandleVelocity(float dt) {
        if (!collisionEnabled) {
            camera.position = Vector3Add(camera.position, playerVelocity);
            camera.target = Vector3Add(camera.target, playerVelocity);
            playerVelocity = Vector3Multiply(playerVelocity, {1.0f - 2*dt});
            return false;
        }
        bool collided = false;
        long long x1 = floor(meshVBOAreaMiddle.x+camera.position.x-PLAYER_WIDTH);
        long long y1 = floor(meshVBOAreaMiddle.y+camera.position.y-playerHeight);
        long long z1 = floor(meshVBOAreaMiddle.z+camera.position.z-PLAYER_WIDTH);
        long long x2 = floor(meshVBOAreaMiddle.x+camera.position.x+PLAYER_WIDTH);
        long long y2 = floor(meshVBOAreaMiddle.y+camera.position.y+PLAYER_WIDTH);
        long long z2 = floor(meshVBOAreaMiddle.z+camera.position.z+PLAYER_WIDTH);

        // horizontal collision
        long long x3 = floor(meshVBOAreaMiddle.x+camera.position.x + playerVelocity.x);
        long long y3 = floor(meshVBOAreaMiddle.y+camera.position.y + 0.5f - playerHeight);
        long long z3 = floor(meshVBOAreaMiddle.z+camera.position.z + playerVelocity.z);
        long long x4 = floor(meshVBOAreaMiddle.x+camera.position.x + playerVelocity.x);
        long long y4 = floor(meshVBOAreaMiddle.y+camera.position.y + 1.5f - playerHeight);
        long long z4 = floor(meshVBOAreaMiddle.z+camera.position.z + playerVelocity.z);

        if (playerVelocity.x < 0) {
            // -x
            if (world->getBlock(x1, y3, z3)->isSolid ||
                world->getBlock(x1, y3, z4)->isSolid ||
                world->getBlock(x1, y4, z3)->isSolid ||
                world->getBlock(x1, y4, z4)->isSolid) {
                    float nx = x1 + 1.0f + PLAYER_WIDTH - meshVBOAreaMiddle.x;
                    float dx = nx - camera.position.x;
                    camera.position.x = nx;
                    camera.target.x += dx;
                    collided = true;
                    playerVelocity.x = 0;
            }
        } else if (playerVelocity.x > 0) {
            // +x
            if (world->getBlock(x2, y3, z3)->isSolid ||
                world->getBlock(x2, y3, z4)->isSolid ||
                world->getBlock(x2, y4, z3)->isSolid ||
                world->getBlock(x2, y4, z4)->isSolid) {
                    float nx = x2 - meshVBOAreaMiddle.x - PLAYER_WIDTH;
                    float dx = nx - camera.position.x;
                    camera.position.x = nx;
                    camera.target.x += dx;
                    collided = true;
                    playerVelocity.x = 0;
            }
        }
        if (playerVelocity.z < 0) {
            // -z
            if (world->getBlock(x3, y3, z1)->isSolid ||
                world->getBlock(x4, y3, z1)->isSolid ||
                world->getBlock(x3, y4, z1)->isSolid ||
                world->getBlock(x4, y4, z1)->isSolid) {
                    float nz = z1 + 1.0f + PLAYER_WIDTH - meshVBOAreaMiddle.z;
                    float dz = nz - camera.position.z;
                    camera.position.z = nz;
                    camera.target.z += dz;
                    collided = true;
                    playerVelocity.z = 0;
            }
        } else if (playerVelocity.z > 0) {
            // +z
            if (world->getBlock(x3, y3, z2)->isSolid ||
                world->getBlock(x4, y3, z2)->isSolid ||
                world->getBlock(x3, y4, z2)->isSolid ||
                world->getBlock(x4, y4, z2)->isSolid) {
                    float nz = z2 - meshVBOAreaMiddle.z - PLAYER_WIDTH;
                    float dz = nz - camera.position.z;
                    camera.position.z = nz;
                    camera.target.z += dz;
                    collided = true;
                    playerVelocity.z = 0;
            }
        }

        x1 = floor(meshVBOAreaMiddle.x+camera.position.x-PLAYER_WIDTH);
        z1 = floor(meshVBOAreaMiddle.z+camera.position.z-PLAYER_WIDTH);
        x2 = floor(meshVBOAreaMiddle.x+camera.position.x+PLAYER_WIDTH);
        z2 = floor(meshVBOAreaMiddle.z+camera.position.z+PLAYER_WIDTH);

        // downward collision
        playerIsGrounded = false;
        if (world->getBlock(x1, y1, z1)->isSolid ||
            world->getBlock(x1, y1, z2)->isSolid ||
            world->getBlock(x2, y1, z1)->isSolid ||
            world->getBlock(x2, y1, z2)->isSolid) {
            if (playerVelocity.y < 0) {
                float ny = y1 + 0.999f + playerHeight - meshVBOAreaMiddle.y;
                float dy = ny - camera.position.y;
                camera.position.y = ny;
                camera.target.y += dy;
                playerVelocity.y = 0;
                collided = true;
                playerIsGrounded = true;
            }
        }
        // upward collision
        if (world->getBlock(x1, y2, z1)->isSolid ||
            world->getBlock(x1, y2, z2)->isSolid ||
            world->getBlock(x2, y2, z1)->isSolid ||
            world->getBlock(x2, y2, z2)->isSolid) {
            if (playerVelocity.y > 0) {
                float ny = y2 - 0.001f - meshVBOAreaMiddle.y - PLAYER_WIDTH;
                float dy = ny - camera.position.y;
                camera.position.y = ny;
                camera.target.y += dy;
                playerVelocity.y = 0;
                playerIsGrounded = true;
                collided = true;
            }
        }

        camera.position = Vector3Add(camera.position, playerVelocity);
        camera.target = Vector3Add(camera.target, playerVelocity);

        return collided;
    }

    void Engine::ComputeFrustum() {
        targetAngle = atan2f(camera.target.z - camera.position.z, camera.target.x - camera.position.x);
        aspectRatio = (float)GetRenderWidth() / (float)GetRenderHeight();
        fovx = camera.fovy * aspectRatio;
    }

    bool Engine::IsChunkInView(Vector3 pos) {
        //float chunkAngle = atan2f(pos.z - camera.target.z, pos.x - camera.target.x);
        //return fabsf(chunkAngle - targetAngle) <= fovx * 0.5f + FRUSTUM_FOV_PADDING;
        return true;
    }

}
