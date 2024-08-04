#ifndef VK_UTILS_H
#define VK_UTILS_H

#define MAX_SWAP_IMAGE 16
#define MAX_FRAMES 2

#include <windows.h>
#include <vulkan/vulkan.h>

#include "types.hh"
#include "third_party/vk_mem_alloc.h"

struct Device
{
    VkPhysicalDevice adapter;
    VkDevice device;
    VkQueue queue;
    uint32_t queue_family_index;
    VmaAllocator allocator;
};

struct SwapChain
{
    VkSwapchainKHR swapchain;
    VkImage swap_images[MAX_SWAP_IMAGE];
    VkImageView swap_views[MAX_SWAP_IMAGE];
    VkRect2D render_area;
    VkFormat swap_format;
};

struct Command
{
    VkCommandPool pool;
    VkCommandBuffer cmds[MAX_FRAMES];
};

struct SyncStructs
{
    VkFence fences[MAX_FRAMES];
    VkSemaphore acq_semas[MAX_FRAMES];
    VkSemaphore pres_semas[MAX_FRAMES];
};

struct Texture
{
    VkImage image;
    VkImageView view;
    VkRect2D rect;
    VmaAllocation alloc;
    u8 mip_count;
};

struct TransitionImageInfo
{
    VkImage image;
    VkImageLayout old_layout;
    VkImageLayout new_layout;
    VkAccessFlags src_access_mask;
    VkAccessFlags dst_access_mask;
    VkImageAspectFlags aspect_mask;
    VkPipelineStageFlags src_stage_mask;
    VkPipelineStageFlags dst_stage_mask;
    u32 mip_count;
};

VkInstance CreateInstance(void);
VkSurfaceKHR CreateSurface(VkInstance instance, HWND window);
Device CreateDevice(VkInstance instance, VkSurfaceKHR surface);
SwapChain CreateSwapChain(Device device, VkSurfaceKHR surface);
Command CreateCommand(Device device);
SyncStructs CreateSyncStructs(Device device);

Texture CreateTexture(Device device, VkFormat format,
                      VkImageUsageFlags usage, u32 width,
                      u32 height, u32 mip_count);

Texture LoadTextreFromDDS(Device device, Command command, const char *file_path);

void TransitionImage(VkCommandBuffer cmd, TransitionImageInfo *transition_info);

#endif //VK_UTILS_H
