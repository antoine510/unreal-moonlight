#include <random>
#include <Engine.h>

extern void Moonlight::LogMessage(const char* msg);

template <typename T>
bool IsValidT(const T* Test) {
	return ::IsValid(Test) && Test->IsValid();
}

namespace Moonlight {

class AsyncEngine {
public:
	AsyncEngine() {}

	void SetTask(std::function<int()>&& asyncF, std::function<void(int)>&& syncF) {
		if(!ensure(IsAvailable())) return;
		_syncF = std::move(syncF);
		_thread = std::thread([asF = std::move(asyncF), this]() { resultCode = asF(); _done.store(true, std::memory_order_release); });
		_running = true;
	}

	void StopTask() {
		if(!_running) return;
		if(_done.load(std::memory_order_acquire)) {
			checkf(_thread.joinable(), L"Async thread not joinable");
			_thread.join();
			_running = false;
		} else {			// Unsalvagable: detach
			_thread.detach();
		}
	}

	void Tick() {
		if(_done.exchange(false, std::memory_order_acquire)) {
			checkf(_thread.joinable(), L"Async thread not joinable");
			_thread.join();
			_running = false;
			_syncF(resultCode);
		}
	}



	bool IsTickable() const { return _running; }
	bool IsAvailable() const { return !_running; }

private:
	std::thread _thread;
	std::atomic<bool> _done;
	bool _running = false;

	int resultCode;

	std::function<void(int)> _syncF;
};

template <int N>
void fillRandomBytes(char* in) {
	std::random_device rd;
	int genCount = ((N - 1) >> 2) + 1;
	for(int i = 0; i < genCount; ++i) {
		uint32_t rnd = rd();
		for(int j = 0; j < 4; ++j) {
			if(i * 4 + j > N) break;
			in[i * 4 + j] = (char)((rnd >> 8 * j) & 0xff);
		}
	}
}

inline char* allocCString(const FString& str) {
	const auto cast = StringCast<char>(*str);
	char* res = (char*)FMemory::SystemMalloc(cast.Length() + 1);
	FMemory::Memcpy(res, cast.Get(), cast.Length());
	res[cast.Length()] = 0;
	return res;
}

inline void freeCString(const char* cstr) {
	FMemory::SystemFree(const_cast<char*>(cstr));
}

template <typename T, typename IGCT>
inline TArray<T> moveToTArray(IGCT* items, uint32_t size) {
	auto v = TArray<T>();
	v.Reserve(size);
	for(int i = 0; i != size; ++i) {
		v.Emplace(items[i]);
	}
	SatLib_Free(items);
	return v;
}

template <typename T1, typename... Ts>
inline bool _valid(T1* obj, Ts*... pack) {
	return IsValid(obj) && _valid(pack...);
}

template <typename Last>
inline bool _valid(Last* obj) {
	return IsValid(obj);
}

template <typename... ObjectsT>
inline bool checkValid(ObjectsT*... objects) {
	if(_valid(objects...)) {
		return true;
	} else {
		MoonlightLog("Trying to use uninitialized object");
		return false;
	}
}

}


