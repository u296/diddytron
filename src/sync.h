#ifndef SYNC_H
#define SYNC_H
#include "common.h"
#include "vulkan/vulkan_core.h"

struct SyncObjectCleanup {
    VkDevice dev;
    VkSemaphore sem1;
    VkSemaphore sem2;
    VkFence fen;
};

bool make_sync_objects(VkDevice dev, VkSemaphore* sem1, VkSemaphore* sem2, VkFence* fen, struct Error* e_out);

void destroy_sync_objects(void* obj);

#endif