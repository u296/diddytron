
#include <string.h>
#include<volk.h>
#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<GLFW/glfw3.h>


#include "commandpool.h"
#include "common.h"
#include "cleanupstack.h"
#include "instance.h"
#include "device.h"
#include "swapchain.h"
#include "renderpass.h"
#include "pipeline.h"
#include "framebuffers.h"
#include "sync.h"
#include "vulkan/vulkan_core.h" // having this here doesn't hurt and  prevents intellisense from adding it at the top which would break compilation


#define N_IMAGE 3







const usize WIDTH = 800;
const usize HEIGHT = 600;


bool make_window(GLFWwindow** window) {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	*window = glfwCreateWindow(WIDTH, HEIGHT, "diddytron", NULL, NULL);

	return false;
}

void destroy_window(void* obj) {
	GLFWwindow* window = (GLFWwindow*)obj;
	glfwDestroyWindow(window);
	glfwTerminate();
}




#define MAINCHECK    if(f) {\
        printf("Error, code=%d: %s\n",e.code,e.origin);\
        goto fail; \
    }



int main() {

	struct CleanupStack cs = {};
	cs_init(&cs);

	volkInitialize();
    bool f = false;    

    struct Error e = {.code=0,.origin=""};

	GLFWwindow* my_window;
    VkInstance my_instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkSurfaceKHR my_surf;
	VkPhysicalDevice my_physdev;
	VkDevice my_device;
	struct Queues my_queues = {};
	VkSwapchainKHR my_swapchain;
	VkFormat swapchain_format;
	VkExtent2D swapchain_extent;
	u32 n_swapchain_images = 0;
	VkImage* swapchain_images;
	VkImageView* my_imageviews;
	VkRenderPass my_renderpass;
	VkPipelineLayout my_pipelinelayout;
	VkPipeline my_pipeline;
	VkFramebuffer* my_framebuffers;
	VkCommandPool my_pool;
	VkCommandBuffer my_commandbuf;
	VkSemaphore sem_imgready, sem_rendfinish;
	VkFence fen_inflight;



	make_window(&my_window);
	cs_push(&cs, &my_window, sizeof(my_window), destroy_window);

    f = make_instance(&my_instance, &e, &cs);
	MAINCHECK
	

	volkLoadInstance(my_instance);

	f = make_debugger(my_instance, &debug_messenger, &e, &cs);
	MAINCHECK
	

	f = make_surface(my_instance, my_window, &my_surf, &e, &cs);
	MAINCHECK
	

	f = make_device(my_instance, my_surf, &my_physdev, &my_device, &my_queues, &e, &cs);
	MAINCHECK
	

	volkLoadDevice(my_device);

	f = make_swapchain(my_physdev, my_device, my_queues, my_surf, my_window, &my_swapchain, &swapchain_format, &swapchain_extent,&n_swapchain_images,&swapchain_images, &e, &cs);
	MAINCHECK

	f = make_swapchain_imageviews(my_device, n_swapchain_images, swapchain_images, swapchain_format, &my_imageviews, &e, &cs);
	MAINCHECK

	f = make_renderpass(my_device, swapchain_format, &my_renderpass, &e, &cs);
	MAINCHECK

	f = make_graphicspipeline(my_device, swapchain_extent,my_renderpass,&my_pipelinelayout,&my_pipeline,&e,&cs);
	MAINCHECK

	f = make_framebuffers(my_device, swapchain_extent, n_swapchain_images, my_imageviews, my_renderpass, &my_framebuffers, &e,&cs);
	MAINCHECK

	f = make_commandpool(my_device, my_queues, &my_pool, &e, &cs);
	MAINCHECK

	f = make_commandbuffers(my_device,my_pool,&my_commandbuf, &e);
	MAINCHECK

	f = make_sync_objects(my_device, &sem_imgready, &sem_rendfinish, &fen_inflight, &e,&cs);
	MAINCHECK

	while (!glfwWindowShouldClose(my_window)) {
		glfwPollEvents();

		vkWaitForFences(my_device, 1, &fen_inflight, VK_TRUE, UINT64_MAX);
    	vkResetFences(my_device, 1,&fen_inflight);

    	u32 i_image = UINT32_MAX;
    	vkAcquireNextImageKHR(my_device, my_swapchain, UINT64_MAX, sem_imgready, VK_NULL_HANDLE, &i_image);
    	vkResetCommandBuffer(my_commandbuf, 0);

    	f = recordcommandbuffer(swapchain_extent, my_framebuffers[i_image], my_commandbuf, my_renderpass, my_pipeline, &e);
		MAINCHECK

		VkSemaphore waitsems[1] = {
			sem_imgready
		};
		VkSemaphore signalsems[1] = {
			sem_rendfinish
		};
		VkPipelineStageFlags waitstages[1] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};
		VkSubmitInfo si = {};
		si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		si.waitSemaphoreCount = 1;
		si.pWaitSemaphores = waitsems;
		si.pWaitDstStageMask = waitstages;
		si.commandBufferCount = 1;
		si.pCommandBuffers = &my_commandbuf;
		si.signalSemaphoreCount = 1;
		si.pSignalSemaphores = signalsems;

		vkQueueSubmit(my_queues.graphics_queue, 1, &si, fen_inflight);

		VkPresentInfoKHR pi = {};
		pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		pi.waitSemaphoreCount = 1;
		pi.pWaitSemaphores = signalsems;
		pi.swapchainCount = 1;
		pi.pSwapchains = &my_swapchain;
		pi.pImageIndices = &i_image;
		pi.pResults = NULL;

		vkQueuePresentKHR(my_queues.present_queue, &pi);

	}

	vkDeviceWaitIdle(my_device);

	cs_consume(&cs);
	return 0;
fail:
	cs_consume(&cs);
	return 1;
    
}