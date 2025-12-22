#ifndef RENDERPASS_H
#define RENDERPASS_H
#include "common.h"

struct RenderPassCleanup {
    VkDevice dev;
    VkRenderPass renderpass;
};

bool make_renderpass(VkDevice dev, VkFormat swapchainformat, VkRenderPass* renderpass, struct Error* e_out);

void destroy_renderpass(void*obj);

#endif