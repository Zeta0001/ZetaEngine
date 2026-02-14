#include <Zeta/time.hpp>

Fps::Fps() {};
Fps::~Fps() {};

int elapsedTime = 0;

void Fps::begin() {
	m_start = std::chrono::steady_clock::now();
};

void Fps::end() {
    auto now = std::chrono::steady_clock::now();
    
    // 1. Get exact work time in microseconds (float for precision)
    float workTimeUs = std::chrono::duration<float, std::micro>(now - m_start).count();
    float targetUs = 1000000.0f / m_limit;

    if (workTimeUs < targetUs) {
        float delayUs = targetUs - workTimeUs;
        
        // 2. Sleep until EXACTLY one frame duration from the start
        std::this_thread::sleep_until(m_start + std::chrono::microseconds(static_cast<long long>(targetUs)));
        
        m_fps = m_limit; // We hit the cap
    } else {
        // 3. We ran slow, calculate actual FPS
        m_fps = 1000000.0f / workTimeUs;
    }
}

float Fps::getFps() {
	return m_fps;
};


void Timer::begin(){
	m_start = std::chrono::steady_clock::now();
};
void Timer::end(){
	m_frame = std::chrono::steady_clock::now();
	m_frameTime = std::chrono::duration_cast<std::chrono::microseconds>(m_frame - m_start).count();
};
float Timer::getFps(){
	return m_frameTime;
};
