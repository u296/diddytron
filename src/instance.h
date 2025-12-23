#ifndef INSTANCE_H
#define INSTANCE_H
#include "common.h"
#include <GLFW/glfw3.h>


bool make_instance(VkInstance* instance, struct Error* e_out);

void destroy_instance(void* obj);


typedef struct DebugCleanup {
	VkInstance inst;
	VkDebugUtilsMessengerEXT msg;
} DebugCleanup;

bool make_debugger(VkInstance instance, VkDebugUtilsMessengerEXT* messenger, struct Error* e_out);
void destroy_debugger(void* obj);

typedef struct SurfaceCleanup {
	VkInstance inst;
	VkSurfaceKHR surf;
} SurfaceCleanup;


bool make_surface(VkInstance inst, GLFWwindow* wnd, VkSurfaceKHR* surface, struct Error* e_out);
void destroy_surface(void* obj);

#endif