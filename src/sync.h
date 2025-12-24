#ifndef SYNC_H
#define SYNC_H
#include "cleanupstack.h"
#include "common.h"
#include "vulkan/vulkan_core.h"



bool make_sync_objects(VkDevice dev, VkSemaphore* sem1, VkSemaphore* sem2, VkFence* fen, struct Error* e_out, CleanupStack*cs);


#endif