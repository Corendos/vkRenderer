#ifndef __WINDOW_USER_DATA_H__
#define __WINDOW_USER_DATA_H__

struct Input;

struct WindowUserData {
    Input* input;
    bool swapchain_need_recreation = false;
};

#endif
