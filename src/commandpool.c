#include "commandpool.h"
#include "common.h"
#include "device.h"
#include "vulkan/vulkan_core.h"

bool make_commandpool(VkDevice dev, struct Queues queues, VkCommandPool* pool, struct Error* e_out) {
    VkCommandPoolCreateInfo cpi = {};
    cpi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpi.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cpi.queueFamilyIndex = queues.i_graphics_queue_fam;

    VkResult r = vkCreateCommandPool(dev, &cpi, NULL, pool);
    VERIFY("commandpool", r)
    return false;
}

void destroy_commandpool(void* obj) {
    struct CommandpoolCleanup* cc = (struct CommandpoolCleanup*)obj;
    vkDestroyCommandPool(cc->dev, cc->pool, NULL);
}

bool make_commandbuffers(VkDevice dev, VkCommandPool pool, VkCommandBuffer* cmdbuf, struct Error* e_out) {
    VkCommandBufferAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool = pool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;

    VkResult r = vkAllocateCommandBuffers(dev, &ai, cmdbuf);
    VERIFY("cmdbuf", r)

    return false;

}

bool recordcommandbuffer(VkExtent2D swapchainextent, VkFramebuffer fb, VkCommandBuffer cmdbuf, VkRenderPass renderpass, VkPipeline pipeline, struct Error* e_out) {

    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = 0;
    cbbi.pInheritanceInfo = NULL;


    VkRenderPassBeginInfo rpbi = {};
    rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.renderPass = renderpass;
    rpbi.framebuffer = fb;
    rpbi.renderArea.offset = (struct VkOffset2D){0,0};
    rpbi.renderArea.extent = swapchainextent;

    VkClearValue clearcol = {};
    clearcol.color.float32[0] = 0.0f;
    clearcol.color.float32[1] = 0.0f;
    clearcol.color.float32[2] = 0.0f;
    clearcol.color.float32[3] = 1.0f;

    rpbi.clearValueCount = 1;
    rpbi.pClearValues = &clearcol;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = swapchainextent.width;
    viewport.height = swapchainextent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor = {};
    scissor.extent = swapchainextent;
    scissor.offset = (struct VkOffset2D){0,0};

    vkBeginCommandBuffer(cmdbuf, &cbbi);


    vkCmdBeginRenderPass(cmdbuf, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdbuf, 0, 1, &scissor);
    vkCmdDraw(cmdbuf, 3, 1, 0, 0);
    vkCmdEndRenderPass(cmdbuf);

    VkResult r = vkEndCommandBuffer(cmdbuf);
    VERIFY("recording", r)

    return false;
}
