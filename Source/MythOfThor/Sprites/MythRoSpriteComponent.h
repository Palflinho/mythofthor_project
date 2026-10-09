#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "Core/RagnarokDirectionHelper.h"
#include "Sprites/MythSpriteAnimComponent.h"
#include "Sprites/RoSpriteFormats.h"
#include "MythRoSpriteComponent.generated.h"

class UTexture2D;
class UMaterialInstanceDynamic;

/** Camadas compostas do personagem, na ordem de desenho do cliente original (de tras para frente). */
UENUM(BlueprintType)
enum class EMythRoLayer : uint8
{
	Body,
	Head
};

/** Um .spr/.act carregado com o atlas de texturas pronto para desenhar. */
USTRUCT()
struct FMythRoSpriteSource
{
	GENERATED_BODY()

	FRoSpr Spr;
	FRoAct Act;

	UPROPERTY() TObjectPtr<UTexture2D> Atlas;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material;

	/** Retangulo de cada imagem no atlas (UV 0..1); indexadas primeiro, depois RGBA. */
	TArray<FBox2f> IndexedUV;
	TArray<FBox2f> RgbaUV;

	bool IsValid() const { return Atlas != nullptr && Act.Actions.Num() > 0; }
};

/**
 * Sprite de personagem no estilo do cliente original: le .spr/.act do disco em runtime e compoe as camadas
 * (corpo + cabeca/cabelo com paleta) a cada quadro, encaixando a cabeca pelas ancoras do .act.
 * Fica sempre paralelo a tela (yaw e pitch da camera) com os pes no ponto de apoio.
 *
 * Arquivos lidos de [Myth] AssetsRoot (DefaultGame.ini) ou da variavel de ambiente MYTH_ASSETS,
 * com a estrutura do GRF: <raiz>/data/sprite/인간족/...
 *
 * A API de animacao (PlayAttack, PlayHit, Die, Revive, FaceLocation) e a mesma do UMythSpriteAnimComponent.
 */
UCLASS(ClassGroup = (Myth), meta = (BlueprintSpawnableComponent))
class MYTHOFTHOR_API UMythRoSpriteComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()

public:
	UMythRoSpriteComponent(const FObjectInitializer& ObjectInitializer);

	/**
	 * Se preenchido, o sprite e de monstro: data/sprite/몬스터/<nome>.spr/.act, sem cabeca, com os grupos de
	 * acao de monstro (0 parado, 1 andando, 2 ataque, 3 dano, 4 morte). Vazio = personagem (corpo + cabeca).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	FString MonsterSpriteName;

	/** Classe no nome do arquivo do corpo (ex.: 초보자 = Aprendiz). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	FString JobSpriteName = TEXT("초보자");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	bool bMale = true;

	/** Penteado (numero do arquivo em 머리통). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	int32 HairStyle = 1;

	/** Cor de cabelo (paleta 머리<estilo>_<sexo>_<cor>.pal); 0 = paleta original do arquivo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	int32 HairColor = 0;

	/** Centimetros por pixel do sprite (mesma escala dos flipbooks: 1 px = 2 cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	float UnitsPerPixel = 2.f;

	/** Quanto (cm) o sprite e puxado em direcao a camera para nao ser cortado por paredes/chao. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	float CameraDepthOffset = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	float WalkSpeedThreshold = 10.f;

	/** Margem (graus) alem do limite de 22,5 graus antes de trocar de direcao: evita o sprite "tremer" entre duas direcoes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	float DirectionHysteresisDeg = 10.f;

	/** Tempo parado (s) antes de voltar para a animacao Idle: curvas do caminho nao reiniciam a caminhada. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	float IdleDelay = 0.12f;

	/** Suavizacao da altura dos pes (degraus da malha do chao). 0 = sem suavizacao. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	float FeetHeightSmoothing = 18.f;

	/** Sprite iluminado pela cena (Lumen, noite, luzes de poste). Falso = sem iluminacao, como o cliente original. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	bool bLitByScene = true;

	/**
	 * Sombra com o formato do sprite: uma copia invisivel virada para o sol/lua que so projeta sombra,
	 * entao a sombra nao muda ao girar a camera (tecnica HD-2D).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	bool bCastSpriteShadow = true;

	/** Limites do quadro atual no espaco local do componente (para a area de clique). */
	const FBox& GetFrameLocalBounds() const { return FrameLocalBounds; }

	/** Recarrega corpo e cabeca (chame apos mudar sexo/penteado/cor). */
	UFUNCTION(BlueprintCallable, Category = "RO Sprite")
	void ReloadSprites();

	UFUNCTION(BlueprintCallable, Category = "RO Sprite")
	void PlayAttack(float AttackDuration = 0.f);

	UFUNCTION(BlueprintCallable, Category = "RO Sprite")
	void PlayHit();

	UFUNCTION(BlueprintCallable, Category = "RO Sprite")
	void Die();

	UFUNCTION(BlueprintCallable, Category = "RO Sprite")
	void Revive();

	UFUNCTION(BlueprintCallable, Category = "RO Sprite")
	void FaceLocation(const FVector& WorldLocation);

	UFUNCTION(BlueprintPure, Category = "RO Sprite")
	EMythSpriteState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "RO Sprite")
	ERagnarokDirection GetDirection() const { return Direction; }

	/** Raiz dos assets extraidos do GRF (pasta que contem 'data'). */
	static FString GetAssetsRoot();

protected:
	virtual void BeginPlay() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY() FMythRoSpriteSource Body;
	UPROPERTY() FMythRoSpriteSource Head;

	/** Copia do sprite que so projeta sombra (nao aparece na imagem). */
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> ShadowMesh;
	TWeakObjectPtr<class AMythDayNightActor> DayNight;
	void UpdateShadowTransform(const FVector& Feet);

	EMythSpriteState State = EMythSpriteState::Idle;
	ERagnarokDirection Direction = ERagnarokDirection::South;
	float StateTime = 0.f;
	float PlayRate = 1.f;
	bool bOneShotPlaying = false;
	FVector FeetRelativeLocation = FVector::ZeroVector;
	/** Quantas secoes de malha ja foram criadas (uma por camada desenhada). */
	int32 SectionsCreated = 0;
	FBox FrameLocalBounds = FBox(ForceInit);
	float StillTime = 0.f;
	float SmoothedFeetZ = 0.f;
	bool bFeetZInitialized = false;

	bool IsMonster() const { return !MonsterSpriteName.IsEmpty(); }
	/** Direcao pela velocidade, com histerese em relacao a direcao atual. */
	ERagnarokDirection ComputeDirection(const FVector& Velocity, const FRotator& CamRot) const;

	bool LoadSource(FMythRoSpriteSource& Source, const FString& SprPath, const FString& ActPath, const FString& PalettePath);
	void BuildAtlas(FMythRoSpriteSource& Source);
	void SetState(EMythSpriteState NewState, bool bOneShot);

	/** Indice da acao no .act do jogador: grupo * 8 + direcao. */
	int32 GetActionIndex() const;
	/** Quadro atual da acao (one-shots param no ultimo quadro; loops repetem). */
	int32 GetFrameIndex(const FRoActAction& Action, bool& bOutFinished) const;

	void UpdateBillboard(const FRotator& CamRot);
	void RebuildMesh(const FRotator& CamRot);
	void AddLayerQuads(const FMythRoSpriteSource& Source, const FRoActFrame& Frame, const FIntPoint& Offset, float DepthBase, int32& Section);
	FRotator GetCameraRotation() const;
};
