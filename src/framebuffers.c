#include "framebuffers.h"
#include "common.h"
#include "vulkan/vulkan_core.h"
#include<stdlib.h>





bool make_framebuffers(VkDevice dev, VkExtent2D swapchainextent, u32 n_swapchain_img, VkImageView* swapchain_imgviews, VkRenderPass renderpass, VkFramebuffer** framebuffers, struct Error* e_out) {
    *framebuffers = malloc(n_swapchain_img * sizeof(VkFramebuffer));
    for (u32 i = 0; i < n_swapchain_img; i++) {
        VkImageView attachments[] = {
            swapchain_imgviews[i]
        };

        VkFramebufferCreateInfo fci = {};
        fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fci.renderPass = renderpass;
        fci.attachmentCount = 1;
        fci.pAttachments = attachments;
        fci.width = swapchainextent.width;
        fci.height = swapchainextent.height;
        fci.layers = 1;

        VkResult r = vkCreateFramebuffer(dev, &fci, NULL, &(*framebuffers)[i]);
        VERIFY("framebuffers", r)
    }

    return false;
}

void destroy_framebuffers(void* obj) {
    struct FramebuffersCleanup* f = (struct FramebuffersCleanup*)obj;
    for (u32 i = 0; i < f->n_framebuffers; i++) {
        vkDestroyFramebuffer(f->dev, f->framebuffers[i], NULL);
    }
    free(f->framebuffers);
}