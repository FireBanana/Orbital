#ifndef GLOBAL_H
#define GLOBAL_H

#include "graphics.h"

#include <atomic>
#include <thread>

// C++17 inline variables
namespace Global {
constexpr uint32_t QUEUE_INDEX = 0;
constexpr uint32_t WIDTH = 1920;
constexpr uint32_t HEIGHT = 969;
constexpr VkFormat FORMAT = VK_FORMAT_B8G8R8A8_SRGB;
constexpr VkFormat DEPTH_FORMAT = VK_FORMAT_D16_UNORM;
constexpr uint32_t SWAPCHAIN_SIZE = 3;
constexpr uint32_t FRAMES_IN_FLIGHT = 2;

inline GLFWwindow *g_window;

inline VkInstance g_instance;
inline VkPhysicalDevice g_physical_device;
inline VkDevice g_device;
inline VkQueue g_queue;
inline VkSurfaceKHR g_surface;
inline VkSwapchainKHR g_swapchain;

inline glm::vec3 g_camera_position = glm::vec3(0., 0., 3.0);
inline glm::mat4 g_model = glm::mat4(1.0f);
inline glm::mat4 g_view = glm::lookAtRH(g_camera_position, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
inline glm::mat4 g_projection = glm::perspectiveZO(glm::radians(60.0f),
                                                   (float) WIDTH / (float) HEIGHT,
                                                   0.1f,
                                                   1000.0f);

inline UniformConstants g_constants = {g_model, g_view, g_projection, glm::vec4(1.)};

inline std::vector<VkImage> g_swapchain_images;
inline std::vector<VkImageView> g_swapchain_views;
inline Texture g_depth;

// Per frame data
inline std::vector<VkSemaphore> g_semaphores;
inline std::array<Frame, SWAPCHAIN_SIZE> g_frame_data{};

inline std::thread g_gui_thread;
inline std::atomic<bool> g_window_running = true;

}; // namespace Global

#endif // GLOBAL_H
