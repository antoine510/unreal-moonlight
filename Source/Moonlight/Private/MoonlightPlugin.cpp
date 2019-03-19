#include "MoonlightPCH.h"
#include "MoonlightPlugin.h"
#include <Engine.h>

using namespace Moonlight;

FLogCategoryMoonlightLog MoonlightLog;

void Moonlight::LogMessage(const FString& msg) {
	UE_LOG(MoonlightLog, Log, TEXT("%s"), *msg);
}

void Moonlight::LogMessageOnScreen(const FString& msg) {
	LogMessage(msg);
	if(GEngine != nullptr) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, msg);
}

void Moonlight::LogMessage(const char* msg) {
	UE_LOG(MoonlightLog, Log, TEXT("%s"), UTF8_TO_TCHAR(msg));
}

void Moonlight::LogMessageOnScreen(const char* msg) {
	LogMessage(msg);
	if(GEngine != nullptr) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, UTF8_TO_TCHAR(msg));
}

FString Moonlight::AddressStripPort(const FString& address) {
	int index;
	FString res(address);
	if(address.FindLastChar(L':', index)) {
		res.RemoveAt(index, address.Len() - index);
	}
	return res;
}


void StreamSource::_UpdateImageCB(const uint8_t* imageDataNV12, void* context) {
	StreamSource* src = (StreamSource*)context;
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		UpdateDynamicTextureCode,
		UTexture2D**, pTextures, &src->_imageY,
		const uint8*, pData, imageDataNV12,
		PSTREAM_CONFIGURATION, pConf, &src->_conf,
		{
			FUpdateTextureRegion2D region(0, 0, 0, 0, pConf->width, pConf->height);

			auto resource = (FTexture2DResource*)pTextures[0]->Resource;	// Y plane update
			RHIUpdateTexture2D(resource->GetTexture2DRHI(), 0, region, pConf->width, pData);

			region.Width >>= 1;
			region.Height >>= 1;
			pData += pConf->width * pConf->height;	// We got 1 byte per pixel already
			resource = (FTexture2DResource*)pTextures[1]->Resource;	// UV plane update
			RHIUpdateTexture2D(resource->GetTexture2DRHI(), 0, region, pConf->width, pData);
		});
}
