#ifndef DEVICE_H
#define DEVICE_H
#include "common.h"
#include "volk.h"
#include "vulkan/vulkan_core.h"

struct Queues {
	VkQueue graphics_queue;
	u32 i_graphics_queue_fam;

	VkQueue present_queue;
	u32 i_present_queue_fam;
};

struct SurfaceCapabilities{
    VkSurfaceCapabilitiesKHR surfcap;

};

bool make_device(VkInstance instance, VkSurfaceKHR surf, VkPhysicalDevice* physdev, VkDevice* device, struct Queues* queues, struct Error* e_out);

#endif