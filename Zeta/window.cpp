//#define VK_USE_PLATFORM_WAYLAND_KHR
#include "Zeta/window.hpp"
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <poll.h>

namespace Zeta {

// Define the static listeners
static const struct wl_registry_listener registry_listener = {
    Window::global_registry_handler,
    Window::global_registry_remover
};

// 1. Define the functions FIRST
    void Window::handle_xdg_surface_configure(void* data, struct xdg_surface* surface, uint32_t serial) {
        auto* win = static_cast<Window*>(data);
        win->m_pending_serial = serial;
        win->m_resize_pending = true;
    }

    void Window::handle_toplevel_configure(void* data, xdg_toplevel* toplevel, 
        int32_t width, int32_t height, wl_array* states) {
        auto* zetaWindow = static_cast<Zeta::Window*>(data);

            // Wayland sends 0,0 if it wants the client to decide
            if (width > 0 && height > 0) {
                if (zetaWindow->m_onResize) {
                zetaWindow->m_onResize(static_cast<uint32_t>(width), 
                    static_cast<uint32_t>(height));
                }
            }
        }

    // 2. Define the listener structs SECOND (now they can see the functions)
    static const struct xdg_surface_listener surface_listener = { 
        .configure = Window::handle_xdg_surface_configure 
    };

    static const struct xdg_toplevel_listener toplevel_listener = { 
        .configure = Window::handle_toplevel_configure, 
        .close = [](void* d, struct xdg_toplevel* t){ 
            static_cast<Window*>(d)->m_should_close = true; 
        } 
    };
void Window::config(){
    xdg_surface_ack_configure(m_xdg_surface, m_pending_serial);
};

Window::Window(int width, int height, const std::string& title) {
    m_width = width;
    m_height = height;
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
    if (!m_surface) throw std::runtime_error("Failed to create wl_surface");

    // Setup XDG Surface
    m_xdg_surface = xdg_wm_base_get_xdg_surface(m_xdg_wm_base, m_surface);
    // static const struct xdg_surface_listener xdg_surface_listener = {
    //     [](void* data, struct xdg_surface* surface, uint32_t serial) {
    //         xdg_surface_ack_configure(surface, serial);
    //         auto* self = static_cast<Zeta::Window*>(data);
    //         wl_surface_commit(self->get_surface());
    //     }
    // };
    xdg_surface_add_listener(m_xdg_surface, &surface_listener, this);

    // Setup Toplevel
    m_xdg_toplevel = xdg_surface_get_toplevel(m_xdg_surface);
    xdg_toplevel_set_title(m_xdg_toplevel, title.c_str());

    // static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    //     [](void* data, struct xdg_toplevel*, int32_t width, int32_t height, struct wl_array*){}, // configure
    //     [](void* data, struct xdg_toplevel*){ // close
    //         auto* self = static_cast<Zeta::Window*>(data);
    //         self->m_should_close = true;
    //     }
    // };
    xdg_toplevel_add_listener(m_xdg_toplevel, &toplevel_listener, this);


    // Initial commit
    wl_surface_set_buffer_scale(m_surface, 1); 
    wl_surface_commit(m_surface);

    // Roundtrip 2: Sync so handles are valid before Render class starts
    wl_display_roundtrip(m_display);}

    void Window::poll_events() {
        // 1. Force-send all pending requests to the compositor (like ACK or Commit)
        wl_display_flush(m_display);
    
        // 2. Non-blocking check for events from the compositor
        // If you don't do this, Vulkan will hang waiting for a "Buffer Release"
        while (wl_display_prepare_read(m_display) != 0) {
            wl_display_dispatch_pending(m_display);
        }
        
        // 3. Read and Dispatch
        struct pollfd pfd = { .fd = wl_display_get_fd(m_display), .events = POLLIN };
        if (poll(&pfd, 1, 0) > 0) {
            wl_display_read_events(m_display);
            wl_display_dispatch_pending(m_display);
        } else {
            wl_display_cancel_read(m_display);
        }
}

Window::~Window() {
    if (m_xdg_toplevel) xdg_toplevel_destroy(m_xdg_toplevel);
    if (m_xdg_surface) xdg_surface_destroy(m_xdg_surface);
    if (m_xdg_wm_base) xdg_wm_base_destroy(m_xdg_wm_base);
    if (m_surface) wl_surface_destroy(m_surface);
    if (m_compositor) wl_compositor_destroy(m_compositor);
    if (m_registry) wl_registry_destroy(m_registry);
    if (m_display) wl_display_disconnect(m_display);
}

void Window::init() {    
   
    }

void Window::global_registry_handler(void *data, struct wl_registry *registry, 
                                     uint32_t id, const char *interface, uint32_t version) {
    auto* self = static_cast<Window*>(data);

    if (std::strcmp(interface, "wl_compositor") == 0) {
        self->m_compositor = (wl_compositor*)wl_registry_bind(registry, id, &wl_compositor_interface, 4);
    } 
    else if (std::strcmp(interface, "xdg_wm_base") == 0) {
        self->m_xdg_wm_base = (xdg_wm_base*)wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
        
        static const struct xdg_wm_base_listener shell_listener = {
            .ping = [](void* data, struct xdg_wm_base* shell, uint32_t serial) {
                xdg_wm_base_pong(shell, serial); // This keeps the window "alive"
            }
        };
        xdg_wm_base_add_listener(self->m_xdg_wm_base, &shell_listener, nullptr);
    }
}

void Window::global_registry_remover(void*, struct wl_registry*, uint32_t) {}

void Window::set_fullscreen(bool fullscreen) {
    if (fullscreen) {
        // NULL tells the compositor to pick the current/default output
        xdg_toplevel_set_fullscreen(m_xdg_toplevel, nullptr);
    } else {
        xdg_toplevel_unset_fullscreen(m_xdg_toplevel);
    }
    // Commit the surface to apply the state change
    wl_surface_commit(m_surface);
}

void Window::acknowledge_resize() {
    if (m_xdg_surface && m_pending_serial != 0) {
        xdg_surface_ack_configure(m_xdg_surface, m_pending_serial);
        m_pending_serial = 0; // Clear after use
    }
}

} // namespace Zeta