#pragma once
#include <cstdint>
#include <mutex>
#include <queue>
#include <variant>

namespace Zeta {

struct QuitEvent {};
struct ResizeEvent { uint32_t width, height; };
struct KeyEvent { uint32_t key; bool pressed; };

using Event = std::variant<QuitEvent, ResizeEvent, KeyEvent>;



class EventBus {
public:
    // Push can be called by any thread (Input, Network, etc.)
    void push(Event e) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(e);
    }

    // Processed usually by the Main Thread
    bool poll(Event& out_event) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) return false;
        
        out_event = m_queue.front();
        m_queue.pop();
        return true;
    }

private:
    std::queue<Event> m_queue;
    std::mutex m_mutex;
};

class InputHandler {
    public:
        InputHandler(EventBus& bus) : m_bus(bus) {}
        
        void on_key_callback(uint32_t key) {
            m_bus.push(KeyEvent{key, true});
        }
    private:
        EventBus& m_bus;
    };
}
