#include <windows.h>
#include "vk_pipeline.hh"

PipelineLayout CreatePipelineLayout(VkDevice device, VkPipelineLayoutCreateInfo *layout_info)
{
    PipelineLayout layout = {};
    layout.set_layout_count = layout_info->setLayoutCount;
    memcpy(layout.set_layouts,
           layout_info->pSetLayouts,
           layout_info->setLayoutCount *
           sizeof(VkDescriptorSetLayout));

    vkCreatePipelineLayout(device, layout_info, 0, &layout.pipe_layout);
    return layout;
}

VkPipelineShaderStageCreateInfo CreateShaderStage(VkDevice device, const char *file_path, VkShaderStageFlagBits stage)
{
    VkShaderModule module;

    HANDLE hfile = CreateFile(file_path, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
    LARGE_INTEGER fsize; GetFileSizeEx(hfile, &fsize);
    HANDLE hmap = CreateFileMapping(hfile, 0, PAGE_READONLY, 0, 0, 0);
    void *buffer = MapViewOfFile(hmap, FILE_MAP_READ, 0, 0, fsize.QuadPart);
    
    VkShaderModuleCreateInfo module_info = {};
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_info.codeSize = fsize.QuadPart;
    module_info.pCode = (u32 *)buffer;
    vkCreateShaderModule(device, &module_info, 0, &module);
    
    VkPipelineShaderStageCreateInfo stage_info = {};
    stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_info.stage = stage;
    stage_info.module = module;
    stage_info.pName = "main";

    return stage_info;
}

VkPipelineVertexInputStateCreateInfo CreateVertexInputState(u32 vertex_binding_count, VkVertexInputBindingDescription *vertex_bindings,
                                                            u32 vertex_attribute_count, VkVertexInputAttributeDescription *vertex_attributes)
{
    VkPipelineVertexInputStateCreateInfo vi_state = {};
    vi_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi_state.vertexBindingDescriptionCount = vertex_binding_count;
    vi_state.pVertexBindingDescriptions = vertex_bindings;
    vi_state.vertexAttributeDescriptionCount = vertex_attribute_count;
    vi_state.pVertexAttributeDescriptions = vertex_attributes;
    return vi_state;
}

VkPipelineInputAssemblyStateCreateInfo CreateInputAssembly(void)
{
    VkPipelineInputAssemblyStateCreateInfo ia_state = {};
    ia_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    return ia_state;
}

VkPipelineViewportStateCreateInfo CreateViewportState(void)
{
    VkPipelineViewportStateCreateInfo vp_state = {};
    vp_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp_state.viewportCount = 1;
    vp_state.scissorCount = 1;
    return vp_state;
}

VkPipelineRasterizationStateCreateInfo CreateRasterState(void)
{
    VkPipelineRasterizationStateCreateInfo raster = {};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_BACK_BIT;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;
    return raster;
}

VkPipelineMultisampleStateCreateInfo CreateMultisampleState(void)
{
    VkPipelineMultisampleStateCreateInfo ms_state = {};
    ms_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    return ms_state;
}

VkPipelineDepthStencilStateCreateInfo CreateDepthStencilState(void)
{
    VkPipelineDepthStencilStateCreateInfo depth_state = {};
    depth_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_state.depthTestEnable = VK_TRUE;
    depth_state.depthWriteEnable = VK_TRUE;
    depth_state.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
    depth_state.maxDepthBounds = 1.0f;
    return depth_state;
}

Pipeline CreateGraphicsPipeline(VkDevice device, GraphicsPipelineCreateInfo *pipeline_info)
{
    Pipeline pipeline = {};
    pipeline.layout = pipeline_info->layout;

    VkPipelineRenderingCreateInfo rendering = {};
    rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachmentFormats = pipeline_info->target_format;
    if(pipeline_info->depth_format)
        rendering.depthAttachmentFormat = *pipeline_info->depth_format;
    
    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0] = CreateShaderStage(device, pipeline_info->vertex_shader_path, VK_SHADER_STAGE_VERTEX_BIT);
    stages[1] = CreateShaderStage(device, pipeline_info->pixel_shader_path, VK_SHADER_STAGE_FRAGMENT_BIT);

    VkPipelineVertexInputStateCreateInfo vi_state = CreateVertexInputState(
        pipeline_info->vertex_binding_count, pipeline_info->vertex_bindings,
        pipeline_info->vertex_attribute_count, pipeline_info->vertex_attributes);
    
    VkPipelineInputAssemblyStateCreateInfo ia_state = CreateInputAssembly();
    VkPipelineViewportStateCreateInfo vp_state = CreateViewportState();
    VkPipelineRasterizationStateCreateInfo raster = CreateRasterState();
    VkPipelineMultisampleStateCreateInfo ms_state = CreateMultisampleState();
    VkPipelineDepthStencilStateCreateInfo *depth_state = 0;
    VkPipelineDepthStencilStateCreateInfo _depth_state;
    if(pipeline_info->depth_format)
    {
        _depth_state = CreateDepthStencilState();
        depth_state = &_depth_state;
    }
    
    VkPipelineColorBlendAttachmentState color_attachment = {};
    color_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT 
                                    | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    
    VkPipelineColorBlendStateCreateInfo cb_state = {};
    cb_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb_state.attachmentCount = 1;
    cb_state.pAttachments = &color_attachment;
    
    VkDynamicState dynamic_states[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates = dynamic_states;
    
    VkGraphicsPipelineCreateInfo gp_info = {};
    gp_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gp_info.pNext = &rendering;
    gp_info.stageCount = 2;
    gp_info.pStages = stages;
    gp_info.pVertexInputState = &vi_state;
    gp_info.pInputAssemblyState = &ia_state;
    gp_info.pViewportState = &vp_state;
    gp_info.pRasterizationState = &raster;
    gp_info.pMultisampleState = &ms_state;
    gp_info.pDepthStencilState = depth_state;
    gp_info.pColorBlendState = &cb_state;
    gp_info.pDynamicState = &dynamic_state;
    gp_info.layout = pipeline_info->layout.pipe_layout;
    
    vkCreateGraphicsPipelines(device, 0, 1, &gp_info, 0, &pipeline.pipeline);
    return pipeline;
}
