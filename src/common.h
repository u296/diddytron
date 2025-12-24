#ifndef COMMON_H
#define COMMON_H
#include <stdint.h>
#include <volk.h>
#include <stdbool.h>

typedef uint32_t u32;
typedef int32_t i32;
typedef uint8_t u8;

typedef uintptr_t usize;

#define CLAMP(a,min,max) (a < min ? min : (a > max ? max : a))

#define VERIFY(o,r) \
if (r != VK_SUCCESS) { \
    *e_out = (struct Error){.origin=o,.code=r}; \
    return true;\
}

typedef struct Error {
    const char* origin;
    VkResult code;
} Error;

#endif