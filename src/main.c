
#include <string.h>
#include<volk.h>
#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<GLFW/glfw3.h>

#include "common.h"
#include "cleanupstack.h"
#include "device.h"
#include "swapchain.h"
#include "vulkan/vulkan_core.h" // having this here doesn't hurt and  prevents intellisense from adding it at the top which would break compilation


#define N_IMAGE 3




#define PROPAGATE(x) \
if (!x)


const usize WIDTH = 800;
const usize HEIGHT = 600;


const usize N_BASE_EXT = 2;
const char* extensions[N_BASE_EXT] = {
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
};

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

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT message_type,
	const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
	void* user_data
) {
	if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		printf("[WARNING]: %s\n", callback_data->pMessage);
	}

	return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT make_debugcreateinfo() {
	VkDebugUtilsMessengerCreateInfoEXT mci = {};
	mci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	mci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	mci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	mci.pfnUserCallback = debug_callback;
	mci.pUserData = NULL;
	return mci;
}

bool make_instance(VkInstance* instance, struct Error* e_out) {

    u32 n_instance_ext = 0;
	u32 n_layer = 0;
    VkExtensionProperties* instance_ext_prop = NULL;
	VkLayerProperties* layer_prop = NULL;
    vkEnumerateInstanceExtensionProperties(NULL, &n_instance_ext, NULL);
	vkEnumerateInstanceLayerProperties(&n_layer, NULL);

    instance_ext_prop = malloc(n_instance_ext * sizeof(VkExtensionProperties));
	layer_prop = malloc(n_layer * sizeof(VkLayerProperties));
    vkEnumerateInstanceExtensionProperties(NULL, &n_instance_ext, instance_ext_prop);
	vkEnumerateInstanceLayerProperties(&n_layer, layer_prop);

    printf("available instance extensions:\n");
    for (u32 i = 0; i < n_instance_ext; i++) {
        printf("\t%s\n", instance_ext_prop[i].extensionName);
    }

	printf("available instance layers:\n");
    for (u32 i = 0; i < n_layer; i++) {
        printf("\t%s\n", layer_prop[i].layerName);
    }

	u32 n_glfw_ext = 0;
	const char** glfw_ext = glfwGetRequiredInstanceExtensions(&n_glfw_ext);


	u32 n_tot_ext = n_glfw_ext + N_BASE_EXT;
	const char** all_ext = malloc(n_tot_ext * sizeof(const char*));

	for (u32 i = 0; i < N_BASE_EXT; i++) {
		all_ext[i] = extensions[i];
	}
	for (u32 i = 0; i < n_glfw_ext; i++) {
		all_ext[i + N_BASE_EXT] = glfw_ext[i];
	}

	printf("requesting these extensions:\n");
	for (u32 i = 0; i < n_tot_ext; i++) {
		printf("\t%s\n", all_ext[i]);
	}

	VkDebugUtilsMessengerCreateInfoEXT mci = make_debugcreateinfo();

    VkApplicationInfo appinfo = {};
    appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appinfo.apiVersion = VK_API_VERSION_1_4;
    appinfo.engineVersion = VK_MAKE_VERSION(0,0,0);
    appinfo.applicationVersion = VK_MAKE_VERSION(0,0,0);
    appinfo.pEngineName = "none";
    appinfo.pApplicationName = "diddytron";
    appinfo.pNext = NULL;

    VkInstanceCreateInfo ici = {};
    ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.pApplicationInfo = &appinfo;
    ici.enabledExtensionCount = n_tot_ext;
    ici.ppEnabledExtensionNames = all_ext;
    ici.enabledLayerCount = 0;
    ici.ppEnabledLayerNames = NULL;
    ici.pNext = &mci;

	const char* desired_layers[1] = {"VK_LAYER_KHRONOS_validation"};

	for (u32 i = 0; i < n_layer; i++) {
		if (strcmp(desired_layers[0], layer_prop[i].layerName) == 0) {
			ici.ppEnabledLayerNames = desired_layers;
			ici.enabledLayerCount = 1;
			printf("requesting validation layer\n");
		}
	}

    VkResult r = vkCreateInstance(&ici,NULL,instance);
    printf("created instance\n");

    free(instance_ext_prop);
	free(all_ext);

    VERIFY("instance",r)
    return false;
}

bool make_debugger(VkInstance instance, VkDebugUtilsMessengerEXT* messenger, struct Error* e_out) {
	VkDebugUtilsMessengerCreateInfoEXT mci = make_debugcreateinfo();

	VkResult r = vkCreateDebugUtilsMessengerEXT(instance, &mci, NULL, messenger);
	VERIFY("debug messenger", r)
	printf("created debug messenger\n");
	return false;
}

struct SelectedQueueFamilies {
	u32 graphicsQF;
	u32 presentQF;
};



bool make_images(VkDevice device, struct Queues queues, VkImage* image, VkImageView* imgview, struct Error* e_out) {

	const VkExtent3D ext = {800, 600, 1};

	const VkFormat myformat = VK_FORMAT_B8G8R8A8_SRGB;

	VkImageCreateInfo ici = {};
	ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ici.imageType = VK_IMAGE_TYPE_2D;
	ici.extent = ext;
	ici.format = myformat;
	ici.mipLevels = 1;
	ici.samples = VK_SAMPLE_COUNT_1_BIT;
	ici.tiling = VK_IMAGE_TILING_OPTIMAL;
	ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ici.queueFamilyIndexCount = 1;
	ici.pQueueFamilyIndices = &queues.i_graphics_queue_fam;
	ici.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	ici.arrayLayers = 1;
	ici.pNext = NULL;

	VkResult r = vkCreateImage(device, &ici, NULL, image);
	VERIFY("image create", r)

	VkImageViewCreateInfo vci = {};
	vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	vci.image = *image;
	vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
	vci.format = myformat;
	vci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	vci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	vci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	vci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	vci.subresourceRange.baseMipLevel = 0;
	vci.subresourceRange.levelCount = 1;
	vci.subresourceRange.baseArrayLayer = 0;
	vci.subresourceRange.layerCount = 1;

	r = vkCreateImageView(device,&vci,NULL,imgview);
	VERIFY("view create", r)




	return false;
}

bool make_surface(VkInstance inst, GLFWwindow* wnd, VkSurfaceKHR* surface, struct Error* e_out) {
	VkResult r = glfwCreateWindowSurface(inst, wnd, NULL, surface);
	VERIFY("create surface", r)

}

struct SurfaceCleanup {
	VkInstance inst;
	VkSurfaceKHR surf;
};
void destroy_surface(void* obj) {
	struct SurfaceCleanup* s = (struct SurfaceCleanup*)obj;
	vkDestroySurfaceKHR(s->inst,s->surf, NULL);
}







void destroy_instance(void* obj) {
	VkInstance* inst = (VkInstance*)obj;
	vkDestroyInstance(*inst,NULL);
}

struct DebugCleanup {
	VkInstance inst;
	VkDebugUtilsMessengerEXT msg;
};
void destroy_debugger(void* obj) {
	struct DebugCleanup* d = (struct DebugCleanup*)obj;
	vkDestroyDebugUtilsMessengerEXT(d->inst, d->msg, NULL);
}

void destroy_device(void* obj) {
	VkDevice* dev = (VkDevice*)obj;
	vkDestroyDevice(*dev,NULL);
}

struct ImageCleanup {
	VkDevice dev;
	VkImage img;
};
void destroy_image(void* obj) {
	struct ImageCleanup* d = (struct ImageCleanup*)obj;
	vkDestroyImage(d->dev, d->img, NULL);
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

	make_window(&my_window);
	cs_push(&cs, &my_window, sizeof(my_window), destroy_window);

    f = make_instance(&my_instance, &e);
	MAINCHECK
	cs_push(&cs, &my_instance, sizeof(my_instance), destroy_instance);
	

	volkLoadInstance(my_instance);

	f = make_debugger(my_instance, &debug_messenger, &e);
	struct DebugCleanup dc = {my_instance, debug_messenger};
	MAINCHECK
	cs_push(&cs, &dc, sizeof(dc), destroy_debugger);
	

	f = make_surface(my_instance, my_window, &my_surf, &e);
	struct SurfaceCleanup sc = {my_instance, my_surf};
	MAINCHECK
	cs_push(&cs, &sc, sizeof(sc), destroy_surface);
	

	f = make_device(my_instance, my_surf, &my_physdev, &my_device, &my_queues, &e);
	MAINCHECK
	cs_push(&cs, &my_device, sizeof(my_device), destroy_device);
	

	volkLoadDevice(my_device);

	f = make_swapchain(my_physdev, my_device, my_queues, my_surf, my_window, &my_swapchain, &swapchain_format, &swapchain_extent,&n_swapchain_images,&swapchain_images, &e);
	MAINCHECK
	struct SwapchainCleanup swc = {my_device,my_swapchain, swapchain_images};
	cs_push(&cs, &swc, sizeof(swc),destroy_swapchain);

	f = make_swapchain_imageviews(my_device, n_swapchain_images, swapchain_images, swapchain_format, &my_imageviews, &e);
	MAINCHECK
	struct ImageViewCleanup ivc = {my_device, my_imageviews, n_swapchain_images};
	cs_push(&cs, &ivc, sizeof(ivc), destroy_imageviews);

	while (false && !glfwWindowShouldClose(my_window)) {

	}

	cs_consume(&cs);
	return 0;
fail:
	cs_consume(&cs);
	return 1;
    
}