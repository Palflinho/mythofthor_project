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

	/** Pasta do sprite "de unidade" dentro de data/sprite: vazio = 몬스터 (monstros); "npc" para NPCs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	FString MonsterSpriteFolder;

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

	/** Classe (id do rAthena): escolhe a pasta das armas/escudos (transclasse usa a da classe base). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite|Equip")
	int32 JobClass = 0;

	/** View ids dos acessorios (LOOK_HEAD_TOP/MID/BOTTOM); 0 = nada. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite|Equip")
	int32 HeadTop = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite|Equip")
	int32 HeadMid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite|Equip")
	int32 HeadBottom = 0;

	/** View id da arma (LOOK_WEAPON) e do escudo (LOOK_SHIELD); 0 = nada. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite|Equip")
	int32 Weapon = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite|Equip")
	int32 Shield = 0;

	/** Capa/asas/mochila (LOOK_ROBE do rAthena): id visual -> data/sprite/로브/<nome> (Config/robe_sprites.json). 0 = sem capa. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite|Equip")
	int32 Robe = 0;

	/** Carrinho do Mercador (OPTION_CART1..5 do rAthena -> 1..5; 0 = sem carrinho): data/sprite/이팩트/손수레N. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite|Equip")
	int32 CartType = 0;

	/** Converte o campo option dos pacotes (OPTION_*) no tipo de carrinho. */
	static int32 CartTypeFromOption(int32 Option);

	/** Estados visuais do EntityState do cliente: opt1 (pedra, gelo, atordoar, sono), opt2 (veneno, maldicao...) e as skills
	 * de opt3 (Aceleracao, Escudo, Berserk...) tingem o sprite; esconder/camuflar some com ele (ou o escurece em quem e voce). */
	void SetBodyState(int32 Value);
	void SetHealthState(int32 Value);
	void SetEfstVirtue(int32 EfstId, bool bOn);
	void SetEffectOption(int32 Option, bool bSelf);

	/** Filtro bilinear como o cliente original (falso = pixel duro, "nearest"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	bool bSmoothSprites = true;

	/** Centimetros por pixel do sprite (mesma escala dos flipbooks: 1 px = 2 cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	float UnitsPerPixel = 2.f;

	/** Quanto (cm) o sprite e puxado em direcao a camera, pela linha de visao, para nao ser cortado por paredes/chao. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	float CameraDepthOffset = 70.f;

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

	/**
	 * Verdadeiro = material iluminado pela engine (recebe tambem a sombra da propria copia de sombra, a "autossombra").
	 * Falso (padrao) = como o cliente original: sprite sem iluminacao, tingido por uma luz calculada (TintLighting).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	bool bLitByScene = false;

	/**
	 * Luz calculada do sprite (sem autossombra): dia/noite, calor dos postes proximos e escurecimento quando
	 * um predio fica entre o sprite e o sol.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite|Light")
	bool bTintLighting = true;

	/** Cor do sprite em plena noite (sem postes por perto). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite|Light")
	FLinearColor NightTint = FLinearColor(0.42f, 0.47f, 0.68f);

	/** Multiplicador quando um predio tapa o sol (sombra "neles"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite|Light")
	float ShadowedTint = 0.6f;

	/** Quanto a luz quente dos postes clareia o sprite a noite. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite|Light")
	float LampTintStrength = 0.75f;

	/**
	 * Sombra com o formato do sprite: uma copia invisivel virada para o sol/lua que so projeta sombra,
	 * entao a sombra nao muda ao girar a camera (tecnica HD-2D).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	bool bCastSpriteShadow = true;

	/** Sombra redonda classica do RO (data/sprite/shadow.spr) sob os pes, somada a sombra dinamica. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	bool bBlobShadow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	float BlobShadowScale = 1.2f;

	/**
	 * Com o sol alto a sombra projetada vira uma tira fina ("papel"). Quando o comprimento dela no chao fica abaixo
	 * deste valor (cm, ~ tamanho do blob), ela encolhe ate sumir e fica so o blob; com o sol baixando ela volta a
	 * crescer suavemente (some por completo em ShadowFadeLength e esta inteira em 2x).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	float ShadowFadeLength = 45.f;

	/** Sprites mais baixos que isto (cm) nao projetam a sombra do Lumen, so a redonda (o Poring virava uma mancha). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	float MinCastShadowHeight = 70.f;

	/** Quanto a base da sombra projetada afunda no chao (cm): ela comeca sob a elipse, sem corte seco nos pes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	float CastShadowSink = 12.f;

	/** Toca os sons pedidos pelos eventos dos quadros do .act (passos, golpes, dano, morte). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RO Sprite")
	bool bPlayActSounds = true;

	/** Limites do quadro atual no espaco local do componente (para a area de clique). */
	const FBox& GetFrameLocalBounds() const { return FrameLocalBounds; }

	/** Recarrega corpo, cabeca e equipamentos (chame apos mudar classe/sexo/penteado/cor/equipamento). */
	UFUNCTION(BlueprintCallable, Category = "RO Sprite")
	void ReloadSprites();

	UFUNCTION(BlueprintCallable, Category = "RO Sprite")
	void PlayAttack(float AttackDuration = 0.f);

	UFUNCTION(BlueprintCallable, Category = "RO Sprite")
	void PlayHit();
	/** Gesto de pegar item do chao (acao 3 do .act do jogador). */
	void PlayPickup();

	/** Senta/levanta (ZC_NOTIFY_ACT tipo 2/3 do servidor): acao 16..23 do .act, parada. */
	void SetSitting(bool bSit);
	bool IsSitting() const { return State == EMythSpriteState::Sit; }

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

	/**
	 * Para onde o corpo olha no MUNDO, no formato do rAthena (0 norte, 1 noroeste ... 4 sul ... 7 nordeste). O quadro do
	 * sprite sai desse valor e da camera (docs/ref/01 3.7): girar a camera mostra outro lado do personagem parado.
	 */
	void SetFacingRoDir(uint8 RoDir);
	uint8 GetFacingRoDir() const;

	/** Cabeca parada ("doridori", docs/ref/03 4.3): 0 frente, 1 virada a direita (CW), 2 a esquerda (CCW). */
	void SetHeadDir(uint8 NewHeadDir);
	uint8 GetHeadDir() const { return HeadTurn; }

	/** Raiz dos assets extraidos do GRF (pasta que contem 'data'). */
	static FString GetAssetsRoot();

protected:
	virtual void BeginPlay() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY() FMythRoSpriteSource Body;
	UPROPERTY() FMythRoSpriteSource Head;
	/** Equipamentos visiveis: acessorios (embaixo, meio, topo), arma e escudo. */
	UPROPERTY() FMythRoSpriteSource HatBottom;
	UPROPERTY() FMythRoSpriteSource HatMid;
	UPROPERTY() FMythRoSpriteSource HatTop;
	UPROPERTY() FMythRoSpriteSource WeaponSource;
	UPROPERTY() FMythRoSpriteSource ShieldSource;
	UPROPERTY() FMythRoSpriteSource RobeSource;
	UPROPERTY() FMythRoSpriteSource CartSource;
	void LoadEquipment(const FString& Root, const FString& Sex);
	/** Camada que acompanha a cabeca (cabelo e acessorios): encaixa pela ancora do corpo. */
	void AddHeadLayer(const FMythRoSpriteSource& Source, const FRoActFrame& BodyFrame, int32 ActionIndex, int32 FrameIndex, float DepthBase, int32& Section);
	/** Camada que acompanha o corpo (arma, escudo): mesmo quadro e origem do corpo. */
	void AddBodyLayer(const FMythRoSpriteSource& Source, int32 ActionIndex, int32 FrameIndex, float DepthBase, int32& Section);

	/** Copia do sprite que so projeta sombra (nao aparece na imagem). */
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> ShadowMesh;
	/** Sombra redonda (shadow.spr) deitada no chao. */
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> BlobMesh;
	UPROPERTY() FMythRoSpriteSource BlobSource;
	void CreateBlobShadow();
	/** Ultimo (acao, quadro) cujo evento de som ja tocou. */
	FIntPoint LastSoundFrame = FIntPoint(-1, -1);
	void PlayFrameEvent(const FRoActFrame& Frame, int32 ActionIndex, int32 FrameIndex);
	TWeakObjectPtr<class AMythDayNightActor> DayNight;
	void UpdateShadowTransform(const FVector& Feet);

	/** Luz calculada atual (suavizada) aplicada na cor dos vertices quando bTintLighting. */
	FLinearColor CurrentTint = FLinearColor::White;

	/** Cor dos estados (corpo x saude x opt3 x camuflagem), multiplicada na cor dos vertices. */
	FLinearColor StatusTint = FLinearColor::White;
	int32 BodyState = 0;
	int32 HealthState = 0;
public:
	int32 GetHealthState() const { return HealthState; }
private:
	int32 Virtue = 0;
	bool bStatusDim = false;
	bool bStatusHidden = false;
	void RecomputeStatusTint();
	/** Segundos que o jogador ainda fica em postura de combate (acao 4 do .act) depois de bater ou apanhar. */
	float ReadyTimeLeft = 0.f;
	TMap<FName, TWeakObjectPtr<AActor>> StatusAttachments;
	void SetStatusAttachment(FName Name, const TCHAR* SprFile, bool bOn, bool bAboveHead = true, float SideOffsetUU = 0.f, float UpOffsetUU = 0.f);
	float SunVisibility = 1.f;
	float SunTraceTimer = 0.f;
	TArray<TWeakObjectPtr<class AMythNightLight>> NearbyLamps;
	void UpdateTint(const FVector& Feet, float DeltaTime);

	EMythSpriteState State = EMythSpriteState::Idle;
	ERagnarokDirection Direction = ERagnarokDirection::South;
	/** Frente do corpo no plano do mundo (unitario). Padrao: sul do RO (+Y), de frente para a camera inicial. */
	FVector2D FacingWorld = FVector2D(0.0, 1.0);
	uint8 HeadTurn = 0;
	float StateTime = 0.f;
	float PlayRate = 1.f;
	/** Distancia andada (unidades do RO = 10 uu) desde que entrou em Walk: o passo segue o chao (docs/ref/03 4.1). */
	float WalkDistanceRo = 0.f;
	bool bOneShotPlaying = false;
	FVector FeetRelativeLocation = FVector::ZeroVector;
	/** Quantas secoes de malha ja foram criadas (uma por camada desenhada). */
	int32 SectionsCreated = 0;
	FBox FrameLocalBounds = FBox(ForceInit);
	float StillTime = 0.f;
	float SmoothedFeetZ = 0.f;
	bool bFeetZInitialized = false;

	bool IsMonster() const { return !MonsterSpriteName.IsEmpty(); }
	/** Direcao do sprite pela frente do corpo no mundo e a camera, com histerese em relacao a direcao atual. */
	ERagnarokDirection ComputeDirection(const FRotator& CamRot) const;

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
