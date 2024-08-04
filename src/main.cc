#include <windows.h>
#include "engine.hh"
#include "camera.hh"
#include "platform.hh"
#include "arena_alloc.hh"
#include "third_party/HandmadeMath.h"

int main(void)
{
    Arena global_arena = CreateNewArena(0, 10 * MB);
    Arena platform_arena = CreateNewArena(&global_arena, sizeof(Platform));
    Platform *platform = CreatePlatform(&platform_arena, 800, 600, "This works too");

    Engine engine = CreateEngine(platform->window);
    Model model = EngineLoadCompiledModel(&engine, "out.cmdl");
    Model model2 = model;

    Camera camera = {};
    camera.cam_pos = {0, 1, -3};
    camera.cam_dir = {0, 0, 1};
    camera.cam_up = {0, 1, 0};

    ProjectionInfo proj_info = {};
    proj_info.fov_rad = 1.57;
    proj_info.width = 800;
    proj_info.height = 600;
    proj_info.near_plane = 0.01;
    CameraSetProjection(&camera, proj_info);

    float i = 0;
    while(true)
    {
        PlatformPollEvents(platform);

        CameraUpdate(&camera, platform->cursor_delta);

        u32 index = EngineBegin(&engine);
        
        i += 1.0/60;
        model.model_matrix = HMM_Rotate_LH(i, {0, 1, 0});
        model2.model_matrix = HMM_Translate({0, 2, 0}) * HMM_Rotate_LH(-i, {0, 1, 0});
        
        Texture swap_texture = EngineGetSwapChainImage(&engine, index);
        EngineBeginRendering(&engine, swap_texture, &engine.depth, {0.4, 0.5, 0.7, 1.0});
        
        EngineDrawModel(&engine, camera.transform, model);
        EngineDrawModel(&engine, camera.transform, model2);
        
        EngineEndRendering(&engine);
        EngineEnd(&engine, index);
    }

    ExitProcess(0);
}
