#ifndef __CAMERA_HPP__
#define __CAMERA_HPP__

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
    float aspect;
    float fov;

    float yaw;
    float pitch;

    float speed;

    CameraContext context;
};

#endif
