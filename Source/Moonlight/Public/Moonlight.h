#pragma once

class MoonlightModule : public IModuleInterface {
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static bool isLoaded() { return initialized; }

private:
	static bool initialized;
	static void* moonvdecHandle;
};
