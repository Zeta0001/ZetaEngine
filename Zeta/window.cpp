#include "Zeta/window.hpp"
#include <cstring>
#include <stdexcept>
#include "xdg-shell-client-protocol.h"

namespace Zeta {

// Listener Definitions
static const struct wl_registry_listener registry_listener = { .global = Window::handle_registry_global, .global_remove = [](auto...){} };
static const struct xdg_wm_base_listener wm_base_listener = { .ping = Window::handle_xdg_wm_base_ping };
static const struct xdg_surface_listener surface_listener = { .configure = Window::handle_xdg_surface_configure };
static const struct xdg_toplevel_listener toplevel_listener = { .configure = Window::handle_xdg_toplevel_configure, .close = [](auto...){} };
static const struct wl_seat_listener seat_listener = { .capabilities = Window::handle_seat_capabilities, .name = [](auto...){} };
static const struct wl_keyboard_listener keyboard_listener = {
    .keymap = [](auto...){}, .enter = [](auto...){}, .leave = [](auto...){},
    .key = Window::handle_keyboard_key,
    .modifiers = [](auto...){}, .repeat_info = [](auto...){}
};

Window::Window(uint32_t width, uint32_t height) : m_width(width), m_height(height) {
    m_display = wl_display_connect(nullptr);
    if (!m_display) throw std::runtime_error("Failed to connect to Wayland display");

    m_registry = wl_display_get_registry(m_display);
    wl_registry_add_listener(m_registry, &registry_listener, this);
    
    // Sync to bind globals (compositor, shell, seat)
    wl_display_roundtrip(m_display);

    if (!m_compositor || !m_xdg_wm_base) throw std::runtime_error("Wayland: Missing required protocols");

    m_surface = wl_compositor_create_surface(m_compositor);
    m_xdg_surface = xdg_wm_base_get_xdg_surface(m_xdg_wm_base, m_surface);
    xdg_surface_add_listener(m_xdg_surface, &surface_listener, this);

    m_xdg_toplevel = xdg_surface_get_toplevel(m_xdg_surface);
    xdg_toplevel_add_listener(m_xdg_toplevel, &toplevel_listener, this);
    xdg_toplevel_set_title(m_xdg_toplevel, "Zeta Engine");

    wl_surface_commit(m_surface);
    wl_display_roundtrip(m_display);
}

Window::~Window() {
    if (m_keyboard) wl_keyboard_destroy(m_keyboard);
    if (m_seat) wl_seat_destroy(m_seat);
    if (m_xdg_toplevel) xdg_toplevel_destroy(m_xdg_toplevel);
    if (m_xdg_surface) xdg_surface_destroy(m_xdg_surface);
    if (m_xdg_wm_base) xdg_wm_base_destroy(m_xdg_wm_base);
    if (m_surface) wl_surface_destroy(m_surface);
    if (m_compositor) wl_compositor_destroy(m_compositor);
    if (m_registry) wl_registry_destroy(m_registry);
    if (m_display) wl_display_disconnect(m_display);
}

void Window::poll_events() {
    wl_display_dispatch_pending(m_display);
}

// Static Handlers
void Window::handle_registry_global(void* data, struct wl_registry* reg, uint32_t id, const char* intf, uint32_t ver) {
    auto* self = static_cast<Window*>(data);
    if (strcmp(intf, "wl_compositor") == 0) {
        self->m_compositor = (wl_compositor*)wl_registry_bind(reg, id, &wl_compositor_interface, 4);
    } else if (strcmp(intf, "xdg_wm_base") == 0) {
        self->m_xdg_wm_base = (xdg_wm_base*)wl_registry_bind(reg, id, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(self->m_xdg_wm_base, &wm_base_listener, self);
    } else if (strcmp(intf, "wl_seat") == 0) {
        self->m_seat = (wl_seat*)wl_registry_bind(reg, id, &wl_seat_interface, 7);
        wl_seat_add_listener(self->m_seat, &seat_listener, self);
    }
}

void Window::handle_seat_capabilities(void* data, struct wl_seat* seat, uint32_t caps) {
    auto* self = static_cast<Window*>(data);
    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
        self->m_keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(self->m_keyboard, &keyboard_listener, self);
    }
}

void Window::handle_keyboard_key(void* data, struct wl_keyboard*, uint32_t, uint32_t, uint32_t key, uint32_t state) {
    auto* self = static_cast<Window*>(data);
    if (self->m_onKey) self->m_onKey(key, state == WL_KEYBOARD_KEY_STATE_PRESSED);
}

void Window::handle_xdg_surface_configure(void* data, struct xdg_surface* surf, uint32_t serial) {
    xdg_surface_ack_configure(surf, serial);
}

void Window::handle_xdg_toplevel_configure(void* data, struct xdg_toplevel*, int32_t w, int32_t h, struct wl_array*) {
    auto* self = static_cast<Window*>(data);
    if (w > 0 && h > 0 && self->m_onResize) self->m_onResize(w, h);
}

void Window::handle_xdg_wm_base_ping(void*, struct xdg_wm_base* wm, uint32_t serial) {
    xdg_wm_base_pong(wm, serial);
}

} // namespace Zeta