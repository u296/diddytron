#include "sync.h"
#include "common.h"
#include "vulkan/vulkan_core.h"




bool make_sync_objects(VkDevice dev, VkSemaphore* sem1, VkSemaphore* sem2, VkFence* fen, struct Error* e_out) {
    VkSemaphoreCreateInfo sci = {};
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fci = {};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkResult r = VK_SUCCESS;
    r = vkCreateSemaphore(dev, &sci, NULL, sem1);
    VERIFY("syncobj", r)
    r = vkCreateSemaphore(dev, &sci, NULL, sem2);
    VERIFY("syncobj", r)
    r = vkCreateFence(dev, &fci, NULL, fen);
    VERIFY("syncobj", r)

    return false;

}

void destroy_sync_objects(void* obj) {
    struct SyncObjectCleanup* s = (struct SyncObjectCleanup*)obj;
    vkDestroySemaphore(s->dev, s->sem1, NULL);
    vkDestroySemaphore(s->dev, s->sem2, NULL);
    vkDestroyFence(s->dev, s->fen, NULL);
}