#pragma once
#include <chrono>
#include <thread>

class Fps {
private:

	std::chrono::steady_clock::time_point m_start = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point m_frame = std::chrono::steady_clock::now();
	float m_frameTime = 0;
	float m_fps = 0.0f;
	float m_delay = 0.0f;

public:
	Fps();
	~Fps();

	void begin();
	void end();

	float getFps();

	int m_limit = 250;
};

class Timer {
private:
	std::chrono::steady_clock::time_point m_start = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point m_frame = std::chrono::steady_clock::now();
	float m_frameTime = 0;
public:
	void begin();
	void end();
	float getFps();
};