#include <cstdio>
#include <iostream>

#include "engine/bsfg.hpp"
#include "json/json.hpp"
#include <stdio.h>
#include <string>
#include <time.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
    JSON::JSON savedata;
    FILE *fd = fopen("save.json", "r");
    if (fd == NULL) {
        savedata.setObject(new JSON::JSONObject());
    } else {
        fseek(fd, 0, SEEK_END);
        size_t len = ftell(fd);
        fseek(fd, 0, SEEK_SET);
        char *data = (char*) malloc(len);
        fread(data, len, 1, fd);
        std::string datastr = std::string(data, len);
        savedata = *JSON::JSON::deserialize(data);
        free(data);
    }

    struct stat attr;
#ifdef PLATFORM_WINDOWS
    if (stat("saves", &attr))
        mkdir("saves");
    if (stat("saves/world", &attr))
        mkdir("saves/world");
#else
    if (stat("saves", &attr))
        mkdir("saves", 0x777);
    if (stat("saves/world", &attr))
        mkdir("saves/world", 0x777);
#endif


    BSFG::Engine *engine = new BSFG::Engine("Beck's Simple Factory Game", time(nullptr));
    engine->Load();

    if (!savedata.contains("camera") || savedata["camera"]->getType() != JSON::Type::Array) {
        savedata.getObject().Add("camera", new JSON::JSON(new JSON::JSONArray(9)));
    } else {
        engine->LoadCameraPositions(savedata["camera"]->getArray());
    }

    if (engine->Generate()) {
        engine->ReloadAllChunks();
        engine->Spawn();

        while (engine->isRunning()) {
            engine->Update();
            engine->Draw();
            engine->HandleInputs();
            engine->LateUpdate();
        }

        savedata["camera"]->setArray(engine->SaveCameraPositions());

        savedata["time"]->set(time(NULL));
        fd = fopen("save.json", "w");
        if (fd != NULL) {
            const char *data = savedata.serialize();
            fwrite(data, strlen(data), 1, fd);
            fclose(fd);
        }
    }

    return 0;
}
