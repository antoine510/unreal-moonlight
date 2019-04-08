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
	ENQUEUE_RENDER_COMMAND(UpdateDynamicTextureCode)([src, imageDataNV12](FRHICommandListImmediate& RHICmdList) {
		FUpdateTextureRegion2D region(0, 0, 0, 0, src->getWidth(), src->getHeight());

		auto resource = (FTexture2DResource*)src->getImageY()->Resource;	// Y plane update
		RHICmdList.UpdateTexture2D(resource->GetTexture2DRHI(), 0, region, src->getWidth(), imageDataNV12);

		region.Width >>= 1;
		region.Height >>= 1;
		const uint8* imageDataUV = imageDataNV12 + src->getWidth() * src->getHeight();	// We got 1 byte per pixel already
		resource = (FTexture2DResource*)src->getImageUV()->Resource;	// UV plane update
		RHICmdList.UpdateTexture2D(resource->GetTexture2DRHI(), 0, region, src->getWidth(), imageDataUV);
	});
}
