
#include <string.h>
#include<volk.h>
#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<time.h>
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
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	*window = glfwCreateWindow(WIDTH, HEIGHT, "diddytron", NULL, NULL);

	return false;
}

void destroy_window(void* obj) {
	GLFWwindow** window = (GLFWwindow**)obj;
	glfwDestroyWindow(*window);
	glfwTerminate();
}

#define MAINCHECK    if(f) {\
        printf("Error, code=%d: %s\n",e.code,e.origin);\
        goto fail; \
    }

typedef enum LoopStatus {
	REMAKE_SWAPCHAIN,
	EXIT_PROGRAM
} LoopStatus;

LoopStatus do_renderloop(
	u64* i_frame,
	clock_t* last_time,
	const u32 n_max_inflight,
	const u32 n_frameratecheck,
	GLFWwindow* wnd,
	VkDevice dev,
	Queues queues,
	VkSwapchainKHR swp,
	VkFramebuffer* fbufs,
	bool* fb_resized,
	VkExtent2D swp_ext,
	VkRenderPass rp,
	VkPipeline pipe,
	VkCommandBuffer* cmdbufs,
	VkSemaphore* sem_imgready,
	VkSemaphore* sem_rendfinish,
	VkFence* fen_inflight

) {
	VkResult f = VK_ERROR_UNKNOWN;
	struct Error e = {};

	while (!glfwWindowShouldClose(wnd)) {
		glfwPollEvents();

		const u64 i_frame_modn = *i_frame % n_max_inflight;

		vkWaitForFences(dev, 1, &fen_inflight[i_frame_modn], VK_TRUE, UINT32_MAX);

    	u32 i_image = UINT32_MAX;
    	VkResult img_ac_res = vkAcquireNextImageKHR(dev, swp, UINT64_MAX, sem_imgready[i_frame_modn], VK_NULL_HANDLE, &i_image);

		switch (img_ac_res) {
			case VK_ERROR_OUT_OF_DATE_KHR:
				return REMAKE_SWAPCHAIN;
				break;
			case VK_SUBOPTIMAL_KHR:
			case VK_SUCCESS:
				break;
			default:
				goto fail;
				break;
		}

		vkResetFences(dev, 1,&fen_inflight[i_frame_modn]);
    	vkResetCommandBuffer(cmdbufs[i_frame_modn], 0);

    	f = recordcommandbuffer(swp_ext, fbufs[i_image], cmdbufs[i_frame_modn], rp, pipe, &e);
		MAINCHECK
		

		VkSemaphore waitsems[1] = {
			sem_imgready[i_frame_modn]
		};
		VkSemaphore signalsems[1] = {
			sem_rendfinish[i_frame_modn]
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
		si.pCommandBuffers = &cmdbufs[i_frame_modn];
		si.signalSemaphoreCount = 1;
		si.pSignalSemaphores = signalsems;


		vkQueueSubmit(queues.graphics_queue, 1, &si, fen_inflight[i_frame_modn]);

		VkPresentInfoKHR pi = {};
		pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		pi.waitSemaphoreCount = 1;
		pi.pWaitSemaphores = signalsems;
		pi.swapchainCount = 1;
		pi.pSwapchains = &swp;
		pi.pImageIndices = &i_image;
		pi.pResults = NULL;

		VkResult pres_res = vkQueuePresentKHR(queues.present_queue, &pi);

		switch (pres_res) {
			case VK_SUCCESS:
				break;
			case VK_SUBOPTIMAL_KHR:
			case VK_ERROR_OUT_OF_DATE_KHR:
				*fb_resized = false;
				return REMAKE_SWAPCHAIN;
				break;
			default:
				goto fail;
				break;
		}

		if (*fb_resized) {
			*fb_resized = false;
			return REMAKE_SWAPCHAIN;
		}

		(*i_frame)++;

		if (*i_frame % n_frameratecheck == n_frameratecheck-1) {
			const clock_t time_now = clock();

			const float fps = (float)n_frameratecheck * (float)CLOCKS_PER_SEC / (float)(time_now - *last_time);

			*last_time = time_now;

			printf("FPS: %.0f\n", fps);
		}

	}
fail: 
	return EXIT_PROGRAM;
}


void fb_resize_callback(GLFWwindow* wnd, int width, int height) {
	bool* fbresize = (bool*)glfwGetWindowUserPointer(wnd);
	*fbresize = true;
}


int main() {

	struct CleanupStack cs = {};
	cs_init(&cs);

	volkInitialize();
    bool f = false;    

    struct Error e = {.code=0,.origin=""};

	const u32 n_max_inflight = 2;

	GLFWwindow* my_window;
	bool fb_resized = false;
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
	VkCommandBuffer* my_commandbufs;
	VkSemaphore* sem_imgready;
	VkSemaphore* sem_rendfinish;
	VkFence* fen_inflight;



	make_window(&my_window);
	CLEANUP_START_ONORES(GLFWwindow*)
	my_window
	CLEANUP_END_O(window)

	glfwSetWindowUserPointer(my_window, &fb_resized);
	glfwSetFramebufferSizeCallback(my_window, fb_resize_callback);

	//cs_push(&cs, &my_window, sizeof(my_window), destroy_window);

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

	CleanupStack swp_cs = {};
	cs_init(&swp_cs);

	f = make_swapchain(my_physdev, my_device, my_queues, my_surf, my_window, &my_swapchain, &swapchain_format, &swapchain_extent,&n_swapchain_images,&swapchain_images, &e, &swp_cs);
	MAINCHECK

	f = make_swapchain_imageviews(my_device, n_swapchain_images, swapchain_images, swapchain_format, &my_imageviews, &e, &swp_cs);
	MAINCHECK

	f = make_renderpass(my_device, swapchain_format, &my_renderpass, &e, &cs);
	MAINCHECK

	f = make_graphicspipeline(my_device, swapchain_extent,my_renderpass,&my_pipelinelayout,&my_pipeline,&e,&cs);
	MAINCHECK

	f = make_framebuffers(my_device, swapchain_extent, n_swapchain_images, my_imageviews, my_renderpass, &my_framebuffers, &e,&swp_cs);
	MAINCHECK

	f = make_commandpool(my_device, my_queues, &my_pool, &e, &cs);
	MAINCHECK

	f = make_commandbuffers(my_device,my_pool,n_max_inflight,&my_commandbufs, &e, &cs);
	MAINCHECK

	f = make_sync_objects(my_device, n_max_inflight,  &sem_imgready, &sem_rendfinish, &fen_inflight, &e,&cs);
	MAINCHECK

	u64 i_frame = 0;
	const u64 n_frameratecheck = 100;

	clock_t last_time = clock();

	bool firstiter = true;

	bool shouldclose = false;

	do {
		// this is the loop that owns the swapchain and recreates it whenever the renderloop exits because the swapchain needs renewal

		if (!firstiter) {

			int width = 0, height = 0;
			
			glfwGetFramebufferSize(my_window, &width, &height);
			while (width == 0 || height == 0) {
				glfwGetFramebufferSize(my_window, &width, &height);
				glfwWaitEvents();
			}

			// if we get here it means the swapchain needs to be recreated
			cs_init(&swp_cs);

			make_swapchain(my_physdev, my_device, my_queues, my_surf, my_window, &my_swapchain, &swapchain_format, &swapchain_extent, &n_swapchain_images, &swapchain_images, &e, &swp_cs);

			make_swapchain_imageviews(my_device, n_swapchain_images, swapchain_images, swapchain_format, &my_imageviews, &e, &swp_cs);

			make_framebuffers(my_device, swapchain_extent, n_swapchain_images, my_imageviews, my_renderpass, &my_framebuffers, &e, &swp_cs);
			printf("remade swapchain\n");
		}
		



		LoopStatus l = do_renderloop(
		&i_frame,
		&last_time,
		n_max_inflight,
		n_frameratecheck,
		my_window,
		my_device,
		my_queues,
		my_swapchain,
		my_framebuffers,
		&fb_resized,
		swapchain_extent,
		my_renderpass,
		my_pipeline,
		my_commandbufs,
		sem_imgready,
		sem_rendfinish,
		fen_inflight
		);

		vkDeviceWaitIdle(my_device);
		cs_consume(&swp_cs);

		switch (l) {
		case REMAKE_SWAPCHAIN:
			break;
		case EXIT_PROGRAM:
			shouldclose = true;
			break;
		
		}

		firstiter = false;
	} while (!shouldclose);
	

	vkDeviceWaitIdle(my_device);

	cs_consume(&cs);
	return 0;
fail:
	cs_consume(&cs);
	return 1;
    
}