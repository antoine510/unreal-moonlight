#pragma once

#include <functional>
#include <thread>
#include <atomic>

extern void Moonlight::LogMessage(const char* msg);

template <typename T>
bool IsValidT(const T* Test) {
	return ::IsValid(Test) && Test->IsValid();
}

namespace Moonlight {

class AsyncEngine {
public:

	void SetTask(std::function<int()>&& asyncF, std::function<void(int)>&& syncF);
	void StopTask();
	void Tick();

	bool IsTickable() const { return _running; }
	bool IsAvailable() const { return !_running; }

private:
	std::thread _thread;
	std::atomic<bool> _done;
	bool _running = false;

	int resultCode;

	std::function<void(int)> _syncF;
};

}


