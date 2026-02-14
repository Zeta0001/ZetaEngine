#include "Zeta/window.hpp"
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <cstring>
#include <stdexcept>
#include <iostream>

namespace Zeta {

// Define the static listener struct
static const struct wl_registry_listener registry_listener = {
    Window::global_registry_handler,
    Window::global_registry_remover
};

Window::Window(int width, int height, const std::string& title) {
    m_display = wl_display_connect(nullptr);
    if (!m_display) throw std::runtime_error("Failed to connect to Wayland display");

    m_registry = wl_display_get_registry(m_display);
    wl_registry_add_listener(m_registry, &registry_listener, this);

    // Roundtrip 1: Find the globals (compositor, xdg_wm_base)
    wl_display_roundtrip(m_display);

    if (!m_compositor || !m_xdg_wm_base) 
        throw std::runtime_error("Essential Wayland globals not found");

    // Create the surface
    m_surface = wl_compositor_create_surface(m_compositor); 

    if (!m_surface) {
        throw std::runtime_error("Failed to create Wayland surface");
    }

    // Setup XDG Shell
    m_xdg_surface = xdg_wm_base_get_xdg_surface(m_xdg_wm_base, m_surface);
    static const struct xdg_surface_listener xdg_surface_listener = {
        .configure = [](void* data, struct xdg_surface* surface, uint32_t serial) {
            xdg_surface_ack_configure(surface, serial);
        }
    };
    xdg_surface_add_listener(m_xdg_surface, &xdg_surface_listener, this);

    m_xdg_toplevel = xdg_surface_get_toplevel(m_xdg_surface);
    xdg_toplevel_set_title(m_xdg_toplevel, title.c_str());

    // Final commit to notify the compositor we exist
    wl_surface_commit(m_surface);

    // Roundtrip 2: Sync with compositor so m_surface is valid for Vulkan
    wl_display_roundtrip(m_display);
}

// 1. You MUST define the destructor
Window::~Window() {
    if (m_xdg_toplevel) xdg_toplevel_destroy(m_xdg_toplevel);
    if (m_xdg_surface) xdg_surface_destroy(m_xdg_surface);
    if (m_surface) wl_surface_destroy(m_surface);
    if (m_display) {
        wl_display_disconnect(m_display);
    }
}

// 2. You MUST define pollEvents
void Window::pollEvents() {
    if (m_display) {
        wl_display_dispatch_pending(m_display);
        wl_display_flush(m_display);
    }
}

void Window::global_registry_handler(void *data, struct wl_registry *registry, 
    uint32_t id, const char *interface, uint32_t version) {
    auto* self = static_cast<Window*>(data);

    std::cout << "Found interface: " << interface << " (v" << version << ")" << std::endl;

    if (std::strcmp(interface, "wl_compositor") == 0) {
    // Bind to version 4 (widely supported)
    self->m_compositor = (wl_compositor*)wl_registry_bind(registry, id, &wl_compositor_interface, 4);
    } 
    else if (std::strcmp(interface, "xdg_wm_base") == 0) {
    // Bind to version 1
    self->m_xdg_wm_base = (xdg_wm_base*)wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);

    // Add the required listener for pings
    static const struct xdg_wm_base_listener wm_base_listener = {
    .ping = [](void* data, struct xdg_wm_base* wm_base, uint32_t serial) {
    xdg_wm_base_pong(wm_base, serial);
    }
    };
    xdg_wm_base_add_listener(self->m_xdg_wm_base, &wm_base_listener, nullptr);
    }

}

void Window::global_registry_remover(void *data, struct wl_registry *registry, uint32_t id) {
    // Usually empty for basic apps
}

} // namespace Zeta