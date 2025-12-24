#ifndef COMMANDPOOL_H
#define COMMANDPOOL_H
#include "cleanupstack.h"
#include "common.h"
#include "device.h"



bool make_commandpool(VkDevice dev, struct Queues queues, VkCommandPool* pool, struct Error* e_out, CleanupStack*cs);



bool make_commandbuffers(VkDevice dev, VkCommandPool pool, VkCommandBuffer* cmdbuf, struct Error* e_out);

bool recordcommandbuffer(VkExtent2D swapchainextent, VkFramebuffer fb, VkCommandBuffer cmdbuf, VkRenderPass renderpass, VkPipeline pipeline, struct Error* e_out);


#endif