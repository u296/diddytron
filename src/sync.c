#include "sync.h"
#include "cleanupstack.h"
#include "common.h"
#include "vulkan/vulkan_core.h"


typedef struct SyncObjectCleanup {
    VkDevice dev;
    VkSemaphore sem1;
    VkSemaphore sem2;
    VkFence fen;
} SyncObjectCleanup;

typedef struct SemaphoreCleanup {
    VkDevice dev;
    VkSemaphore sem;
} SemaphoreCleanup;

typedef struct FenceCleanup {
    VkDevice dev;
    VkFence fen;
} FenceCleanup;

void destroy_semaphore(void* obj) {
    SemaphoreCleanup* s = (SemaphoreCleanup*) obj;
    vkDestroySemaphore(s->dev,s->sem, NULL);
}

void destroy_fence(void* obj) {
    FenceCleanup* f = (FenceCleanup*)obj;
    vkDestroyFence(f->dev, f->fen, NULL);
}

void destroy_sync_objects(void* obj) {
    struct SyncObjectCleanup* s = (struct SyncObjectCleanup*)obj;
    vkDestroySemaphore(s->dev, s->sem1, NULL);
    vkDestroySemaphore(s->dev, s->sem2, NULL);
    vkDestroyFence(s->dev, s->fen, NULL);
}

bool make_sync_objects(VkDevice dev, VkSemaphore* sem1, VkSemaphore* sem2, VkFence* fen, struct Error* e_out, CleanupStack*cs) {
    VkSemaphoreCreateInfo sci = {};
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fci = {};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkResult r = VK_ERROR_UNKNOWN;
    r = vkCreateSemaphore(dev, &sci, NULL, sem1);

    CLEANUP_START(SemaphoreCleanup)
    {dev,*sem1}
    CLEANUP_END(semaphore)
    VERIFY("syncobj", r)
    r = vkCreateSemaphore(dev, &sci, NULL, sem2);
    CLEANUP_START(SemaphoreCleanup)
    {dev,*sem2}
    CLEANUP_END(semaphore)
    VERIFY("syncobj", r)
    r = vkCreateFence(dev, &fci, NULL, fen);
    CLEANUP_START(FenceCleanup)
    {dev,*fen}
    CLEANUP_END(fence)
    VERIFY("syncobj", r)

    return false;

}

