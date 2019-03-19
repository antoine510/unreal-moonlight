#include "MoonlightPCH.h"
#include "Moonlight.h"
#include "Core.h"
#include "ModuleManager.h"

#include "moonvdec/moonvdec.h"

bool MoonlightModule::initialized = false;
void* MoonlightModule::moonvdecHandle = nullptr;

namespace Moonlight {
	extern void LogMessage(const char* msg);
}

void MoonlightModule::StartupModule() {
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	// Add on the relative location of the third party dll and load it
	FString libraryPath = FPaths::Combine(FPaths::ProjectPluginsDir(), FString("Moonlight/ThirdParty/moonvdec/lib/"));
	FString libraryName;

#if PLATFORM_WINDOWS
#if PLATFORM_64BITS
	libraryPath = FPaths::Combine(libraryPath, L"Win64");
#elif PLATFORM_32BITS
	libraryPath = FPaths::Combine(libraryPath, L"Win32");
#endif
	libraryName = L"moonvdec.dll";
#elif PLATFORM_LINUX
	libraryPath = FPaths::Combine(libraryPath, L"Linux");
	libraryName = L"libmoonvdec.so";
#elif PLATFORM_MAC
	libraryPath = FPaths::Combine(libraryPath, L"MacOS");
	libraryName = L"libmoonvdec.dylib";
#endif
	if(libraryName.Len() == 0) {
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(L"Unsuported platform!"));
		return;
	}

	if(!FPaths::FileExists(FPaths::Combine(libraryPath, libraryName))) {
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(L"Couldn't find Moonlight, libeay or ssleay library at: " + libraryPath));
		return;
	}

	FPlatformProcess::PushDllDirectory(*libraryPath);

	moonvdecHandle = FPlatformProcess::GetDllHandle(*libraryName);

	FPlatformProcess::PopDllDirectory(*libraryPath);

	if(moonvdecHandle == nullptr) {
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(L"Couldn't load Moonlight, libeay or ssleay library"));
		ShutdownModule();
		return;
	}

	if(mvd_Init(Moonlight::LogMessage) != 0) {
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(L"Initialization of moonvdec failed"));
		ShutdownModule();
		return;
	}
	initialized = true;
}

void MoonlightModule::ShutdownModule() {
	initialized = false;
	if(moonvdecHandle != nullptr) mvd_Close();

	FPlatformProcess::FreeDllHandle(moonvdecHandle);
}

IMPLEMENT_MODULE(MoonlightModule, Moonlight);
