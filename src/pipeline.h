#ifndef PIPELINE_H
#define PIPELINE_H
#include "common.h"

struct PipelineCleanup{
    VkDevice dev;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
};

bool make_graphicspipeline(VkDevice dev, VkExtent2D swapchainextent, VkRenderPass renderpass, VkPipelineLayout* pipeline_layout, VkPipeline* pipeline, struct Error* e_out);

void destroy_pipeline(void* obj);

#endif