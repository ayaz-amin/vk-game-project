#ifndef VK_PIPELINE_H
#define VK_PIPELINE_H

#define MAX_SET_LAYOUTS 32

#include "types.hh"
#include <vulkan/vulkan.h>

struct PipelineLayout
{
    u32 set_layout_count;
    VkDescriptorSetLayout set_layouts[MAX_SET_LAYOUTS];
    VkPipelineLayout pipe_layout;
};

struct GraphicsPipelineCreateInfo
{
    PipelineLayout layout;
    const char *vertex_shader_path;
    const char *pixel_shader_path;
    u32 vertex_binding_count;
    VkVertexInputBindingDescription *vertex_bindings;
    u32 vertex_attribute_count;
    VkVertexInputAttributeDescription *vertex_attributes;
    VkFormat *target_format;
    VkFormat *depth_format;
};

struct Pipeline
{
    PipelineLayout layout;
    VkPipeline pipeline;
};

PipelineLayout CreatePipelineLayout(VkDevice device, VkPipelineLayoutCreateInfo *layout_info);
Pipeline CreateGraphicsPipeline(VkDevice device, GraphicsPipelineCreateInfo *pipeline_info);

#endif //VK_PIPELINE_H
