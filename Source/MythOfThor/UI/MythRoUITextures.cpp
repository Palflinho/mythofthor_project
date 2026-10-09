#include "UI/MythRoUITextures.h"

#include "Sprites/MythRoSpriteComponent.h"
#include "Sprites/RoSpriteFormats.h"
#include "Engine/Texture2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

namespace
{
	const TCHAR* UIDir = TEXT("유저인터페이스"); // 유저인터페이스

	TMap<FString, TWeakObjectPtr<UTexture2D>>& UITextureCache()
	{
		static TMap<FString, TWeakObjectPtr<UTexture2D>> Map;
		return Map;
	}

	// Cor media (ponderada pela intensidade) de cada textura carregada: a luz dos efeitos sai dela
	TMap<FString, FLinearColor>& UITextureAverage()
	{
		static TMap<FString, FLinearColor> Map;
		return Map;
	}
}

UTexture2D* FMythRoUITextures::Get(const FString& RelativePath)
{
	const FString Key = RelativePath.ToLower();
	if (const TWeakObjectPtr<UTexture2D>* Found = UITextureCache().Find(Key))
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
		UITextureCache().Add(Key, nullptr);
		return nullptr;
	}

	IImageWrapperModule& Module = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	const EImageFormat Format = Module.DetectImageFormat(Bytes.GetData(), Bytes.Num());
	TSharedPtr<IImageWrapper> Wrapper = Module.CreateImageWrapper(Format == EImageFormat::Invalid ? EImageFormat::BMP : Format);
	TArray<uint8> Raw;
	if (!Wrapper.IsValid() || !Wrapper->SetCompressed(Bytes.GetData(), Bytes.Num()) || !Wrapper->GetRaw(ERGBFormat::BGRA, 8, Raw))
	{
		UE_LOG(LogTemp, Warning, TEXT("MythRoUI: imagem invalida: %s"), *Path);
		UITextureCache().Add(Key, nullptr);
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

	{
		FLinearColor Sum(0.f, 0.f, 0.f, 0.f);
		float Weight = 0.f;
		for (int32 i = 0; i < W * H; i += 3)
		{
			const FLinearColor C(Pixels[i]);
			const float L = (C.R + C.G + C.B) * C.A;
			Sum += C * L;
			Weight += L;
		}
		UITextureAverage().Add(Key, Weight > 0.f ? Sum / Weight : FLinearColor::White);
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
	UITextureCache().Add(Key, Tex);
	return Tex;
}

UTexture2D* FMythRoUITextures::GetSpriteFrame(const FString& SprPath, int32 Index)
{
	const FString Key = FString::Printf(TEXT("spr:%s#%d"), *SprPath.ToLower(), Index);
	if (const TWeakObjectPtr<UTexture2D>* Found = UITextureCache().Find(Key))
	{
		return Found->Get();
	}
	UITextureCache().Add(Key, nullptr); // falhas nao sao re-tentadas a cada quadro

	TArray<uint8> Bytes;
	const FString Path = FPaths::Combine(UMythRoSpriteComponent::GetAssetsRoot(), TEXT("data"), TEXT("sprite"), SprPath + TEXT(".spr"));
	FRoSpr Spr;
	if (!FFileHelper::LoadFileToArray(Bytes, *Path) || !Spr.Load(Bytes))
	{
		UE_LOG(LogTemp, Warning, TEXT("MythRoUI: sprite nao encontrado: %s"), *Path);
		return nullptr;
	}
	const bool bRgba = Index >= Spr.Indexed.Num();
	const FRoSprImage* Img = Spr.Find(bRgba ? Index - Spr.Indexed.Num() : Index, bRgba ? 1 : 0);
	if (!Img || Img->Width <= 0 || Img->Height <= 0)
	{
		return nullptr;
	}
	UTexture2D* Tex = UTexture2D::CreateTransient(Img->Width, Img->Height, PF_B8G8R8A8);
	if (!Tex)
	{
		return nullptr;
	}
	Tex->Filter = TF_Nearest;
	Tex->SRGB = true;
	Tex->NeverStream = true;
	Tex->LODGroup = TEXTUREGROUP_UI;
	Tex->AddToRoot();
	FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
	FMemory::Memcpy(Mip.BulkData.Lock(LOCK_READ_WRITE), Img->Pixels.GetData(), Img->Pixels.Num() * sizeof(FColor));
	Mip.BulkData.Unlock();
	Tex->UpdateResource();
	UITextureCache().Add(Key, Tex);
	return Tex;
}

FLinearColor FMythRoUITextures::AverageColor(const FString& RelativePath)
{
	Get(RelativePath);
	const FLinearColor* C = UITextureAverage().Find(RelativePath.ToLower());
	return C ? *C : FLinearColor::White;
}
