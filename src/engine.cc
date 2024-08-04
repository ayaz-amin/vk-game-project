#include "engine.hh"
#include "vk_utils.hh"
#include "vk_pipeline.hh"
#include "vulkan/vulkan_core.h"

#include "third_party/HandmadeMath.h"

void CreateDefaultPipeline(Engine *engine, const char *vs_path, const char *fs_path,
                           VkFormat *target_format, VkFormat *depth_format)
{
    VkDevice device = engine->device.device;
    
    VkDescriptorSetLayoutBinding sampler_binding = {};
    sampler_binding.binding = 0;
    sampler_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_binding.descriptorCount = 1;
    sampler_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo ds_layout_info = {};
    ds_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_info.bindingCount = 1;
    ds_layout_info.pBindings = &sampler_binding;
    
    VkDescriptorSetLayout ds_layout;
    vkCreateDescriptorSetLayout(device, &ds_layout_info, 0, &ds_layout);

    VkPushConstantRange push_constant = {};;
    push_constant.size = sizeof(HMM_Mat4) * 2;
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &ds_layout;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push_constant;
    
    PipelineLayout layout = CreatePipelineLayout(device, &layout_info);

    VkVertexInputBindingDescription binding = {};
    binding.binding = 0;
    binding.stride = 16;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attr[3] = {};
    attr[0].location = 0;
    attr[0].binding = 0;
    attr[0].format = VK_FORMAT_R16G16B16A16_SFLOAT;
    attr[0].offset = 0;

    attr[1].location = 1;
    attr[1].binding = 0;
    attr[1].format = VK_FORMAT_R8G8B8A8_SNORM;
    attr[1].offset = 8;
    
    attr[2].location = 2;
    attr[2].binding = 0;
    attr[2].format = VK_FORMAT_R16G16_SFLOAT;
    attr[2].offset = 12;

    GraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.layout = layout;
    pipeline_info.vertex_shader_path = "compiled/mesh.vert.spv";
    pipeline_info.pixel_shader_path = "compiled/mesh.frag.spv";
    pipeline_info.vertex_binding_count = 1;
    pipeline_info.vertex_bindings = &binding;
    pipeline_info.vertex_attribute_count = 3;
    pipeline_info.vertex_attributes = attr;
    pipeline_info.target_format = target_format;
    pipeline_info.depth_format = depth_format;
    
    engine->mesh_pipeline = CreateGraphicsPipeline(device, &pipeline_info);
}

Engine CreateEngine(HWND window)
{
    Engine engine = {0};
    engine.instance = CreateInstance();
    engine.surface = CreateSurface(engine.instance, window);
    engine.device = CreateDevice(engine.instance, engine.surface);
    engine.swapchain = CreateSwapChain(engine.device, engine.surface);

    VkFormat depth_format = VK_FORMAT_D16_UNORM_S8_UINT;
    VkImageUsageFlags depth_usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    u32 width = engine.swapchain.render_area.extent.width;
    u32 height = engine.swapchain.render_area.extent.height;
    
    engine.depth = CreateTexture(engine.device, depth_format, depth_usage, width, height, 1);
    engine.depth_format = depth_format;
    
    engine.sync = CreateSyncStructs(engine.device);
    engine.command = CreateCommand(engine.device);

    CreateDefaultPipeline(&engine,
                          "compiled/mesh.vert.spv",
                          "compiled/mesh.frag.spv",
                          &engine.swapchain.swap_format,
                          &engine.depth_format);
    
    VkDescriptorPoolSize pool_size = {};
    pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_size.descriptorCount = 2;
    
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;

    vkCreateDescriptorPool(engine.device.device, &pool_info, 0, &engine.mesh_pool);
    return engine;
}

struct CompiledMDL
{
    u32 vertex_size;
    u32 index_size;
    char data_begin;
};

Model EngineLoadCompiledModel(Engine *engine, const char *file_path)
{
    Model model = {};
    
    Device device = engine->device;
    Command command = engine->command;
    VmaAllocator allocator = engine->device.allocator;
    
    u32 set_layout_count = engine->mesh_pipeline.layout.set_layout_count;
    VkDescriptorSetLayout *set_layouts = engine->mesh_pipeline.layout.set_layouts;

    HANDLE hfile = CreateFile(file_path, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    LARGE_INTEGER fsize; GetFileSizeEx(hfile, &fsize);
    HANDLE hmap = CreateFileMapping(hfile, 0, PAGE_READONLY, 0, 0, 0);
    CompiledMDL *buffer = (CompiledMDL *)MapViewOfFile(hmap, FILE_MAP_READ, 0, 0, fsize.QuadPart);
    
    u32 vertex_size = buffer->vertex_size;
    u32 index_size = buffer->index_size;
    model.num_indices = index_size / sizeof(u32);
    
    char *vertex_data = &buffer->data_begin;
    char *index_data = vertex_data + vertex_size;
    
    VkBufferCreateInfo buff_info = {};
    buff_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buff_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buff_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buff_info.size = vertex_size;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    alloc_info.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    vmaCreateBuffer(allocator, &buff_info, &alloc_info, &model.vbo, &model.vbo_alloc, 0);
    
    void *dst_data;
    vmaMapMemory(allocator, model.vbo_alloc, &dst_data);
    memcpy(dst_data, vertex_data, vertex_size);

    buff_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    buff_info.size = index_size;

    vmaCreateBuffer(allocator, &buff_info, &alloc_info, &model.ibo, &model.ibo_alloc, 0);
    vmaMapMemory(allocator, model.ibo_alloc, &dst_data);
    memcpy(dst_data, index_data, index_size);
    
    model.texture = LoadTextreFromDDS(device, command, "image.dds");

    VkDescriptorSetAllocateInfo set_alloc_info = {};
    set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_alloc_info.descriptorPool = engine->mesh_pool;
    set_alloc_info.descriptorSetCount = set_layout_count;
    set_alloc_info.pSetLayouts = set_layouts;
    
    vkAllocateDescriptorSets(device.device, &set_alloc_info, &model.set);

    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.maxLod = model.texture.mip_count;
   
    vkCreateSampler(device.device, &sampler_info, 0, &model.tex_sampler);
    
    VkDescriptorImageInfo img_info = {};
    img_info.sampler = model.tex_sampler;
    img_info.imageView = model.texture.view;
    img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = model.set;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &img_info;

    vkUpdateDescriptorSets(device.device, 1, &write, 0, 0);
    
    model.model_matrix = HMM_M4D(1.f);
    
    return model;
}

u32 EngineBegin(Engine *engine)
{
    VkDevice device = engine->device.device;
    VkSwapchainKHR swapchain = engine->swapchain.swapchain;
    VkFence fence = engine->sync.fences[engine->frame_idx];
    VkSemaphore wait_sema = engine->sync.acq_semas[engine->frame_idx];
    VkCommandBuffer cmd = engine->command.cmds[engine->frame_idx];
    
    vkWaitForFences(device, 1, &fence, 0, UINT64_MAX);
    vkResetFences(device, 1, &fence);

    uint32_t img_idx;
    vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, wait_sema, 0, &img_idx);

    VkCommandBufferBeginInfo begin = {};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);

    VkImage swap_image = engine->swapchain.swap_images[img_idx];

    TransitionImageInfo trans_info = {};
    trans_info.image = swap_image;
    trans_info.old_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    trans_info.new_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    trans_info.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    trans_info.aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    trans_info.src_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    trans_info.dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    trans_info.mip_count = 1;
    
    TransitionImage(cmd, &trans_info);
    
    return img_idx;
}

void EngineEnd(Engine *engine, uint32_t img_idx)
{
    VkQueue queue = engine->device.queue;
    VkSwapchainKHR swapchain = engine->swapchain.swapchain;
    VkImage swap_image = engine->swapchain.swap_images[img_idx];
    VkFence fence = engine->sync.fences[engine->frame_idx];
    VkSemaphore wait_sema = engine->sync.acq_semas[engine->frame_idx];
    VkSemaphore sign_sema = engine->sync.pres_semas[engine->frame_idx];
    VkCommandBuffer cmd = engine->command.cmds[engine->frame_idx];
    
    TransitionImageInfo trans_info = {};
    trans_info.image = swap_image;
    trans_info.old_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    trans_info.new_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    trans_info.src_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    trans_info.aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    trans_info.src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    trans_info.dst_stage_mask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    trans_info.mip_count = 1;
    
    TransitionImage(cmd, &trans_info);
    
    vkEndCommandBuffer(cmd);
    
    VkPipelineStageFlags wait_dst_stage[1] = {VK_PIPELINE_STAGE_TRANSFER_BIT};
    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &wait_sema;
    submit.pWaitDstStageMask = wait_dst_stage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &sign_sema;
    
    vkQueueSubmit(queue, 1, &submit, fence);

    VkPresentInfoKHR pres_info = {};
    pres_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pres_info.waitSemaphoreCount = 1;
    pres_info.pWaitSemaphores = &sign_sema;
    pres_info.swapchainCount = 1;
    pres_info.pSwapchains = &swapchain;
    pres_info.pImageIndices = &img_idx;
    vkQueuePresentKHR(queue, &pres_info);

    engine->frame_idx = (engine->frame_idx + 1) % MAX_FRAMES;
}

Texture EngineGetSwapChainImage(Engine *engine, u32 img_idx)
{
    Texture swap_texture = {};
    swap_texture.image = engine->swapchain.swap_images[img_idx];
    swap_texture.view = engine->swapchain.swap_views[img_idx];
    swap_texture.rect = engine->swapchain.render_area;
    return swap_texture;
}

void EngineBeginRendering(Engine *engine, Texture target, Texture *depth, VkClearValue clear_color)
{
    u32 frame_idx = engine->frame_idx;
    VkCommandBuffer cmd = engine->command.cmds[frame_idx];
    
    VkRenderingAttachmentInfo color_attachment = {};
    color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color_attachment.imageView = target.view;
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.clearValue = clear_color;

    VkRenderingAttachmentInfo *depth_attachment = 0;
    VkRenderingAttachmentInfo _depth_attachment;
    if(depth)
    {
        _depth_attachment = {};
        _depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        _depth_attachment.imageView = depth->view;
        _depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        _depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        _depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment = &_depth_attachment;
    }

    VkRenderingInfo render_info = {};
    render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    render_info.renderArea = target.rect;
    render_info.layerCount = 1;
    render_info.colorAttachmentCount = 1;
    render_info.pColorAttachments = &color_attachment;
    render_info.pDepthAttachment = depth_attachment;

    vkCmdBeginRendering(cmd, &render_info);
}

void EngineEndRendering(Engine *engine)
{
    u32 frame_idx = engine->frame_idx;
    VkCommandBuffer cmd = engine->command.cmds[frame_idx];
    vkCmdEndRendering(cmd);
}

void EngineDrawModel(Engine *engine, HMM_Mat4 transform, Model model)
{
    VkCommandBuffer cmd = engine->command.cmds[engine->frame_idx];
    VkPipeline mesh_pipeline = engine->mesh_pipeline.pipeline;
    VkPipelineLayout mesh_layout = engine->mesh_pipeline.layout.pipe_layout;

    VkViewport viewport = {};
    viewport.width = engine->swapchain.render_area.extent.width;
    viewport.height = engine->swapchain.render_area.extent.height;
    viewport.maxDepth = 1.0f;
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_pipeline);
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &engine->swapchain.render_area);

    HMM_Mat4 push_constants[2] = {model.model_matrix, transform};
    vkCmdPushConstants(cmd, mesh_layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(push_constants), push_constants);
    
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_layout,
                            0, 1, &model.set, 0, 0);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &model.vbo, &offset);
    vkCmdBindIndexBuffer(cmd, model.ibo, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, model.num_indices, 1, 0, 0, 0);
}
