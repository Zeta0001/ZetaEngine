#pragma once
#include <string>
#include <cstdint>
#include <iostream>

// 1. Forward declare in the GLOBAL namespace
struct wl_display;
struct wl_registry;
struct wl_compositor;
struct wl_surface;
struct xdg_wm_base;
struct xdg_surface;
struct xdg_toplevel;

namespace Zeta {

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    bool shouldClose() const { return m_shouldClose; }
    void pollEvents();

    // 2. Temporarily move these to public so the listener can see them
    static void global_registry_handler(void *data, struct wl_registry *registry, 
                                        uint32_t id, const char *interface, uint32_t version);
    static void global_registry_remover(void *data, struct wl_registry *registry, uint32_t id);

    struct wl_display* get_display() const { return m_display; }
    struct wl_surface* get_surface() const { return m_surface; }

private:
    struct wl_display* m_display = nullptr;
    struct wl_registry* m_registry = nullptr;
    struct wl_compositor* m_compositor = nullptr;
    struct wl_shm* m_shm = nullptr;       
    struct wl_surface* m_surface = nullptr;
    struct xdg_wm_base* m_xdg_wm_base = nullptr;
    struct xdg_surface* m_xdg_surface = nullptr;
    struct xdg_toplevel* m_xdg_toplevel = nullptr;

    bool m_shouldClose = false;
};

} // namespace Zeta