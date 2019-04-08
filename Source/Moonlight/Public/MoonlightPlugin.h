#pragma once

#include "moonvdec/moonvdec.h"

#include "Moonlight.h"
#include "Kismet/BlueprintFunctionLibrary.h"

extern MOONLIGHT_API struct FLogCategoryMoonlightLog : public FLogCategory<ELogVerbosity::Log, ELogVerbosity::All> {
	FLogCategoryMoonlightLog() : FLogCategory(TEXT("Moonlight Log")) {}
} MoonlightLog;

namespace Moonlight {

extern bool ModuleRunning;

void MOONLIGHT_API LogMessage(const FString& msg);
void MOONLIGHT_API LogMessageOnScreen(const FString& msg);
void MOONLIGHT_API LogMessage(const char* msg);
void MOONLIGHT_API LogMessageOnScreen(const char* msg);
FString MOONLIGHT_API AddressStripPort(const FString& address);
void MOONLIGHT_API AddressStripPort(FString& address);

struct MOONLIGHT_API App {
	FString name;
	int id;
};

class MOONLIGHT_API StreamSource {
public:
	StreamSource(const FString& address, PSTREAM_CONFIGURATION sconf) :
		_src(mvd_GetStreamSource(TCHAR_TO_ANSI(*address))),
		_conf(*sconf),
		_imageY(_CreateImage(sconf->width, sconf->height, PF_G8)),
		_imageUV(_CreateImage(sconf->width >> 1, sconf->height >> 1, PF_R8G8, false)) {
		if(!ensure(IsValid())) {
			LogMessageOnScreen(L"Could not get stream source @ " + address);
		}
	}


	StreamSource(StreamSource&& old) : _src(old._src), _conf(old._conf), _imageY(old._imageY), _imageUV(old._imageUV)
	{
		old._src = nullptr;
		old._imageY = nullptr;
		old._imageUV = nullptr;
	}

	StreamSource& operator=(StreamSource&& old)
	{
		_src = old._src;
		old._src = nullptr;
		_imageY = old._imageY;
		old._imageY = nullptr;
		_imageUV = old._imageUV;
		old._imageUV = nullptr;
		_conf = old._conf;
		return *this;
	}

	StreamSource(const StreamSource& other) = delete;
	StreamSource& operator=(const StreamSource& old) = delete;

	~StreamSource() {
		if(_src != nullptr) mvd_DiscardStreamSource(_src);
	}

	int getWidth() const { return _conf.width; }
	int getHeight() const { return _conf.height; }
	UTexture2D* getImageY() const { return _imageY; }
	UTexture2D* getImageUV() const { return _imageUV; }

	int Pair(const FString& PIN) {
		if(!ensure(IsValid())) return -1;
		return mvd_PairStreamSource(_src, TCHAR_TO_ANSI(*PIN));
	}
	int GetAppList(TArray<App>& apps) {
		if(!ensure(IsValid())) return -1;
		apps.Empty();
		const int* ids, *lengths;
		const wchar_t* const* names;
		int appCount = mvd_GetAppList(_src, &ids, &names, &lengths);
		for(int i = 0; i < appCount; ++i) {
			apps.Add(App{FString(lengths[i], names[i]), ids[i]});
		}
		return appCount;
	}

	void LaunchApp(const App& app) {
		if(!ensure(IsValid())) return;
		mvd_LaunchApp(_src, app.id, &_conf);
	}

	int StartStream() {
		if(!ensure(IsValid())) return -1;
		return mvd_StartStream(_src, &_conf, _UpdateImageCB, this);
	}

	void StopStream() {
		if(!ensure(IsValid())) return;
		mvd_StopStream(_src);
	}


	bool IsValid() { return _src != nullptr; }

private:
	static void _UpdateImageCB(const uint8_t* imageDataNV12, void* context);

	static UTexture2D* _CreateImage(int width, int height, EPixelFormat format, bool sRGB = true) {
		UTexture2D* image = UTexture2D::CreateTransient(width, height, format);
		image->UpdateResource();

		image->AddressX = TextureAddress::TA_Wrap;
		image->AddressY = TextureAddress::TA_Wrap;
		image->Filter = TextureFilter::TF_Default;
		image->SRGB = (uint8)sRGB;
		image->RefreshSamplerStates();

		return image;
	}

	mvd_StreamSource _src;
	STREAM_CONFIGURATION _conf;
	UTexture2D* _imageY, *_imageUV;
};


}
