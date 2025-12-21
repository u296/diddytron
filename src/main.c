#include <string.h>
#include<volk.h>
#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>



#define VERIFY(o,r) \
if (r != VK_SUCCESS) { \
    *e_out = (struct Error){.origin=o,.code=r}; \
    return true;\
}

#define PROPAGATE(x) \
if (!x)



typedef uint32_t u32;
typedef uint8_t u8;

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

bool enumerate_devices(VkInstance instance, VkDevice* device, struct Error* e_out) {

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


	printf("queue families of selected device:\n");
	for (u32 i = 0; i < n_qfam; i++) {
		if (qfams[i].queueFlags | VK_QUEUE_GRAPHICS_BIT) {
			sqf.graphicsQF = i;
			set_if_found.graphicsQF = 1;
			break;
		}
	}

	if (!set_if_found.graphicsQF) {
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

	return false;

}

void cleanup(VkInstance instance, VkDebugUtilsMessengerEXT messenger, VkDevice device) {
	vkDestroyDevice(device, NULL);
	vkDestroyDebugUtilsMessengerEXT(instance, messenger, NULL);
    vkDestroyInstance(instance, NULL);
}


#define MAINCHECK    if(f) {\
        printf("Error, code=%d: %s\n",e.code,e.origin);\
        return 1;\
    }

int main() {


	volkInitialize();
    bool f = false;    

    struct Error e = {.code=0,.origin=""};

    VkInstance my_instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkDevice my_device;

    f = make_instance(&my_instance, &e);
	MAINCHECK
	volkLoadInstance(my_instance);
	f = make_debugger(my_instance, &debug_messenger, &e);
	MAINCHECK

	enumerate_devices(my_instance,&my_device, &e);

	cleanup(my_instance, debug_messenger, my_device);

	

   return 0;
    
}