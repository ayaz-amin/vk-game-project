#include <windows.h>

#include "vk_utils.hh"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

VkInstance CreateInstance(void)
{
    VkInstance instance;

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.apiVersion = VK_API_VERSION_1_3;
    
    const char *instance_enabled_extensions[3] = {
        "VK_KHR_surface",
        "VK_KHR_win32_surface",
        "VK_EXT_debug_utils"
    };

    const char *instance_enabled_layers[1] = {
        "VK_LAYER_KHRONOS_validation"
    };
    
    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pApplicationInfo = &app_info;
    inst_info.enabledExtensionCount = 3;
    inst_info.ppEnabledExtensionNames = instance_enabled_extensions;
    inst_info.enabledLayerCount = 1;
    inst_info.ppEnabledLayerNames = instance_enabled_layers;

    vkCreateInstance(&inst_info, 0, &instance);
    return instance;
}

VkSurfaceKHR CreateSurface(VkInstance instance, HWND window)
{
    VkSurfaceKHR surface;
    
    VkWin32SurfaceCreateInfoKHR surf_info = {};
    surf_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surf_info.hwnd = window;

    vkCreateWin32SurfaceKHR(instance, &surf_info, 0, &surface);
    return surface;
}

Device CreateDevice(VkInstance instance, VkSurfaceKHR surface)
{
    Device device = {};
    
    uint32_t adapter_count = 0;
    VkPhysicalDevice adapters[16];
    vkEnumeratePhysicalDevices(instance, &adapter_count, 0);
    vkEnumeratePhysicalDevices(instance, &adapter_count, adapters);

    for(int i = 0; i < adapter_count; i++)
    {
        VkPhysicalDevice adapter = adapters[i];
        uint32_t queue_fam_prop_count = 0;
        VkQueueFamilyProperties queue_fam_props[16];
        vkGetPhysicalDeviceQueueFamilyProperties(adapter, &queue_fam_prop_count, 0);
        vkGetPhysicalDeviceQueueFamilyProperties(adapter, &queue_fam_prop_count, queue_fam_props);
        
        bool found_adapter = false;
        for(int j = 0; j < queue_fam_prop_count; j++)
        {
            VkQueueFamilyProperties queue_fam_prop = queue_fam_props[j];
            if((queue_fam_prop.queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                VkBool32 supported = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(adapter, j, surface, &supported);
                if(supported)
                {
                    device.queue_family_index = j;
                    found_adapter = true;
                    break;
                }
            }
        }

        if(found_adapter)
        {
            device.adapter = adapter;
            break;
        }
    }

    float queue_priority[1] = {1.0f};
    VkDeviceQueueCreateInfo queue_info = {};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = queue_priority;
    queue_info.queueFamilyIndex = device.queue_family_index;

    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering = {};
    dynamic_rendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamic_rendering.dynamicRendering = VK_TRUE;
    
    const char *device_enabled_extension[1] = {"VK_KHR_swapchain"};
    VkDeviceCreateInfo dev_info = {};
    dev_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dev_info.pNext = &dynamic_rendering;
    dev_info.enabledExtensionCount = 1;
    dev_info.ppEnabledExtensionNames = device_enabled_extension;
    dev_info.queueCreateInfoCount = 1;
    dev_info.pQueueCreateInfos = &queue_info;
    
    vkCreateDevice(device.adapter, &dev_info, 0, &device.device);
    vkGetDeviceQueue(device.device, device.queue_family_index, 0, &device.queue);

    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;
    allocator_info.instance = instance;
    allocator_info.physicalDevice = device.adapter;
    allocator_info.device = device.device;

    vmaCreateAllocator(&allocator_info, &device.allocator);
    
    return device;
}

SwapChain CreateSwapChain(Device device, VkSurfaceKHR surface)
{
    SwapChain swapchain = {};
    
    VkSurfaceCapabilitiesKHR surf_caps = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.adapter, surface, &surf_caps);
    uint32_t img_count = surf_caps.minImageCount + 1;
    if((img_count > surf_caps.maxImageCount) && (surf_caps.maxImageCount > 0))
    {
        img_count = surf_caps.maxImageCount;
    }

    swapchain.render_area = {};
    swapchain.render_area.extent = surf_caps.currentExtent;

    uint32_t surf_format_count = 0;
    VkSurfaceFormatKHR surf_formats[64];
    vkGetPhysicalDeviceSurfaceFormatsKHR(device.adapter, surface, &surf_format_count, 0);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device.adapter, surface, &surf_format_count, surf_formats);
    swapchain.swap_format = surf_formats[0].format;
    
    VkSwapchainCreateInfoKHR swap_info = {};
    swap_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap_info.surface = surface;
    swap_info.minImageCount = img_count;
    swap_info.imageFormat = surf_formats[0].format;
    swap_info.imageColorSpace = surf_formats[0].colorSpace;
    swap_info.imageExtent = surf_caps.currentExtent;
    swap_info.imageArrayLayers = 1;
    swap_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swap_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swap_info.preTransform = surf_caps.currentTransform;
    swap_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swap_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swap_info.clipped = VK_TRUE;
    
    vkCreateSwapchainKHR(device.device, &swap_info, 0, &swapchain.swapchain);

    uint32_t swap_image_count = 0;
    vkGetSwapchainImagesKHR(device.device, swapchain.swapchain, &swap_image_count, 0);
    vkGetSwapchainImagesKHR(device.device, swapchain.swapchain, &swap_image_count, swapchain.swap_images);

    VkImageViewCreateInfo img_view_info = {};
    img_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    img_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    img_view_info.format = surf_formats[0].format;
    img_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    img_view_info.subresourceRange.levelCount = 1;
    img_view_info.subresourceRange.layerCount = 1;

    for(int i = 0; i < swap_image_count; i++)
    {
        img_view_info.image = swapchain.swap_images[i];
        vkCreateImageView(device.device, &img_view_info, 0, &swapchain.swap_views[i]);
    }
    
    return swapchain;
}

Command CreateCommand(Device device)
{
    Command command = {};

    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = device.queue_family_index;
    
    vkCreateCommandPool(device.device, &pool_info, 0, &command.pool);

    VkCommandBufferAllocateInfo cmd_info = {};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_info.commandPool = command.pool;
    cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_info.commandBufferCount = MAX_FRAMES;

    vkAllocateCommandBuffers(device.device, &cmd_info, command.cmds);

    return command;
}

SyncStructs CreateSyncStructs(Device device)
{
    SyncStructs sync;
    
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo sema_info = {};
    sema_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for(int i = 0; i < MAX_FRAMES; i++)
    {
        vkCreateFence(device.device, &fence_info, 0, &sync.fences[i]);
        vkCreateSemaphore(device.device, &sema_info, 0, &sync.acq_semas[i]);
        vkCreateSemaphore(device.device, &sema_info, 0, &sync.pres_semas[i]);
    }

    return sync;
}

struct DDSPixelFormat
{
    u32 size;
    u32 flags;
    u32 four_cc;
    u32 rgb_bit_count;
    u32 r_bit_mask;
    u32 g_bit_mask;
    u32 b_bit_mask;
    u32 a_bit_mask;
};

struct DDSHeader
{
    u32 size;
    u32 flags;
    u32 height;
    u32 width;
    u32 pitch_or_linear_size;
    u32 depth;
    u32 mip_map_count;
    u32 reserved1[11];
    DDSPixelFormat ddspf;
    u32 caps;
    u32 caps2;
    u32 caps3;
    u32 caps4;
    u32 reserved;
};

struct DDSFile
{
    char magic[4];
    DDSHeader header;
    char data_begin;
};

Texture CreateTexture(Device device, VkFormat format,
                      VkImageUsageFlags usage, u32 width,
                      u32 height, u32 mip_count)
{
    Texture texture = {};
    
    texture.rect.extent.width = width;
    texture.rect.extent.height = height;
    texture.mip_count = mip_count;
    
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = format;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = mip_count;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = usage;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocation_info = {};
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    
    vmaCreateImage(device.allocator, &image_info, &allocation_info, &texture.image, &texture.alloc, 0);

    bool is_depth = usage == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VkImageAspectFlags aspect_mask = is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = texture.image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = image_info.format;
    view_info.subresourceRange.aspectMask = aspect_mask;
    view_info.subresourceRange.levelCount = mip_count;
    view_info.subresourceRange.layerCount = 1;

    vkCreateImageView(device.device, &view_info, 0, &texture.view);
    return texture;
}

Texture LoadTextreFromDDS(Device device, Command command, const char *file_path)
{
    HANDLE hfile = CreateFile(file_path, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
    LARGE_INTEGER fsize; GetFileSizeEx(hfile, &fsize);
    HANDLE hmap = CreateFileMapping(hfile, 0, PAGE_READONLY, 0, 0, 0);
    DDSFile *buffer = (DDSFile *)MapViewOfFile(hmap, FILE_MAP_READ, 0, 0, fsize.QuadPart);

    VkDeviceSize buffer_size = 0;
    int w = buffer->header.width;
    int h = buffer->header.height;
    int block_size = 8;
    for(int i = 0; i < buffer->header.mip_map_count; i++)
    {
        int size = ((w + 3) / 4) * ((h + 3) / 4) * block_size;
        buffer_size += size;
        w /= 2; h /= 2;
    }

    VkBufferCreateInfo staging_info = {};
    staging_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    staging_info.size = buffer_size;
    staging_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    staging_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo staging_alloc_info = {};
    staging_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    staging_alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    VkBuffer staging;
    VmaAllocation staging_alloc;
    vmaCreateBuffer(device.allocator, &staging_info,
                    &staging_alloc_info, &staging,
                    &staging_alloc, 0);

    void *staging_data;
    vmaMapMemory(device.allocator, staging_alloc, &staging_data);
    memcpy(staging_data, &buffer->data_begin, buffer_size);
    vmaUnmapMemory(device.allocator, staging_alloc);
        
    VkFormat tex_format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    VkImageUsageFlags tex_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    u32 width = buffer->header.width;
    u32 height = buffer->header.height;
    u32 mip_count = buffer->header.mip_map_count;

    Texture texture = CreateTexture(device, tex_format, tex_usage, width, height, mip_count);
    
    VkCommandBufferBeginInfo begin = {};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    vkBeginCommandBuffer(command.cmds[0], &begin);

    TransitionImageInfo trans_info = {};
    trans_info.image = texture.image;
    trans_info.old_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    trans_info.new_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    trans_info.dst_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
    trans_info.aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    trans_info.src_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    trans_info.dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    trans_info.mip_count = mip_count;

    TransitionImage(command.cmds[0], &trans_info);

    int offset = 0;
    w = buffer->header.width;
    h = buffer->header.height;
    for(int i = 0; i < mip_count; i++)
    {
        int size = ((w + 3) / 4) * ((h + 3) / 4) * block_size;

        VkBufferImageCopy region = {};
        region.bufferOffset = offset;
        region.imageExtent.width = w;
        region.imageExtent.height = h;
        region.imageExtent.depth = 1;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = i;
        region.imageSubresource.layerCount = 1;

        offset += size;

        vkCmdCopyBufferToImage(command.cmds[0], staging, texture.image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        w /= 2; h /= 2;
    }

    trans_info.old_layout = trans_info.new_layout;
    trans_info.new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    trans_info.src_access_mask = trans_info.dst_access_mask;
    trans_info.dst_access_mask = VK_ACCESS_SHADER_READ_BIT;
    trans_info.src_stage_mask = trans_info.dst_stage_mask;
    trans_info.dst_stage_mask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    trans_info.mip_count = mip_count;

    TransitionImage(command.cmds[0], &trans_info);
    
    vkEndCommandBuffer(command.cmds[0]);

    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command.cmds[0];

    vkQueueSubmit(device.queue, 1, &submit, 0);
    vkQueueWaitIdle(device.queue);
    vmaDestroyBuffer(device.allocator, staging, staging_alloc);
    
    return texture;
}

void TransitionImage(VkCommandBuffer cmd, TransitionImageInfo *transition)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = transition->src_access_mask;
    barrier.dstAccessMask = transition->dst_access_mask;
    barrier.oldLayout = transition->old_layout;
    barrier.newLayout = transition->new_layout;
    barrier.image = transition->image;
    barrier.subresourceRange.aspectMask = transition->aspect_mask;
    barrier.subresourceRange.levelCount = transition->mip_count;
    barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(cmd, transition->src_stage_mask,
                         transition->dst_stage_mask,
                         0, 0, 0, 0, 0, 1, &barrier);
}
