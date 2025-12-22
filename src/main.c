#include <string.h>
#include<volk.h>
#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>

#include "common.h"
#include "cleanupstack.h"
#include "vulkan/vulkan_core.h" // having this here doesn't hurt and  prevents intellisense from adding it at the top which would break compilation

#define N_IMAGE 3


#define VERIFY(o,r) \
if (r != VK_SUCCESS) { \
    *e_out = (struct Error){.origin=o,.code=r}; \
    return true;\
}

#define PROPAGATE(x) \
if (!x)





struct Error {
    const char* origin;
    VkResult code;
};

const char* extensions[3] = {
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
    "aoeu"
};

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
    ici.enabledExtensionCount = 2;
    ici.ppEnabledExtensionNames = extensions;
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
};

struct Queues {
	VkQueue graphics_queue;
	u32 i_graphics_queue_fam;
};

bool make_device(VkInstance instance, VkDevice* device, struct Queues* queues, struct Error* e_out) {

	u32 n_dev = 0;
	VkPhysicalDevice* devs = NULL;
	vkEnumeratePhysicalDevices(instance, &n_dev, NULL);

	devs = malloc(sizeof(VkPhysicalDevice) * n_dev);
	VkPhysicalDeviceProperties* props = malloc(sizeof(VkPhysicalDeviceProperties) * n_dev);
	VkPhysicalDeviceFeatures* feats = malloc(sizeof(VkPhysicalDeviceFeatures) * n_dev);

	vkEnumeratePhysicalDevices(instance, &n_dev, devs);

	printf("Physical devices found: %u\n", n_dev);
	for (u32 i = 0; i < n_dev; i++) {
		vkGetPhysicalDeviceProperties(devs[i], &props[i]);
		vkGetPhysicalDeviceFeatures(devs[i], &feats[i]);
		printf("\t%s\n", props[i].deviceName);
	}

	if (n_dev == 0) {
		printf("no devices found, aborting");
		exit(1);
	}

	// just select device 0

	u32 i_sel = 0;

	u32 n_qfam = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(devs[i_sel], &n_qfam, NULL);

	VkQueueFamilyProperties* qfams = malloc(n_qfam*sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(devs[i_sel],&n_qfam,qfams);

	struct SelectedQueueFamilies sqf = {};
	struct SelectedQueueFamilies set_if_found = {};


	printf("queue families of selected device (%u total):\n", n_qfam);
	for (u32 i = 0; i < n_qfam; i++) {
		printf("\tindex %u flags: %x\n",i,qfams[i].queueFlags);
		if (qfams[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			sqf.graphicsQF = i;
			set_if_found.graphicsQF = 1;
			break;
		}
	}

	if (!set_if_found.graphicsQF) {
		printf("NO GRAPHICS QUEUE\n");
		exit(1);
	}

	float qprio = 1.0f;

	VkDeviceQueueCreateInfo dqci = {};
	dqci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	dqci.queueFamilyIndex = sqf.graphicsQF;
	dqci.queueCount = 1;
	dqci.pQueuePriorities = &qprio;

	VkPhysicalDeviceFeatures df = {};

	VkDeviceCreateInfo dci = {};
	dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	dci.queueCreateInfoCount = 1;
	dci.pQueueCreateInfos = &dqci;
	dci.pEnabledFeatures = &df;

	VkResult r = vkCreateDevice(devs[i_sel], &dci, NULL, device);
	
	free(qfams);
	free(props);
	free(devs);
	VERIFY("device creation", r);
	printf("created device\n");


	vkGetDeviceQueue(*device, sqf.graphicsQF, 0, &queues->graphics_queue);
	printf("got graphics queue %p\n", queues->graphics_queue);
	queues->i_graphics_queue_fam = sqf.graphicsQF;

	return false;

}

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

struct ImageViewCleanup {
	VkDevice dev;
	VkImageView imgv;
};
void destroy_image_view(void* obj) {
	struct ImageViewCleanup* d = (struct ImageViewCleanup*)obj;
	vkDestroyImageView(d->dev, d->imgv, NULL);
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

    VkInstance my_instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkDevice my_device;
	struct Queues my_queues = {};
	VkImage my_img[N_IMAGE];
	VkImageView my_imgview[N_IMAGE];

	

    f = make_instance(&my_instance, &e);
	cs_push(&cs, &my_instance, sizeof(my_instance), destroy_instance);
	MAINCHECK

	volkLoadInstance(my_instance);

	f = make_debugger(my_instance, &debug_messenger, &e);
	struct DebugCleanup dc = {my_instance, debug_messenger};
	cs_push(&cs, &dc, sizeof(dc), destroy_debugger);
	MAINCHECK

	f = make_device(my_instance,&my_device, &my_queues, &e);
	cs_push(&cs, &my_device, sizeof(my_device), destroy_device);
	MAINCHECK

	volkLoadDevice(my_device);

	f = make_images(my_device, my_queues, my_img, my_imgview, &e);
	struct ImageCleanup ic = {my_device,my_img[0]};
	struct ImageViewCleanup ivc = {my_device, my_imgview[0]};
	cs_push(&cs,&ic,sizeof(ic),destroy_image);
	cs_push(&cs,&ivc,sizeof(ivc),destroy_image_view);
	MAINCHECK



	cs_consume(&cs);
	return 0;
fail:
	cs_consume(&cs);
	return 1;
    
}