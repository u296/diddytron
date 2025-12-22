#ifndef FRAMEBUFFERS_H
#define FRAMEBUFFERS_H
#include "common.h"
#include "vulkan/vulkan_core.h"



struct FramebuffersCleanup {
    VkDevice dev;
    VkFramebuffer* framebuffers;
    u32 n_framebuffers;
};

bool make_framebuffers(VkDevice dev, VkExtent2D swapchainextent, u32 n_swapchain_img, VkImageView* swapchain_imgviews, VkRenderPass renderpass, VkFramebuffer** framebuffers, struct Error* e_out);

void destroy_framebuffers(void* obj);


#endif