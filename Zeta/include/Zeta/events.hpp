#pragma once
#include <cstdint>
#include <mutex>
#include <queue>
#include <variant>

namespace Zeta {

struct QuitEvent {};
struct ResizeEvent { uint32_t w, h; };
struct KeyEvent { uint32_t key; bool pressed; };

// The "Base" variant that Zeta knows how to process
using CoreEvent = std::variant<QuitEvent, ResizeEvent, KeyEvent>;


template<typename EventVariant>
class EventBus {
public:
    void push(EventVariant e) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(e));
    }

    std::optional<EventVariant> poll() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) return std::nullopt;
        
        EventVariant e = std::move(m_queue.front());
        m_queue.pop();
        return e;
    }

private:
    std::queue<EventVariant> m_queue;
    std::mutex m_mutex;
};


template<typename T>
class InputHandler {
public:
    InputHandler(EventBus<T>& bus) : m_bus(bus) {}
    void on_key_callback(uint32_t key, bool pressed) {
        m_bus.push(KeyEvent{key, pressed});
    }
private:
    EventBus<T>& m_bus;
};
}
