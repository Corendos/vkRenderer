#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "cg_math.h"
#include "cg_memory.h"

struct CameraContext {
    Mat4f projection;
    Mat4f view;
};

struct CameraResources {
    VkDescriptorSet* descriptor_sets;
    VkBuffer* buffers;
    AllocatedMemoryChunk* allocations;
};

struct Camera {
    Vec3f position;
    f32 aspect;
    f32 fov;
    
    f32 yaw;
    f32 pitch;
    
    f32 speed;
    
    CameraContext context;
};

#endif
