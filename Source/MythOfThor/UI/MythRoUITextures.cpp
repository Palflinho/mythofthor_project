#include "UI/MythRoUITextures.h"

#include "Sprites/MythRoSpriteComponent.h"
#include "Engine/Texture2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

namespace
{
	const TCHAR* UIDir = TEXT("유저인터페이스"); // 유저인터페이스

	TMap<FString, TWeakObjectPtr<UTexture2D>>& Cache()
	{
		static TMap<FString, TWeakObjectPtr<UTexture2D>> Map;
		return Map;
	}
}

UTexture2D* FMythRoUITextures::Get(const FString& RelativePath)
{
	const FString Key = RelativePath.ToLower();
	if (const TWeakObjectPtr<UTexture2D>* Found = Cache().Find(Key))
	{
		if (Found->IsValid())
		{
			return Found->Get();
		}
	}

	const FString Path = FPaths::Combine(UMythRoSpriteComponent::GetAssetsRoot(), TEXT("data"), TEXT("texture"), UIDir, RelativePath);
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *Path))
	{
		UE_LOG(LogTemp, Warning, TEXT("MythRoUI: textura nao encontrada: %s"), *Path);
		Cache().Add(Key, nullptr);
		return nullptr;
	}

	IImageWrapperModule& Module = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	const EImageFormat Format = Module.DetectImageFormat(Bytes.GetData(), Bytes.Num());
	TSharedPtr<IImageWrapper> Wrapper = Module.CreateImageWrapper(Format == EImageFormat::Invalid ? EImageFormat::BMP : Format);
	TArray<uint8> Raw;
	if (!Wrapper.IsValid() || !Wrapper->SetCompressed(Bytes.GetData(), Bytes.Num()) || !Wrapper->GetRaw(ERGBFormat::BGRA, 8, Raw))
	{
		UE_LOG(LogTemp, Warning, TEXT("MythRoUI: imagem invalida: %s"), *Path);
		Cache().Add(Key, nullptr);
		return nullptr;
	}

	const int32 W = Wrapper->GetWidth();
	const int32 H = Wrapper->GetHeight();
	FColor* Pixels = reinterpret_cast<FColor*>(Raw.GetData());
	for (int32 i = 0; i < W * H; ++i)
	{
		// Chave de transparencia do cliente original: magenta puro (com tolerancia para BMPs reamostrados).
		if (Pixels[i].R > 240 && Pixels[i].G < 15 && Pixels[i].B > 240)
		{
			Pixels[i] = FColor(0, 0, 0, 0);
		}
	}

	UTexture2D* Tex = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
	if (!Tex)
	{
		return nullptr;
	}
	Tex->Filter = TF_Nearest;
	Tex->SRGB = true;
	Tex->NeverStream = true;
	Tex->LODGroup = TEXTUREGROUP_UI;
	Tex->AddToRoot(); // texturas de interface vivem o jogo todo (cache estatico)
	FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
	FMemory::Memcpy(Mip.BulkData.Lock(LOCK_READ_WRITE), Pixels, W * H * sizeof(FColor));
	Mip.BulkData.Unlock();
	Tex->UpdateResource();
	Cache().Add(Key, Tex);
	return Tex;
}
