#include "cg_vk_helper.h"

#include <string.h>

#include "cg_macros.h"
#include "cg_memory.h"

const char* required_layers[]     = { "VK_LAYER_LUNARG_standard_validation" };
const char* required_device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

inline const char* get_device_type_str(VkPhysicalDeviceType type) {
    switch (type) {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU";
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        return "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU";
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        return "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU";
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
        return "VK_PHYSICAL_DEVICE_TYPE_CPU";
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        default:
        return "VK_PHYSICAL_DEVICE_TYPE_OTHER";
    }
}

inline const char* vk_error_code_str(VkResult result) {
    switch (result) {
        case VK_SUCCESS:
        return "VK_SUCCESS";
        break;
        case VK_NOT_READY:
        return "VK_NOT_READY";
        break;
        case VK_TIMEOUT:
        return "VK_TIMEOUT";
        break;
        case VK_EVENT_SET:
        return "VK_EVENT_SET";
        break;
        case VK_EVENT_RESET:
        return "VK_EVENT_RESET";
        break;
        case VK_INCOMPLETE:
        return "VK_INCOMPLETE";
        break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "VK_ERROR_OUT_OF_HOST_MEMORY";
        break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        break;
        case VK_ERROR_INITIALIZATION_FAILED:
        return "VK_ERROR_INITIALIZATION_FAILED";
        break;
        case VK_ERROR_DEVICE_LOST:
        return "VK_ERROR_DEVICE_LOST";
        break;
        case VK_ERROR_MEMORY_MAP_FAILED:
        return "VK_ERROR_MEMORY_MAP_FAILED";
        break;
        case VK_ERROR_LAYER_NOT_PRESENT:
        return "VK_ERROR_LAYER_NOT_PRESENT";
        break;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
        break;
        case VK_ERROR_FEATURE_NOT_PRESENT:
        return "VK_ERROR_FEATURE_NOT_PRESENT";
        break;
        case VK_ERROR_INCOMPATIBLE_DRIVER:
        return "VK_ERROR_INCOMPATIBLE_DRIVER";
        break;
        case VK_ERROR_TOO_MANY_OBJECTS:
        return "VK_ERROR_TOO_MANY_OBJECTS";
        break;
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
        return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        break;
        case VK_ERROR_FRAGMENTED_POOL:
        return "VK_ERROR_FRAGMENTED_POOL";
        break;
        case VK_SUBOPTIMAL_KHR:
        return "VK_SUBOPTIMAL_KHR";
        break;
        case VK_ERROR_OUT_OF_DATE_KHR:
        return "VK_ERROR_OUT_OF_DATE_KHR";
        break;
        default:
        return "UNKNOWN";
        break;
    }
}

inline VkFormat get_format_from_channels(u32 channels) {
    switch (channels) {
        case 1:
        return VK_FORMAT_R8_UNORM;
        case 2:
        return VK_FORMAT_R8G8_UNORM;
        case 3:
        return VK_FORMAT_B8G8R8_UNORM;
        case 4:
        return VK_FORMAT_B8G8R8A8_UNORM;
    }
    return VK_FORMAT_UNDEFINED;
}

inline bool check_queue_availability(PhysicalDeviceSelection* selection, VkSurfaceKHR surface) {
    u32 queue_family_property_count = 0;
    
    vkGetPhysicalDeviceQueueFamilyProperties(selection->device, &queue_family_property_count, nullptr);
    
    VkQueueFamilyProperties* queue_family_property = (VkQueueFamilyProperties*)calloc(queue_family_property_count, sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(selection->device, &queue_family_property_count, queue_family_property);
    
    
    bool graphics_found = false;
    bool transfer_found = false;
    
    bool compute_found = false;
    bool present_found = false;
    
    for (int i = 0;i < queue_family_property_count;++i) {
        if (!graphics_found && queue_family_property[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            selection->graphics_queue_family_index = i;
            graphics_found = true;
        }
        
        if (!transfer_found && queue_family_property[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            selection->transfer_queue_family_index = i;
            transfer_found = true;
        }
        
        if (!compute_found && queue_family_property[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            selection->compute_queue_family_index = i;
            compute_found = true;
        }
        
        if (!present_found) {
            VkBool32 present_supported = false;
            if (vkGetPhysicalDeviceSurfaceSupportKHR(selection->device, i, surface, &present_supported) != VK_SUCCESS) {
                println("Error: failed to query present support for queue family #%d", i);
                free_null(queue_family_property);
                return false;
            }
            
            if (present_supported) {
                selection->present_queue_family_index = i;
                present_found = true;
            }
        }
    }
    
    free_null(queue_family_property);
    return graphics_found && transfer_found && compute_found && present_found;
}


inline u32 get_device_type_score(VkPhysicalDeviceType type) {
    switch (type) {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return 10;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        return 20;
        default:
        return 0;
    }
}

inline bool select_physical_device(RendererState* state) {
    u32 physical_device_count = 0;
    VkResult result = vkEnumeratePhysicalDevices(state->instance, &physical_device_count, nullptr);
    
    if (result != VK_SUCCESS) {
        println("vkEnumeratePhysicalDevices returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    VkPhysicalDevice* physical_devices = (VkPhysicalDevice*) calloc(physical_device_count, sizeof(VkPhysicalDevice));
    
    result = vkEnumeratePhysicalDevices(state->instance, &physical_device_count, physical_devices);
    if (result != VK_SUCCESS) {
        println("vkEnumeratePhysicalDevices returned (%s)", vk_error_code_str(result));
        free_null(physical_devices);
        return false;
    }
    
    u32 selected_device_score = 0;
    
    for (int i = 0;i < physical_device_count;++i) {
        PhysicalDeviceSelection current_selection = {};
        current_selection.device = physical_devices[i];
        if (!check_queue_availability(&current_selection, state->surface)) continue;
        
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);
        
        u32 score = get_device_type_score(properties.deviceType);
        if (score > selected_device_score) {
            selected_device_score = score;
            state->selection = current_selection;
        }
    }
    
    free_null(physical_devices);
    return selected_device_score > 0;
}

inline u32 get_surface_format_score(VkSurfaceFormatKHR surface_format) {
    u32 score = 0;
    switch (surface_format.format) {
        case VK_FORMAT_B8G8R8A8_UNORM:
        score = 10;
        break;
        case VK_FORMAT_B8G8R8A8_SRGB:
        score = 20;
        break;
    }
    
    return score;
}

inline bool select_surface_format(RendererState* state) {
    u32 surface_format_count = 0;
    VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(state->selection.device, state->surface, &surface_format_count, nullptr);
    
    if (result != VK_SUCCESS) {
        println("vkGetPhysicalDeviceSurfaceFormatsKHR returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    VkSurfaceFormatKHR* surface_formats = (VkSurfaceFormatKHR*)calloc(surface_format_count, sizeof(VkSurfaceFormatKHR));
    
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(state->selection.device, state->surface, &surface_format_count, surface_formats);
    if (result != VK_SUCCESS) {
        println("vkGetPhysicalDeviceSurfaceFormatsKHR returned (%s)", vk_error_code_str(result));
        free_null(surface_formats);
        return false;
    }
    
    u32 selection_score = 0;
    
    for (int i = 0;i < surface_format_count;++i) {
        u32 score = get_surface_format_score(surface_formats[i]);
        if (score > selection_score) {
            selection_score = score;
            state->surface_format = surface_formats[i];
        }
    }
    
    free_null(surface_formats);
    return selection_score > 0;
}

inline u32 get_present_mode_score(VkPresentModeKHR present_mode) {
    u32 score = 0;
    switch (present_mode) {
        case VK_PRESENT_MODE_IMMEDIATE_KHR:
        score = 10;
        break;
        case VK_PRESENT_MODE_MAILBOX_KHR:
        score = 20;
        break;
        case VK_PRESENT_MODE_FIFO_KHR:
        score = 30;
        break;
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
        score = 35;
        break;
    }
    
    return score;
}

inline bool select_present_mode(RendererState* state) {
    u32 present_mode_count = 0;
    VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(state->selection.device, state->surface, &present_mode_count, nullptr);
    
    if (result != VK_SUCCESS) {
        println("vkGetPhysicalDeviceSurfacePresentModesKHR returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    VkPresentModeKHR* present_modes = (VkPresentModeKHR*)calloc(present_mode_count, sizeof(VkPresentModeKHR));
    
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(state->selection.device, state->surface, &present_mode_count, present_modes);
    if (result != VK_SUCCESS) {
        println("vkGetPhysicalDevoceSurfacePresentModesKHR returned (%s)", vk_error_code_str(result));
        free_null(present_modes);
        return false;
    }
    
    u32 selection_score = 0;
    
    for (int i = 0;i < present_mode_count;++i) {
        u32 score = get_present_mode_score(present_modes[i]);
        if (score > selection_score) {
            selection_score = score;
            state->present_mode = present_modes[i];
        }
    }
    
    free_null(present_modes);
    return selection_score > 0;
}


inline bool check_required_layers() {
    VkLayerProperties layer_properties[32];
    u32 layer_property_count = 32;
    
    VkResult result = vkEnumerateInstanceLayerProperties(&layer_property_count, layer_properties);
    
    if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
        println("vkEnumerateInstanceLayerProperties returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    for (int j = 0;j < array_size(required_layers);++j) {
        bool found = false;
        
        for (int i = 0;i < layer_property_count;++i) {
            if (strcmp(required_layers[j], layer_properties[i].layerName) == 0) {
                found = true;
            }
        }
        
        if (!found) {
            println("Error: Missing layer '%s'", required_layers[j]);
            return false;
        }
    }
    
    return true;
}

inline bool check_required_instance_extensions(const char** extensions, u32 count) {
    u32 extension_property_count = 0;
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_property_count, nullptr);
    
    if (result != VK_SUCCESS) {
        println("vkEnumerateInstanceExtensionProperties returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    VkExtensionProperties* extension_properties = (VkExtensionProperties*)calloc(extension_property_count, sizeof(VkExtensionProperties));
    result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_property_count, extension_properties);
    
    if (result != VK_SUCCESS) {
        println("vkEnumerateInstanceExtensionProperties returned (%s)", vk_error_code_str(result));
        free_null(extension_properties);
        return false;
    }
    
    for (int i = 0;i < count;++i) {
        bool found = false;
        for (int j = 0;j < extension_property_count;++j) {
            if (strcmp(extensions[i], extension_properties[j].extensionName) == 0) {
                found = true;
            }
        }
        
        if (!found) {
            println("Error: instance extension '%s' is not available", extensions[i]);
            free_null(extension_properties);
            return false;
        }
    }
    
    free_null(extension_properties);
    return true;
}

inline bool check_required_device_extensions(VkPhysicalDevice physical_device) {
    u32 extension_property_count = 0;
    VkResult result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_property_count, nullptr);
    
    if (result != VK_SUCCESS) {
        println("vkEnumerateDeviceExtensionProperties returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    VkExtensionProperties* extension_properties = (VkExtensionProperties*)calloc(extension_property_count, sizeof(VkExtensionProperties));
    result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_property_count, extension_properties);
    
    if (result != VK_SUCCESS) {
        println("vkEnumerateDeviceExtensionProperties returned (%s)", vk_error_code_str(result));
        free_null(extension_properties);
        return false;
    }
    
    for (int i = 0;i < array_size(required_device_extensions);++i) {
        bool found = false;
        for (int j = 0;j < extension_property_count;++j) {
            if (strcmp(required_device_extensions[i], extension_properties[j].extensionName) == 0) {
                found = true;
            }
        }
        
        if (!found) {
            println("Error: device extension '%s' is not available.", required_device_extensions[i]);
            free_null(extension_properties);
            return false;
        }
    }
    
    free_null(extension_properties);
    return true;
}

inline bool create_instance(VkInstance* instance) {
    u32 version;
    VkResult result = vkEnumerateInstanceVersion(&version);
    if (result != VK_SUCCESS) {
        println("vkEnumerateInstanceVersion returned (%s)", vk_error_code_str(result));
        return false;
    };
    
    println("Supported Version: %d.%d.%d",
            VK_VERSION_MAJOR(version),
            VK_VERSION_MINOR(version),
            VK_VERSION_PATCH(version));
    
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Renderer";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Godo Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = version;
    
    u32 extension_count = 0;
    const char** required_instance_extensions = glfwGetRequiredInstanceExtensions(&extension_count);
    
    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &app_info;
    instance_create_info.enabledLayerCount = array_size(required_layers);
    instance_create_info.ppEnabledLayerNames = required_layers;
    instance_create_info.enabledExtensionCount = extension_count;
    instance_create_info.ppEnabledExtensionNames = required_instance_extensions;
    
    result = vkCreateInstance(&instance_create_info, nullptr, instance);
    
    if (result != VK_SUCCESS) {
        println("vkCreateInstance returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    return true;
}

inline bool create_device_and_queues(RendererState* state) {
    u32 graphics_queue_index = 0;
    u32 transfer_queue_index = 0;
    u32 compute_queue_index = 0;
    u32 present_queue_index = 0;
    
    u32 queue_count_per_family[8] = {0};
    graphics_queue_index = queue_count_per_family[state->selection.graphics_queue_family_index]++;
    transfer_queue_index = queue_count_per_family[state->selection.transfer_queue_family_index]++;
    compute_queue_index  = queue_count_per_family[state->selection.compute_queue_family_index ]++;
    present_queue_index  = queue_count_per_family[state->selection.present_queue_family_index ]++;
    
    u32 queue_create_info_count = 0;
    
    VkDeviceQueueCreateInfo queue_create_info[8] = {};
    f32 graphics_queue_priority[8][8] = {1.0};
    
    for (int i = 0;i < 8;++i) {
        if (queue_count_per_family[i] == 0) continue;
        
        queue_create_info[queue_create_info_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info[queue_create_info_count].queueFamilyIndex = i;
        queue_create_info[queue_create_info_count].queueCount = queue_count_per_family[i];
        queue_create_info[queue_create_info_count].pQueuePriorities = graphics_queue_priority[queue_create_info_count];
        
        queue_create_info_count++;
    }
    
    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = queue_create_info_count;
    device_create_info.pQueueCreateInfos = queue_create_info;
    device_create_info.enabledLayerCount = array_size(required_layers);
    device_create_info.ppEnabledLayerNames = required_layers;
    device_create_info.enabledExtensionCount = array_size(required_device_extensions);
    device_create_info.ppEnabledExtensionNames = required_device_extensions;
    
    VkResult result = vkCreateDevice(state->selection.device, &device_create_info, nullptr, &state->device);
    
    if (result != VK_SUCCESS) {
        println("vkCreateDevice returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    vkGetDeviceQueue(state->device, state->selection.graphics_queue_family_index, graphics_queue_index, &state->graphics_queue);
    vkGetDeviceQueue(state->device, state->selection.transfer_queue_family_index, transfer_queue_index, &state->transfer_queue);
    vkGetDeviceQueue(state->device, state->selection.compute_queue_family_index, compute_queue_index, &state->compute_queue);
    vkGetDeviceQueue(state->device, state->selection.present_queue_family_index, present_queue_index, &state->present_queue);
    
    return true;
}

inline bool create_swapchain(RendererState* state) {
    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state->selection.device, state->surface, &surface_capabilities);
    if (result != VK_SUCCESS) {
        println("vkGetPhysicalDeviceSurfaceCapabilitiesKHR returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    state->swapchain_extent = surface_capabilities.currentExtent;
    
    VkSwapchainKHR new_swapchain = {};
    
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = state->surface;
    swapchain_create_info.minImageCount = surface_capabilities.minImageCount;
    swapchain_create_info.imageFormat = state->surface_format.format;
    swapchain_create_info.imageColorSpace = state->surface_format.colorSpace;
    swapchain_create_info.imageExtent = surface_capabilities.currentExtent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    u32 queue_indices[2] = {0};
    u32 queue_indice_count = 0;
    if (state->selection.graphics_queue_family_index == state->selection.present_queue_family_index) {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        queue_indice_count = 0;
    } else {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        queue_indice_count = 2;
        queue_indices[0] = state->selection.present_queue_family_index;
        queue_indices[1] = state->selection.graphics_queue_family_index;
    }
    
    swapchain_create_info.queueFamilyIndexCount = queue_indice_count;
    swapchain_create_info.pQueueFamilyIndices = queue_indices;
    swapchain_create_info.presentMode = state->present_mode;
    swapchain_create_info.preTransform = surface_capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = state->swapchain;
    
    result = vkCreateSwapchainKHR(state->device, &swapchain_create_info, nullptr, &new_swapchain);
    if (result != VK_SUCCESS) {
        println("vkCreateSwapchainKHR returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    if (state->swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(state->device, state->swapchain, nullptr);
    }
    
    state->swapchain = new_swapchain;
    
    return true;
}

inline bool get_swapchain_images(RendererState* state) {
    state->swapchain_image_count = 0;
    VkResult result = vkGetSwapchainImagesKHR(state->device, state->swapchain, &state->swapchain_image_count, nullptr);
    if (result != VK_SUCCESS) {
        println("vkGetSwapchainImagesKHR returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    state->swapchain_images = (VkImage*)calloc(state->swapchain_image_count, sizeof(VkImage));
    
    result = vkGetSwapchainImagesKHR(state->device, state->swapchain, &state->swapchain_image_count, state->swapchain_images);
    if (result != VK_SUCCESS) {
        println("vkGetSwapchainImagesKHR returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    return true;
}

inline bool create_swapchain_image_views(RendererState* state) {
    state->swapchain_image_views = (VkImageView*)calloc(state->swapchain_image_count, sizeof(VkImageView));
    
    VkImageSubresourceRange subresource_range = {};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = 1;
    
    VkImageViewCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = state->surface_format.format;
    create_info.subresourceRange = subresource_range;
    
    for (int i = 0;i < state->swapchain_image_count;++i) {
        create_info.image = state->swapchain_images[i];
        VkResult result = vkCreateImageView(state->device, &create_info, nullptr, &state->swapchain_image_views[i]);
        if (result != VK_SUCCESS) {
            println("vkCreateImageView returned (%s)", vk_error_code_str(result));
            return false;
        }
    }
    
    return true;
}

inline bool create_depth_image_views(RendererState* state) {
    state->depth_image_views = (VkImageView*)calloc(state->swapchain_image_count, sizeof(VkImageView));
    
    VkImageSubresourceRange subresource_range = {};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = 1;
    
    VkImageViewCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = VK_FORMAT_D32_SFLOAT;
    create_info.subresourceRange = subresource_range;
    
    for (int i = 0;i < state->swapchain_image_count;++i) {
        create_info.image = state->depth_images[i];
        
        VkResult result = vkCreateImageView(state->device, &create_info, nullptr, &state->depth_image_views[i]);
        if (result != VK_SUCCESS) {
            println("vkCreateImageView returned (%s)", vk_error_code_str(result));
            free_null(state->depth_image_views);
            return false;
        }
    }
    
    return true;
}

inline bool create_depth_images(RendererState* state) {
    state->depth_images            = (VkImage*)calloc(state->swapchain_image_count, sizeof(VkImage));
    state->depth_image_allocations = (AllocatedMemoryChunk*)calloc(state->swapchain_image_count, sizeof(AllocatedMemoryChunk));
    
    VkImageCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.imageType = VK_IMAGE_TYPE_2D;
    create_info.format = VK_FORMAT_D32_SFLOAT;
    create_info.extent = { state->swapchain_extent.width, state->swapchain_extent.height, 1 };
    create_info.mipLevels = 1;
    create_info.arrayLayers = 1;
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &state->selection.graphics_queue_family_index;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    
    for (int i = 0;i < state->swapchain_image_count;++i) {
        VkResult result = vkCreateImage(state->device, &create_info, nullptr, &state->depth_images[i]);
        if (result != VK_SUCCESS) {
            println("vkCreateImage returned (%s)", vk_error_code_str(result));
            free_null(state->depth_images);
            return false;
        }
    }
    
    for (int i = 0;i < state->swapchain_image_count;++i) {
        VkMemoryRequirements requirements = {};
        
        vkGetImageMemoryRequirements(state->device, state->depth_images[i], &requirements);
        
        VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        
        if(!allocate(&state->memory_manager, state->device, requirements, memory_flags, &state->depth_image_allocations[i])) {
            println("Error: failed to allocate memory chunk for image.");
            free_null(state->depth_image_allocations);
            return false;
        }
        
        VkResult result = vkBindImageMemory(state->device, state->depth_images[i], state->depth_image_allocations[i].device_memory, state->depth_image_allocations[i].offset);
        if (result != VK_SUCCESS) {
            println("vkBindImageMemory returned (%s)", vk_error_code_str(result));
            free_null(state->depth_image_allocations);
            return false;
        }
    }
    
    if (!create_depth_image_views(state)) {
        return false;
    }
    
    return true;
}

inline bool create_semaphore(RendererState* state) {
    VkSemaphoreCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    state->present_semaphores = (VkSemaphore*)calloc(state->swapchain_image_count, sizeof(VkSemaphore));
    state->acquire_semaphores = (VkSemaphore*)calloc(state->swapchain_image_count, sizeof(VkSemaphore));
    
    for (int i = 0;i < state->swapchain_image_count;++i) {
        VkResult result = vkCreateSemaphore(state->device, &create_info, nullptr, &state->present_semaphores[i]);
        if (result != VK_SUCCESS) {
            println("vkCreateSemaphore returned (%s)", vk_error_code_str(result));
            return false;
        }
    }
    
    for (int i = 0;i < state->swapchain_image_count;++i) {
        VkResult result = vkCreateSemaphore(state->device, &create_info, nullptr, &state->acquire_semaphores[i]);
        if (result != VK_SUCCESS) {
            println("vkCreateSemaphore returned (%s)", vk_error_code_str(result));
            return false;
        }
    }
    return true;
}

inline bool create_submit_fences(RendererState* state) {
    VkFenceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    state->submit_fences = (VkFence*)calloc(state->swapchain_image_count, sizeof(VkFence));
    state->submissions = (CommandBufferSubmission*)calloc(state->swapchain_image_count, sizeof(CommandBufferSubmission));
    
    for (int i = 0;i < state->swapchain_image_count;++i) {
        VkResult result = vkCreateFence(state->device, &create_info, nullptr, &state->submit_fences[i]);
        if (result != VK_SUCCESS) {
            println("vkCreateFence returned (%s)", vk_error_code_str(result));
            return false;
        }
        state->submissions[i].fence = &state->submit_fences[i];
        state->submissions[i].command_buffer = 0;
    }
    
    return true;
}

inline bool create_acquire_fences(RendererState* state) {
    VkFenceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    
    state->acquire_fences = (VkFence*)calloc(state->swapchain_image_count, sizeof(VkFence));
    
    for (int i = 0;i < state->swapchain_image_count;++i) {
        VkResult result = vkCreateFence(state->device, &create_info, nullptr, &state->acquire_fences[i]);
        if (result != VK_SUCCESS) {
            println("vkCreateFence returned (%s)", vk_error_code_str(result));
            return false;
        }
    }
    
    return true;
}

inline bool create_fences(RendererState* state) {
    if (!create_submit_fences(state)) {
        return false;
    }
    
    if (!create_acquire_fences(state)) {
        return false;
    }
    
    return true;
}


inline bool create_command_pool(RendererState* state) {
    VkCommandPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    create_info.queueFamilyIndex = state->selection.graphics_queue_family_index;
    
    VkResult result = vkCreateCommandPool(state->device, &create_info, nullptr, &state->command_pool);
    if (result != VK_SUCCESS) {
        println("vkCreateCommandPool returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    return true;
}

inline bool create_descriptor_set_layout(RendererState* state) {
    // Create Camera descriptor set layout
    VkDescriptorSetLayoutBinding binding = {};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.bindingCount = 1;
    create_info.pBindings = &binding;
    
    VkResult result = vkCreateDescriptorSetLayout(state->device, &create_info, nullptr, &state->descriptor_set_layouts[CameraDescriptorSetLayout]);
    
    if (result != VK_SUCCESS) {
        println("vkCreateDescriptorSetLayout returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    // Create model transform descriptor set layout
    binding = {};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.bindingCount = 1;
    create_info.pBindings = &binding;
    
    result = vkCreateDescriptorSetLayout(state->device, &create_info, nullptr, &state->descriptor_set_layouts[TransformDescriptorSetLayout]);
    
    if (result != VK_SUCCESS) {
        println("vkCreateDescriptorSetLayout returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    return true;
}

inline bool create_pipeline_layout(RendererState* state) {
    VkPipelineLayoutCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    create_info.setLayoutCount = CountDescriptorSetLayout;
    create_info.pSetLayouts = state->descriptor_set_layouts;
    
    VkResult result = vkCreatePipelineLayout(state->device, &create_info, nullptr, &state->pipeline_layout);
    if (result != VK_SUCCESS) {
        println("vkCreatePipelineLayout returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    return true;
}

inline bool create_render_pass(RendererState* state) {
    VkAttachmentDescription attachment_descriptions[2] = {};
    attachment_descriptions[0].format = state->surface_format.format;
    attachment_descriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_descriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    attachment_descriptions[1].format = VK_FORMAT_D32_SFLOAT;
    attachment_descriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference attachment_reference = {};
    attachment_reference.attachment = 0;
    attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depth_attachment_reference = {};
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass_description = {};
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = nullptr;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &attachment_reference;
    subpass_description.pResolveAttachments = nullptr;
    subpass_description.pDepthStencilAttachment = &depth_attachment_reference;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments = nullptr;
    
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    VkRenderPassCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = array_size(attachment_descriptions);
    create_info.pAttachments = attachment_descriptions;
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass_description;
    create_info.dependencyCount = 1;
    create_info.pDependencies = &dependency;
    
    VkResult result = vkCreateRenderPass(state->device, &create_info, nullptr, &state->renderpass);
    if (result != VK_SUCCESS) {
        println("vkCreateRenderPass returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    return true;
}

inline bool create_graphics_pipeline(RendererState* state) {
    VkPipelineShaderStageCreateInfo stage_create_info[2] = {};
    stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    if (!get_shader_from_catalog("basic.vert", &state->shader_catalog, &stage_create_info[0].module)) {
        return false;
    }
    stage_create_info[0].pName = "main";
    
    stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    if (!get_shader_from_catalog("basic.frag", &state->shader_catalog, &stage_create_info[1].module)) {
        return false;
    }
    stage_create_info[1].pName = "main";
    
    VkVertexInputBindingDescription binding_description = {};
    binding_description.binding = 0;
    binding_description.stride = sizeof(Vertex);
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    VkVertexInputAttributeDescription attribute_description[4] = {};
    attribute_description[0].location = 0;
    attribute_description[0].binding = 0;
    attribute_description[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_description[0].offset = offsetof(Vertex, position);
    
    attribute_description[1].location = 1;
    attribute_description[1].binding = 0;
    attribute_description[1].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_description[1].offset = offsetof(Vertex, uv);
    
    attribute_description[2].location = 2;
    attribute_description[2].binding = 0;
    attribute_description[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_description[2].offset = offsetof(Vertex, normal);
    
    attribute_description[3].location = 3;
    attribute_description[3].binding = 0;
    attribute_description[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_description[3].offset = offsetof(Vertex, color);
    
    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = array_size(attribute_description);
    vertex_input_state_create_info.pVertexAttributeDescriptions = attribute_description;
    
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
    input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width  = (f32)state->swapchain_extent.width;
    viewport.height = (f32)state->swapchain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = state->swapchain_extent;
    
    VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports = &viewport;
    viewport_state_create_info.scissorCount = 1;
    viewport_state_create_info.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {};
    rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.depthClampEnable = VK_FALSE;
    rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_state_create_info.depthBiasEnable = VK_FALSE;
    rasterization_state_create_info.lineWidth = 1.0f;
    
    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {};
    multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {};
    depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    
    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    
    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.logicOpEnable = VK_FALSE;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &color_blend_attachment;
    
    VkGraphicsPipelineCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_info.stageCount = 2;
    create_info.pStages = stage_create_info;
    create_info.pVertexInputState = &vertex_input_state_create_info;
    create_info.pInputAssemblyState = &input_assembly_state_create_info;
    create_info.pViewportState = &viewport_state_create_info;
    create_info.pRasterizationState = &rasterization_state_create_info;
    create_info.pMultisampleState = &multisample_state_create_info;
    create_info.pDepthStencilState = &depth_stencil_state_create_info;
    create_info.pColorBlendState = &color_blend_state_create_info;
    create_info.layout = state->pipeline_layout;
    create_info.renderPass = state->renderpass;
    create_info.subpass = 0;
    
    VkResult result = vkCreateGraphicsPipelines(state->device, VK_NULL_HANDLE, 1, &create_info, nullptr, &state->pipeline);
    if (result != VK_SUCCESS) {
        println("vkCreateGraphicsPipelines returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    return true;
}

inline bool create_framebuffers(RendererState* state) {
    VkFramebufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create_info.renderPass = state->renderpass;
    create_info.attachmentCount = 2;
    create_info.width = state->swapchain_extent.width;
    create_info.height = state->swapchain_extent.height;
    create_info.layers = 1;
    
    state->framebuffers = (VkFramebuffer*)calloc(state->swapchain_image_count, sizeof(VkFramebuffer));
    
    for (int i = 0;i < state->swapchain_image_count;++i) {
        VkImageView attachments[2] = {state->swapchain_image_views[i], state->depth_image_views[i] };
        create_info.pAttachments = attachments;
        
        VkResult result = vkCreateFramebuffer(state->device, &create_info, nullptr, &state->framebuffers[i]);
        if (result != VK_SUCCESS) {
            println("vkCreateFramebuffer returned (%s)", vk_error_code_str(result));
            return false;
        }
    }
    
    return true;
}

inline bool create_descriptor_pool(RendererState* state) {
    VkDescriptorPoolSize pool_sizes[3] = {};
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = 256;
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    pool_sizes[1].descriptorCount = 256;
    pool_sizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[2].descriptorCount = 256;
    
    VkDescriptorPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.maxSets = 256 * array_size(pool_sizes);
    create_info.poolSizeCount = array_size(pool_sizes);
    create_info.pPoolSizes = pool_sizes;
    
    VkResult result = vkCreateDescriptorPool(state->device, &create_info, nullptr, &state->descriptor_pool);
    if (result != VK_SUCCESS) {
        println("vkCreateDescriptorPool returned (%s)", vk_error_code_str(result));
        return false;
    }
    
    return true;
}

inline void destroy_window(RendererState* state, bool verbose) {
    if (state->window) {
        if (verbose) {
            println("Destroying window");
        }
        
        glfwDestroyWindow(state->window);
        
        if (verbose) {
            println("");
        }
    }
}

inline void destroy_framebuffers(RendererState* state, bool verbose) {
    if (verbose) {
        println("Destroying framebuffers");
    }
    if (state->framebuffers) {
        for (int i = 0;i < state->swapchain_image_count;++i) {
            if (verbose) {
                println("    Destroying framebuffer (%p)", state->framebuffers[i]);
            }
            vkDestroyFramebuffer(state->device, state->framebuffers[i], nullptr);
        }
        
        free_null(state->framebuffers);
    }
    if (verbose) {
        println("");
    }
}


inline void destroy_pipeline(RendererState* state, bool verbose) {
    if (state->pipeline) {
        if (verbose) {
            println("Destroying graphics pipeline (%p)", state->pipeline);
        }
        vkDestroyPipeline(state->device, state->pipeline, nullptr);
        state->pipeline = 0;
        if (verbose) {
            println("");
        }
    }
}

inline void destroy_renderpass(RendererState* state, bool verbose) {
    if (state->renderpass) {
        if (verbose) {
            println("Destroying render pass (%p)", state->renderpass);
        }
        vkDestroyRenderPass(state->device, state->renderpass, nullptr);
        if (verbose) {
            println("");
        }
    }
}

inline void destroy_descriptor_set_layout(RendererState* state, bool verbose) {
    if (verbose) {
        println("Destroying descriptor set layouts");
    }
    for (int i = 0;i < CountDescriptorSetLayout;++i) {
        if (state->descriptor_set_layouts[i]) {
            if (verbose) {
                println("    Destroying decriptor set layout (%p)", state->descriptor_set_layouts[i]);
            }
            vkDestroyDescriptorSetLayout(state->device, state->descriptor_set_layouts[i], nullptr);
        }
    }
    if (verbose) {
        println("");
    }
}

inline void destroy_descriptor_pool(RendererState* state, bool verbose) {
    if (state->descriptor_pool) {
        if (verbose) {
            println("Destroying decriptor pool (%p)", state->descriptor_pool);
        }
        vkDestroyDescriptorPool(state->device, state->descriptor_pool, nullptr);
        if (verbose) {
            println("");
        }
    }
    
}

inline void destroy_pipeline_layout(RendererState* state, bool verbose) {
    if (state->pipeline_layout) {
        if (verbose) {
            println("Destroying pipeline layout (%p)", state->pipeline_layout);
        }
        vkDestroyPipelineLayout(state->device, state->pipeline_layout, nullptr);
        if (verbose) {
            println("");
        }
    }
}

inline void destroy_command_pool(RendererState* state, bool verbose) {
    if (state->command_pool) {
        if (verbose) {
            println("Destroying command pool (%p)", state->command_pool);
        }
        vkDestroyCommandPool(state->device, state->command_pool, nullptr);
        if (verbose) {
            println("");
        }
    }
}

inline void destroy_fences(RendererState* state, bool verbose) {
    if (verbose) {
        println("Destroying fences");
    }
    if (state->submit_fences) {
        for (int i = 0;i < state->swapchain_image_count;++i) {
            println("    Destroying submit fence (%p)", state->submit_fences[i]);
            vkDestroyFence(state->device, state->submit_fences[i], nullptr);
        }
        
        free_null(state->submit_fences);
    }
    
    if (state->acquire_fences) {
        for (int i = 0;i < state->swapchain_image_count;++i) {
            println("    Destroying acquire fence (%p)", state->acquire_fences[i]);
            vkDestroyFence(state->device, state->acquire_fences[i], nullptr);
        }
        
        free_null(state->acquire_fences);
    }
    if (verbose) {
        println("");
    }
}

inline void destroy_submissions(RendererState* state, bool verbose) {
    if (verbose) {
        println("Destroying submissions");
    }
    if (state->submissions) {
        if (verbose) {
            println("    Freeing submissions");
        }
        free_null(state->submissions);
    }
    if (verbose) {
        println("");
    }
}

inline void destroy_semaphores(RendererState* state, bool verbose) {
    if (verbose) {
        println("Destroying semaphores");
    }
    if (state->present_semaphores) {
        for (int i = 0;i < state->swapchain_image_count;++i) {
            if (verbose) {
                println("    Destroying present semaphore (%p)", state->present_semaphores[i]);
            }
            vkDestroySemaphore(state->device, state->present_semaphores[i], nullptr);
        }
        
        free_null(state->present_semaphores);
    }
    
    if (state->acquire_semaphores) {
        for (int i = 0;i < state->swapchain_image_count;++i) {
            if (verbose) {
                println("    Destroying acquire semaphore (%p)", state->acquire_semaphores[i]);
            }
            vkDestroySemaphore(state->device, state->acquire_semaphores[i], nullptr);
        }
        
        free_null(state->acquire_semaphores);
    }
    
    if (verbose) {
        println("");
    }
}

inline void destroy_depth_images(RendererState* state, bool verbose) {
    if (verbose) {
        println("Destroying depth images");
    }
    if (state->depth_image_views) {
        for (int i = 0;i < state->swapchain_image_count;++i) {
            if (verbose) {
                println("    Destroying depth image view (%p)", state->depth_image_views[i]);
            }
            vkDestroyImageView(state->device, state->depth_image_views[i], nullptr);
        }
        
        free_null(state->depth_image_views);
    }
    
    if (state->depth_images) {
        for (int i = 0;i < state->swapchain_image_count;++i) {
            if (verbose) {
                println("    Destroying depth image (%p)", state->depth_images[i]);
            }
            vkDestroyImage(state->device, state->depth_images[i], nullptr);
        }
        
        free_null(state->depth_images);
    }
    
    if (state->depth_image_allocations) {
        for (int i = 0;i < state->swapchain_image_count;++i) {
            if (verbose) {
                println("    Freeing depth image allocation (%p)", &state->depth_image_allocations[i]);
            }
            free(&state->memory_manager, &state->depth_image_allocations[i]);
        }
        free_null(state->depth_image_allocations);
    }
    
    if (verbose) {
        println("");
    }
}

inline void destroy_swapchain_image_views(RendererState* state, bool verbose) {
    if (verbose) {
        println("Destroying swapchain image views");
    }
    if (state->swapchain_image_views) {
        for (int i = 0;i < state->swapchain_image_count;++i) {
            if (verbose) {
                println("    Destroying swapchain image view (%p)", state->swapchain_image_views[i]);
            }
            vkDestroyImageView(state->device, state->swapchain_image_views[i], nullptr);
        }
        free_null(state->swapchain_image_views);
    }
    if (verbose) {
        println("");
    }
}

inline void destroy_swapchain(RendererState* state, bool verbose) {
    if (verbose) {
        println("Destroying swapchain resources");
    }
    if (state->swapchain) {
        if (verbose) {
            println("    Destroying swapchain (%p)", state->swapchain);
        }
        vkDestroySwapchainKHR(state->device, state->swapchain, nullptr);
    }
    
    if (state->swapchain_images) {
        if (verbose) {
            println("    Freeing swapchain images");
        }
        free_null(state->swapchain_images);
    }
    if (verbose) {
        println("");
    }
}

inline void destroy_device(RendererState* state, bool verbose) {
    if (state->device) {
        if (verbose) {
            println("Destroying device (%p)", state->device);
        }
        vkDestroyDevice(state->device, nullptr);
    }
    if (verbose) {
        println("");
    }
}

inline void destroy_surface(RendererState* state, bool verbose) {
    if (state->surface) {
        if (verbose) {
            println("Destroying surface (%p)", state->surface);
        }
        vkDestroySurfaceKHR(state->instance, state->surface, nullptr);
    }
    if (verbose) {
        println("");
    }
}

inline void destroy_instance(RendererState* state, bool verbose) {
    if (state->instance) {
        if (verbose) {
            println("Destroying instance (%p)", state->instance);
        }
        vkDestroyInstance(state->instance, nullptr);
    }
    if (verbose) {
        println("");
    }
}
