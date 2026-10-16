#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MythStrEffect.generated.h"

class UProceduralMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;
class UTexture2D;

/** Quadro-chave de uma camada do .str (formato STRM 0x94 do cliente original). */
struct FRoStrKey
{
	int32 Frame = 0;
	uint32 Type = 0;           // 0 = pose absoluta; 1 = variacao por quadro aplicada a pose anterior
	FVector2f Pos = FVector2f::ZeroVector;
	float UV[8] = { 0 };       // retangulo da textura: u0, v0, u1, v1 (o resto se repete)
	float XY[8] = { 0 };       // x0..x3, y0..y3 (cantos relativos a Pos, em pixels de tela)
	float AniFrame = 0.f;      // indice da textura da camada
	uint32 AniType = 0;        // 0 fixa, 1 normal, 2 ate o fim, 3 loop, 4 loop reverso
	float Delay = 0.f;
	float Angle = 0.f;
	FLinearColor Color = FLinearColor::White; // 0..255
	uint32 SrcBlend = 5;
	uint32 DstBlend = 6;       // D3DBLEND: 6 = transparencia; 2/7 = aditivo
};

struct FRoStrLayer
{
	TArray<FString> Textures;
	TArray<FRoStrKey> Keys;
};

/** Arquivo .str (data/texture/effect/<nome>.str): camadas de textura animadas a 'Fps' quadros por segundo. */
struct FRoStr
{
	int32 Fps = 60;
	int32 MaxKey = 0;
	TArray<FRoStrLayer> Layers;

	bool Load(const FString& Path);
};

/**
 * Efeito .str do cliente original tocando no mundo: cada camada e um quad virado para a camera (como os sprites),
 * animado pelos quadros-chave. HD-2D: uma luz do Lumen acompanha o efeito, com a cor das texturas visiveis e a
 * intensidade pelo alfa delas a cada quadro, entao o efeito ilumina o chao e os personagens em volta.
 */
UCLASS()
class MYTHOFTHOR_API AMythStrEffect : public AActor
{
	GENERATED_BODY()

public:
	AMythStrEffect();

	/** Toca data/texture/effect/<StrFile>.str em Location (ou preso ao ator Follow). Nulo se o arquivo nao existir. */
	static AMythStrEffect* Play(UObject* WorldContext, const FString& StrFile, const FVector& Location, AActor* Follow = nullptr);

	/** Altura acima dos pes (uu) quando o efeito acompanha um ator: efeitos marcados "head" ficam acima da cabeca. */
	float HeadOffset = 0.f;

	virtual void Tick(float DeltaSeconds) override;

	/** Pes da unidade (base da capsula), onde os efeitos e sprites se apoiam. */
	static FVector FeetOf(const AActor* Unit);

	/** cm por pixel de tela do .str (calibrado em 2.8 para presença visual epica em HD-2D). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	float UnitsPerPixel = 2.8f;

	/** Puxado para a camera para ficar na frente do sprite (os sprites usam 70). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	float CameraDepthOffset = 95.f;

	/** Brilho das camadas aditivas (luz somada) para aparecer na exposicao da cena com Lumen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	float AdditiveBrightness = 8.0f;

	/** Brilho das camadas com transparencia comum. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	float TranslucentBrightness = 3.2f;

	/** Intensidade maxima da luz do efeito (candelas). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	float LightIntensity = 140.f;

	UPROPERTY(VisibleAnywhere, Category = "Effect")
	TObjectPtr<UProceduralMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Effect")
	TObjectPtr<UPointLightComponent> Light;

private:
	TSharedPtr<FRoStr> Str;
	FString Folder; // pasta do .str dentro de data/texture/effect (as texturas sao relativas a ela)
	TWeakObjectPtr<AActor> Follow;
	FVector Origin = FVector::ZeroVector;
	float Elapsed = 0.f;
	bool bLoggedFirstFrame = false;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> LayerMaterials;
	TArray<bool> SectionCreated;

	bool Setup(const FString& StrFile);
	bool EvalLayer(const FRoStrLayer& Layer, float Frame, FRoStrKey& Out) const;
	UTexture2D* LayerTexture(const FRoStrLayer& Layer, float AniFrame) const;
	FString TexturePath(const FString& Name) const;
};

/**
 * Clarao de luz do Lumen (HD-2D) para skills sem efeito .str no cliente (muitos efeitos classicos ficam no executavel):
 * sobe rapido e apaga suave, na cor do elemento da skill.
 */
UCLASS()
class MYTHOFTHOR_API AMythLightFlash : public AActor
{
	GENERATED_BODY()

public:
	AMythLightFlash();

	static AMythLightFlash* Flash(UObject* WorldContext, const FVector& Location, const FLinearColor& Color, float Intensity = 60.f, float Duration = 0.6f, AActor* Follow = nullptr);

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Effect")
	TObjectPtr<UPointLightComponent> Light;

private:
	TWeakObjectPtr<AActor> Follow;
	float Peak = 60.f;
	float Duration = 0.6f;
	float Elapsed = 0.f;
};
