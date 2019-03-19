#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Tickable.h"
#include <atomic>
#include <thread>

#include "MoonlightPlugin.h"
#include "MoonlightUtils.h"

#include "MoonlightPluginBP.generated.h"

using namespace Moonlight;

UENUM(BlueprintType)
enum class EMoonlightErrorState : uint8 {
	Ok,
	Fail
};


UENUM(BlueprintType)
enum class EMoonlightAudioConfig : uint8 {
	Stereo = 0		UMETA(DisplayName = "Stereo"),
	Surround51 = 1	UMETA(DisplayName = "Surround 5.1")
};

UCLASS(BlueprintType)
class MOONLIGHT_API UMoonlightStreamConfig : public UObject {
	GENERATED_UCLASS_BODY()
public:
	PSTREAM_CONFIGURATION getNative() const {
		_native.width = width;
		_native.height = height;
		_native.fps = fps;
		_native.bitrate = bitrate;
		_native.packetSize = packetSize;
		_native.streamingRemotely = (int)streamingRemotely;
		_native.audioConfiguration = (int)audioConfiguration;
		_native.supportsHevc = false;
		_native.enableHdr = false;
		_native.hevcBitratePercentageMultiplier = 0.75f;
		_native.clientRefreshRateX100 = clientRefreshRateX100;
		return &_native;
	}

	UFUNCTION(BlueprintPure, Meta = (DisplayName = "Make config"))
		static void MakeMoonlightStreamConfig(int width, int height, int fps, int bitrate, int packetSize,
											  bool streamingRemotely, EMoonlightAudioConfig audioConfiguration,
											  int clientRefreshRateX100,
											  UMoonlightStreamConfig*& config) {
		config = NewObject<UMoonlightStreamConfig>();
		config->width = width;
		config->height = height;
		config->fps = fps;
		config->bitrate = bitrate;
		config->packetSize = packetSize;
		config->streamingRemotely = streamingRemotely;
		config->audioConfiguration = audioConfiguration;
		config->clientRefreshRateX100 = clientRefreshRateX100;
	}

	UPROPERTY(BlueprintReadWrite) int width;
	UPROPERTY(BlueprintReadWrite) int height;
	UPROPERTY(BlueprintReadWrite) int fps;
	UPROPERTY(BlueprintReadWrite) int bitrate;
	UPROPERTY(BlueprintReadWrite) int packetSize = 1024;
	UPROPERTY(BlueprintReadWrite) bool streamingRemotely;
	UPROPERTY(BlueprintReadWrite) EMoonlightAudioConfig audioConfiguration;
	UPROPERTY(BlueprintReadWrite) int clientRefreshRateX100;

private:
	mutable STREAM_CONFIGURATION _native = {0};
};

UCLASS(BlueprintType)
class MOONLIGHT_API UMoonlightApp : public UObject {
	GENERATED_UCLASS_BODY()
public:

	UPROPERTY(BlueprintReadOnly) FString name;
	UPROPERTY(BlueprintReadOnly) int id;

	void moveNative(Moonlight::App&& app) {
		name = std::move(app.name);
		id = app.id;
	}

	operator Moonlight::App() const {
		return Moonlight::App{name, id};
	}

	bool IsValid() const { return name.Len() > 0; }

	/**
	* Test validity of SBG Pairing
	*
	* @param	Test			The object to test
	* @return	Return true if the object is usable
	*/
	UFUNCTION(BlueprintPure, Meta = (CompactNodeTitle = "IsValid"))
		static bool IsValid(const UMoonlightApp* Test) { return IsValidT(Test); }
};


UCLASS(BlueprintType)
class MOONLIGHT_API UMoonlightSource : public UObject, public FTickableGameObject {
	GENERATED_UCLASS_BODY()
public:
	DECLARE_DYNAMIC_DELEGATE_OneParam(FLoadStreamSourceDelegate, bool, success);
	DECLARE_DYNAMIC_DELEGATE_OneParam(FPairDelegate, bool, success);
	DECLARE_DYNAMIC_DELEGATE(FLaunchAppDelegate);
	DECLARE_DYNAMIC_DELEGATE_OneParam(FStartStreamDelegate, bool, success);


	void BeginDestroy() override {
		asyncEngine.StopTask();
		delete _native;
		_native = nullptr;
		Super::BeginDestroy();
	}

	TStatId GetStatId() const override { return GetStatID(); }
	bool IsTickable() const override {
		return asyncEngine.IsTickable();
	}

	void Tick(float) override {
		asyncEngine.Tick();
	}


	UFUNCTION(BlueprintCallable, Category = "Moonlight")
		static void LoadStreamSource(const FString& address,
									 UMoonlightStreamConfig* sconf,
									 const FLoadStreamSourceDelegate& sourceLoaded,
									 UMoonlightSource*& source) {
		if(!ensure(MoonlightModule::isLoaded())) return;
		source = NewObject<UMoonlightSource>();
		source->SourceLoaded = sourceLoaded;
		source->asyncEngine.SetTask([=]() { source->_native = new Moonlight::StreamSource(AddressStripPort(address), sconf->getNative()); return source->_native->IsValid(); },
									[source](int out) { source->imageY = source->_native->getImageY(); source->imageUV = source->_native->getImageUV(); source->SourceLoaded.ExecuteIfBound((bool)out); });
	}

	UFUNCTION(BlueprintCallable)
		void Pair(const FString& PIN, const FPairDelegate& paired) {
		if(!ensure(_IsAvailable())) return;
		Paired = paired;
		asyncEngine.SetTask([this, PIN]() { return _native->Pair(PIN); },
							[this](int out) { Paired.ExecuteIfBound(out == 0); });
	}

	UFUNCTION(BlueprintPure, Meta = (ExpandEnumAsExecs = "Branches"))
		void GetAppList(TArray<UMoonlightApp*>& apps, EMoonlightErrorState& Branches) {
		if(!ensure(_IsAvailable())) return;
		TArray<Moonlight::App> cppApps;
		int res = _native->GetAppList(cppApps);
		for(auto app : cppApps) {
			auto bpapp = NewObject<UMoonlightApp>();
			bpapp->moveNative(std::move(app));
			apps.Add(bpapp);
		}
		Branches = (res < 0) ? EMoonlightErrorState::Fail : EMoonlightErrorState::Ok;
	}

	UFUNCTION(BlueprintCallable)
		void LaunchApp(UMoonlightApp* app, const FLaunchAppDelegate& appLaunched) {
		if(!ensure(_IsAvailable())) return;
		AppLaunched = appLaunched;
		asyncEngine.SetTask([this, app]() { _native->LaunchApp(*app); return 0; },
							[this](int out) { AppLaunched.ExecuteIfBound(); });
	}

	UFUNCTION(BlueprintCallable)
		void StartStream(const FStartStreamDelegate& streamStarted) {
		if(!ensure(_IsAvailable())) return;
		StreamStarted = streamStarted;
		asyncEngine.SetTask([this]() { return _native->StartStream(); },
							[this](int out) { StreamStarted.ExecuteIfBound(out == 0); });
	}

	UFUNCTION(BlueprintCallable)
		void StopStream() {
		if(!ensure(IsValid())) return;
		_native->StopStream();
	}

	bool IsValid() const { return _native != nullptr && MoonlightModule::isLoaded(); }

	/**
	* Test validity of SBG Pairing
	*
	* @param	Test			The object to test
	* @return	Return true if the object is usable
	*/
	UFUNCTION(BlueprintPure, Meta = (CompactNodeTitle = "IsValid"))
		static bool IsValid(const UMoonlightSource* Test) { return IsValidT(Test); }


	FLoadStreamSourceDelegate SourceLoaded;
	FPairDelegate Paired;
	FLaunchAppDelegate AppLaunched;
	FStartStreamDelegate StreamStarted;


	UPROPERTY(BlueprintReadOnly) UTexture2D* imageY;
	UPROPERTY(BlueprintReadOnly) UTexture2D* imageUV;

	UFUNCTION(BlueprintPure)
		void GetImage(UTexture2D*& Y, UTexture2D*& UV) {
		check(IsValid());
		Y = imageY;
		UV = imageUV;
	}

private:
	bool _IsAvailable() const {
		if(!IsValid()) LogMessageOnScreen(L"Using invalid StreamSource");
		else if(!asyncEngine.IsAvailable()) LogMessageOnScreen(L"Async operation in progress on this StreamSource");
		else return true;
		return false;
	}

	AsyncEngine asyncEngine;

	Moonlight::StreamSource* _native = nullptr;
};


