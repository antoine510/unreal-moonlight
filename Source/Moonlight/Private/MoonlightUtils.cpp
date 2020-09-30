#include "MoonlightUtils.h"

void AsyncEngine::SetTask(std::function<int()>&& asyncF, std::function<void(int)>&& syncF) {
	if(!ensure(IsAvailable())) return;
	_syncF = std::move(syncF);
	_thread = std::thread([asF = std::move(asyncF), this]() { resultCode = asF(); _done.store(true, std::memory_order_release); });
	_running = true;
}

void AsyncEngine::StopTask() {
	if(!_running) return;
	if(_done.load(std::memory_order_acquire)) {
		checkf(_thread.joinable(), L"Async thread not joinable");
		_thread.join();
		_running = false;
	} else {			// Unsalvagable: detach
		_thread.detach();
	}
}

void AsyncEngine::Tick() {
	if(_done.exchange(false, std::memory_order_acquire)) {
		checkf(_thread.joinable(), L"Async thread not joinable");
		_thread.join();
		_running = false;
		_syncF(resultCode);
	}
}
