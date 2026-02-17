#pragma once

#include <wayland-client.h>
#include <functional>
#include <string>
#include <vector>
#include "Zeta/events.hpp"

struct wl_display;
struct wl_surface;
struct wl_registry;
struct xdg_wm_base;
struct xdg_surface;
struct xdg_toplevel;
struct wl_seat;
struct wl_keyboard;

namespace Zeta {

class Window {
public:
    using ResizeCallback = std::function<void(uint32_t, uint32_t)>;
    using KeyCallback = std::function<void(uint32_t, bool)>;

    Window(uint32_t width, uint32_t height);
    ~Window();

    // Prevent copying due to raw Wayland handles
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    uint32_t m_width = 600;
    uint32_t m_height = 600;

    void poll_events();

    void update(Zeta::Event e);
    
    // Setters for App-level callbacks
    void set_resize_callback(ResizeCallback cb) { m_onResize = std::move(cb); }
    void set_key_callback(KeyCallback cb) { m_onKey = std::move(cb); }

    // Getters for Vulkan Surface creation
    struct wl_display* get_display() const { return m_display; }
    struct wl_surface* get_surface() const { return m_surface; }

    // Static Callback Handlers (Wayland interface)
    static void handle_registry_global(void* data, struct wl_registry* reg, uint32_t id, const char* intf, uint32_t ver);
    static void handle_seat_capabilities(void* data, struct wl_seat* seat, uint32_t caps);
    static void handle_xdg_wm_base_ping(void* data, struct xdg_wm_base* wm, uint32_t serial);
    static void handle_xdg_surface_configure(void* data, struct xdg_surface* surf, uint32_t serial);
    static void handle_xdg_toplevel_configure(void* data, struct xdg_toplevel* top, int32_t w, int32_t h, struct wl_array* states);
    static void handle_keyboard_key(void* data, struct wl_keyboard* kbd, uint32_t ser, uint32_t time, uint32_t key, uint32_t state);

private:
    // Wayland State
    struct wl_display*   m_display = nullptr;
    struct wl_registry*  m_registry = nullptr;
    struct wl_compositor* m_compositor = nullptr;
    struct wl_surface*   m_surface = nullptr;
    struct wl_seat*       m_seat = nullptr;
    struct wl_keyboard*   m_keyboard = nullptr;
    
    // XDG Shell State
    struct xdg_wm_base*  m_xdg_wm_base = nullptr;
    struct xdg_surface*  m_xdg_surface = nullptr;
    struct xdg_toplevel* m_xdg_toplevel = nullptr;

    // Callbacks
    ResizeCallback m_onResize;
    KeyCallback    m_onKey;

    // Internal initialization helpers
    void init_wayland(uint32_t w, uint32_t h);

  
};

} // namespace Zeta