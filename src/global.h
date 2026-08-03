#ifndef GLOBAL_H
#define GLOBAL_H

#include "graphics.h"

#include <atomic>
#include <thread>

// C++17 inline variables
namespace Global {
constexpr uint32_t QUEUE_INDEX = 0;
constexpr VkFormat FORMAT = VK_FORMAT_B8G8R8A8_UNORM;
constexpr VkFormat RENDER_TARGET_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;
constexpr VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;
//constexpr uint32_t FRAMES_IN_FLIGHT = 2;

inline uint32_t g_width = 960;
inline uint32_t g_height = 960;

inline GLFWwindow *g_window;

inline VkInstance g_instance;
inline VkPhysicalDevice g_physical_device;
inline VkDevice g_device;
inline VkQueue g_queue;
inline VkSurfaceKHR g_surface;
inline VkSwapchainKHR g_swapchain = VK_NULL_HANDLE;
inline std::vector<Texture> g_render_targets;
inline bool g_swapchain_dirty = false;

inline std::vector<VkImage> g_swapchain_images;
inline std::vector<VkImageView> g_swapchain_views;

// Per frame data
inline std::vector<VkSemaphore> g_semaphores;
inline std::vector<Frame> g_frame_data{};

inline std::thread g_gui_thread;
inline std::atomic<bool> g_window_running = true;

}; // namespace Global

#endif // GLOBAL_H
