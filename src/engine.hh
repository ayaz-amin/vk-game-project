#ifndef ENGINE_H
#define ENGINE_H

#include "types.hh"
#include "vk_utils.hh"
#include "vk_pipeline.hh"

#include "third_party/vk_mem_alloc.h"
#include "third_party/HandmadeMath.h"

struct Engine
{
    VkInstance instance;
    VkSurfaceKHR surface;

    Device device;
    SwapChain swapchain;
    Texture depth;
    VkFormat depth_format;
    
    SyncStructs sync;
    Command command;

    VkDescriptorPool mesh_pool;

    u32 frame_idx;
    Pipeline mesh_pipeline;
};

struct Model
{
    VkBuffer vbo;
    VmaAllocation vbo_alloc;
    
    VkBuffer ibo;
    VmaAllocation ibo_alloc;
    u32 num_indices;

    Texture texture;
    VkSampler tex_sampler;
    VkDescriptorSet set;

    HMM_Mat4 model_matrix;
};

Engine CreateEngine(HWND window);
Model EngineLoadCompiledModel(Engine *engine, const char *file_path);

u32 EngineBegin(Engine *engine);
void EngineEnd(Engine *engine, uint32_t img_idx);

Texture EngineGetSwapChainImage(Engine *engine, u32 img_idx);
void EngineBeginRendering(Engine *engine, Texture target, Texture *depth, VkClearValue clear_color);
void EngineEndRendering(Engine *engine);

void EngineDrawModel(Engine *engine, HMM_Mat4 transform, Model model);

#endif //ENGINE_H
