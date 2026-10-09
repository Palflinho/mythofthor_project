#include "Sprites/MythRoSpriteComponent.h"

#include "World/MythDayNightActor.h"
#include "World/MythNightLight.h"
#include "Sprites/MythRoSound.h"
#include "Components/PointLightComponent.h"
#include "EngineUtils.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	// Caminhos do GRF em coreano, escritos como codigos Unicode para nao depender da codificacao do fonte.
	const TCHAR* HumanDir = TEXT("인간족");   // 인간족
	const TCHAR* BodyDir = TEXT("몸통");          // 몸통
	const TCHAR* HeadDir = TEXT("머리통");    // 머리통
	const TCHAR* HairPalDir = TEXT("머리");       // 머리
	const TCHAR* MaleTag = TEXT("남");                // 남
	const TCHAR* FemaleTag = TEXT("여");              // 여
	const TCHAR* MonsterDir = TEXT("몬스터"); // 몬스터

	// Grupos de acao do .act (acao = grupo * 8 + direcao), iguais ao cliente original.
	// Jogador: 0 parado, 1 andando, 2 sentado, 3 pegando, 4 em guarda, 5 ataque, 6 dano, 7 congelado, 8 morto...
	constexpr int32 GroupIdle = 0;
	constexpr int32 GroupWalk = 1;
	constexpr int32 GroupAttack = 5;
	constexpr int32 GroupHurt = 6;
	constexpr int32 GroupDead = 8;
	// Monstro: 0 parado, 1 andando, 2 ataque, 3 dano, 4 morte.
	constexpr int32 MobGroupAttack = 2;
	constexpr int32 MobGroupHurt = 3;
	constexpr int32 MobGroupDead = 4;

	// Materiais de sprite do Paper2D: textura "SpriteTexture" x cor do vertice, mascarados (com e sem iluminacao).
	const TCHAR* LitMaterialPath = TEXT("/Paper2D/MaskedLitSpriteMaterial.MaskedLitSpriteMaterial");
	const TCHAR* UnlitMaterialPath = TEXT("/Paper2D/MaskedUnlitSpriteMaterial.MaskedUnlitSpriteMaterial");
}

UMythRoSpriteComponent::UMythRoSpriteComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics; // depois do movimento e da camera do quadro
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CastShadow = false;
	bUseAsyncCooking = true;
	JobSpriteName = TEXT("초보자"); // 초보자 (Aprendiz)
}

FString UMythRoSpriteComponent::GetAssetsRoot()
{
	FString Root;
	if (GConfig && GConfig->GetString(TEXT("Myth"), TEXT("AssetsRoot"), Root, GGameIni) && !Root.IsEmpty())
	{
		return Root;
	}
	const FString Env = FPlatformMisc::GetEnvironmentVariable(TEXT("MYTH_ASSETS"));
	return Env.IsEmpty() ? FString() : FPaths::Combine(Env, TEXT("grf"));
}

void UMythRoSpriteComponent::BeginPlay()
{
	Super::BeginPlay();
	FeetRelativeLocation = GetRelativeLocation();
	ReloadSprites();

	if (bCastSpriteShadow && GetOwner())
	{
		ShadowMesh = NewObject<UProceduralMeshComponent>(GetOwner(), TEXT("RoSpriteShadow"));
		ShadowMesh->SetupAttachment(GetAttachParent());
		ShadowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ShadowMesh->SetRenderInMainPass(false);   // invisivel na imagem...
		ShadowMesh->SetCastShadow(true);           // ...mas projeta a sombra
		ShadowMesh->bCastDynamicShadow = true;
		ShadowMesh->RegisterComponent();
	}
	if (bBlobShadow)
	{
		CreateBlobShadow();
	}
	DayNight = AMythDayNightActor::Find(this);
	if (const UWorld* World = GetWorld())
	{
		for (TActorIterator<AMythNightLight> It(World); It; ++It)
		{
			NearbyLamps.Add(*It);
		}
	}
}

void UMythRoSpriteComponent::UpdateTint(const FVector& Feet, float DeltaTime)
{
	if (!bTintLighting || bLitByScene)
	{
		CurrentTint = FLinearColor::White;
		return;
	}
	if (!DayNight.IsValid())
	{
		DayNight = AMythDayNightActor::Find(this);
	}
	const float Night = DayNight.IsValid() ? DayNight->GetNightFactor() : 0.f;

	// Sombra "neles": a cada 0,2 s, um raio dos pes em direcao ao sol (canal Camera = so a malha visual do mapa).
	SunTraceTimer -= DeltaTime;
	if (SunTraceTimer <= 0.f && DayNight.IsValid() && GetWorld())
	{
		SunTraceTimer = 0.2f;
		const FVector Start = Feet + FVector(0.f, 0.f, 60.f);
		const FVector End = Start - DayNight->GetLightDirection() * 5000.f;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(MythSpriteSunTrace), false, GetOwner());
		FHitResult Hit;
		const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Camera, Params);
		SunVisibility = bBlocked ? 0.f : 1.f;
	}

	// Dia: branco (ou escurecido sob um predio); noite: azulado. Postes somam luz quente pela distancia.
	const float DayShade = FMath::Lerp(ShadowedTint, 1.f, SunVisibility);
	FLinearColor Target = FMath::Lerp(FLinearColor::White * DayShade, NightTint, Night);
	for (const TWeakObjectPtr<AMythNightLight>& Lamp : NearbyLamps)
	{
		if (!Lamp.IsValid() || !Lamp->Light)
		{
			continue;
		}
		const float Radius = Lamp->Light->AttenuationRadius;
		const float Dist = FVector::Dist(Lamp->GetActorLocation(), Feet);
		if (Dist >= Radius || Lamp->NightIntensity <= 0.f)
		{
			continue;
		}
		const float Falloff = FMath::Square(1.f - Dist / Radius);
		const float On = Lamp->Light->Intensity / Lamp->NightIntensity;
		Target += Lamp->Light->GetLightColor() * (Falloff * On * LampTintStrength);
	}
	Target.R = FMath::Clamp(Target.R, 0.f, 1.15f);
	Target.G = FMath::Clamp(Target.G, 0.f, 1.15f);
	Target.B = FMath::Clamp(Target.B, 0.f, 1.15f);
	Target.A = 1.f;
	// Transicao suave (entrar/sair da sombra de um predio, acender dos postes)
	const float Alpha = FMath::Clamp(DeltaTime * 5.f, 0.f, 1.f);
	CurrentTint = FMath::Lerp(CurrentTint, Target, Alpha);
}

void UMythRoSpriteComponent::CreateBlobShadow()
{
	const FString Base = FPaths::Combine(GetAssetsRoot(), TEXT("data"), TEXT("sprite"), TEXT("shadow"));
	if (!GetOwner() || !LoadSource(BlobSource, Base + TEXT(".spr"), Base + TEXT(".act"), FString()) || BlobSource.Spr.Indexed.Num() == 0)
	{
		return;
	}
	// A sombra do RO e uma elipse semitransparente: material translucido, alpha pela cor do vertice.
	if (UMaterialInterface* Translucent = LoadObject<UMaterialInterface>(nullptr, TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial")))
	{
		BlobSource.Material = UMaterialInstanceDynamic::Create(Translucent, this);
		BlobSource.Material->SetTextureParameterValue(TEXT("SpriteTexture"), BlobSource.Atlas);
	}
	BlobMesh = NewObject<UProceduralMeshComponent>(GetOwner(), TEXT("RoSpriteBlobShadow"));
	BlobMesh->SetupAttachment(GetAttachParent());
	BlobMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BlobMesh->SetCastShadow(false);
	BlobMesh->RegisterComponent();

	const FRoSprImage& Img = BlobSource.Spr.Indexed[0];
	const FBox2f UV = BlobSource.IndexedUV[0];
	const float HalfW = Img.Width * UnitsPerPixel * BlobShadowScale * 0.5f;
	const float HalfH = Img.Height * UnitsPerPixel * BlobShadowScale * 0.5f;
	const TArray<FVector> Verts = { {-HalfH, -HalfW, 0.f}, {-HalfH, HalfW, 0.f}, {HalfH, HalfW, 0.f}, {HalfH, -HalfW, 0.f} };
	const TArray<FVector2D> Tex = { {UV.Min.X, UV.Min.Y}, {UV.Max.X, UV.Min.Y}, {UV.Max.X, UV.Max.Y}, {UV.Min.X, UV.Max.Y} };
	const uint8 BlobAlpha = 165; // o shadow.act pede 128 sobre o fundo do RO; aqui o chao HD (Lumen) e mais claro
	const TArray<FColor> Colors = { FColor(255, 255, 255, BlobAlpha), FColor(255, 255, 255, BlobAlpha), FColor(255, 255, 255, BlobAlpha), FColor(255, 255, 255, BlobAlpha) };
	const TArray<FVector> Normals = { FVector::UpVector, FVector::UpVector, FVector::UpVector, FVector::UpVector };
	const TArray<int32> Tris = { 0, 1, 2, 0, 2, 3, 0, 2, 1, 0, 3, 2 };
	BlobMesh->CreateMeshSection(0, Verts, Tris, Normals, Tex, Colors, TArray<FProcMeshTangent>(), false);
	BlobMesh->SetMaterial(0, BlobSource.Material);
}

void UMythRoSpriteComponent::PlayFrameEvent(const FRoActFrame& Frame, int32 ActionIndex, int32 FrameIndex)
{
	const FIntPoint Key(ActionIndex, FrameIndex);
	if (Key == LastSoundFrame)
	{
		return;
	}
	LastSoundFrame = Key;
	if (!bPlayActSounds || !Body.Act.Events.IsValidIndex(Frame.EventId))
	{
		return;
	}
	// Eventos do .act: nomes de .wav tocam o som; "atk" so marca o instante do golpe (sem som proprio).
	const FString& Event = Body.Act.Events[Frame.EventId];
	if (Event.EndsWith(TEXT(".wav"), ESearchCase::IgnoreCase) && GetOwner())
	{
		UMythRoSound::PlayWavAtLocation(this, Event, GetOwner()->GetActorLocation());
	}
}

void UMythRoSpriteComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (ShadowMesh)
	{
		ShadowMesh->DestroyComponent();
		ShadowMesh = nullptr;
	}
	if (BlobMesh)
	{
		BlobMesh->DestroyComponent();
		BlobMesh = nullptr;
	}
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UMythRoSpriteComponent::UpdateShadowTransform(const FVector& Feet)
{
	if (!ShadowMesh)
	{
		return;
	}
	if (!DayNight.IsValid())
	{
		DayNight = AMythDayNightActor::Find(this);
	}
	// Plano em pe, de frente para a luz (so o yaw da luz importa): a silhueta inteira vira sombra no chao.
	FVector LightDir = DayNight.IsValid() ? DayNight->GetLightDirection() : FVector(0.3f, 0.2f, -1.f);

	// Comprimento da sombra no chao = altura do sprite / tan(elevacao da luz). Perto do meio-dia ela fica do
	// tamanho do blob: a copia em pe e achatada ate sumir (fica so o blob) e cresce de novo quando o sol baixa.
	const float Horizontal = FVector2D(LightDir.X, LightDir.Y).Size();
	const float Down = FMath::Max(-LightDir.Z, KINDA_SMALL_NUMBER);
	const FBox& Frame = GetFrameLocalBounds();
	const float Height = Frame.IsValid ? FMath::Max(Frame.GetSize().Z, 40.f) : 90.f;
	const float ShadowLength = Height * Horizontal / Down;
	const float Fade = ShadowFadeLength > 0.f ? FMath::Clamp((ShadowLength - ShadowFadeLength) / ShadowFadeLength, 0.f, 1.f) : 1.f;
	// Sprites baixos (Poring, Fabre...) projetam uma mancha estranha: ficam so com a sombra redonda, como no RO
	const bool bShowCast = bCastSpriteShadow && Fade > 0.02f && Height >= MinCastShadowHeight;
	if (ShadowMesh->CastShadow != bShowCast)
	{
		ShadowMesh->SetCastShadow(bShowCast);
	}
	ShadowMesh->SetWorldScale3D(FVector(1.f, 1.f, FMath::Max(Fade, 0.02f)));

	LightDir.Z = 0.f;
	if (!LightDir.Normalize())
	{
		LightDir = FVector::ForwardVector; // luz a pino: qualquer orientacao serve
	}
	const FVector Right = FVector::CrossProduct(FVector::UpVector, LightDir).GetSafeNormal();
	// A base fica um pouco abaixo do chao: a sombra nasce embaixo do centro da elipse, que esconde a emenda
	ShadowMesh->SetWorldLocationAndRotation(Feet - FVector(0.f, 0.f, CastShadowSink), FRotationMatrix::MakeFromXZ(Right, FVector::UpVector).Rotator());
}

void UMythRoSpriteComponent::ReloadSprites()
{
	const FString Root = GetAssetsRoot();
	if (IsMonster())
	{
		const FString Folder = MonsterSpriteFolder.IsEmpty() ? FString(MonsterDir) : MonsterSpriteFolder;
		const FString Base = FPaths::Combine(Root, TEXT("data"), TEXT("sprite"), Folder, MonsterSpriteName);
		Head = FMythRoSpriteSource();
		if (!LoadSource(Body, Base + TEXT(".spr"), Base + TEXT(".act"), FString()))
		{
			UE_LOG(LogTemp, Warning, TEXT("MythRoSprite: falha ao carregar o monstro '%s' (raiz '%s')."), *MonsterSpriteName, *Root);
		}
		return;
	}

	const FString Sex = bMale ? MaleTag : FemaleTag;
	const FString Sprite = FPaths::Combine(Root, TEXT("data"), TEXT("sprite"), HumanDir);

	const FString BodyBase = FPaths::Combine(Sprite, BodyDir, Sex, FString::Printf(TEXT("%s_%s"), *JobSpriteName, *Sex));
	const FString HeadBase = FPaths::Combine(Sprite, HeadDir, Sex, FString::Printf(TEXT("%d_%s"), HairStyle, *Sex));
	const FString Palette = HairColor > 0
		? FPaths::Combine(Root, TEXT("data"), TEXT("palette"), HairPalDir, FString::Printf(TEXT("%s%d_%s_%d.pal"), HairPalDir, HairStyle, *Sex, HairColor))
		: FString();

	const bool bBody = LoadSource(Body, BodyBase + TEXT(".spr"), BodyBase + TEXT(".act"), FString());
	const bool bHead = LoadSource(Head, HeadBase + TEXT(".spr"), HeadBase + TEXT(".act"), Palette);
	if (!bBody || !bHead)
	{
		UE_LOG(LogTemp, Warning, TEXT("MythRoSprite: falha ao carregar sprites (raiz '%s'). Corpo=%d Cabeca=%d. Configure [Myth] AssetsRoot no DefaultGame.ini."),
			*Root, bBody, bHead);
	}
}

bool UMythRoSpriteComponent::LoadSource(FMythRoSpriteSource& Source, const FString& SprPath, const FString& ActPath, const FString& PalettePath)
{
	Source = FMythRoSpriteSource();
	TArray<uint8> SprBytes, ActBytes, PalBytes;
	if (!FFileHelper::LoadFileToArray(SprBytes, *SprPath) || !FFileHelper::LoadFileToArray(ActBytes, *ActPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("MythRoSprite: arquivo nao encontrado: %s"), *SprPath);
		return false;
	}

	TArray<FColor> Palette;
	const bool bPalette = !PalettePath.IsEmpty() && FFileHelper::LoadFileToArray(PalBytes, *PalettePath) && FRoSpr::LoadPalette(PalBytes, Palette);
	if (!Source.Spr.Load(SprBytes, bPalette ? &Palette : nullptr) || !Source.Act.Load(ActBytes))
	{
		UE_LOG(LogTemp, Warning, TEXT("MythRoSprite: arquivo invalido: %s"), *SprPath);
		return false;
	}
	BuildAtlas(Source);

	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, bLitByScene ? LitMaterialPath : UnlitMaterialPath))
	{
		Source.Material = UMaterialInstanceDynamic::Create(Base, this);
		Source.Material->SetTextureParameterValue(TEXT("SpriteTexture"), Source.Atlas);
	}
	return Source.IsValid() && Source.Material != nullptr;
}

void UMythRoSpriteComponent::BuildAtlas(FMythRoSpriteSource& Source)
{
	// Empacota todas as imagens em prateleiras (1 px de margem), indexadas e depois RGBA.
	constexpr int32 MaxWidth = 1024;
	TArray<const FRoSprImage*> Images;
	for (const FRoSprImage& Img : Source.Spr.Indexed) { Images.Add(&Img); }
	for (const FRoSprImage& Img : Source.Spr.Rgba) { Images.Add(&Img); }

	TArray<FIntPoint> Positions;
	Positions.SetNum(Images.Num());
	int32 X = 0, Y = 0, ShelfH = 0, UsedW = 1;
	for (int32 i = 0; i < Images.Num(); ++i)
	{
		const FIntPoint Size(Images[i]->Width + 1, Images[i]->Height + 1);
		if (X + Size.X > MaxWidth)
		{
			X = 0;
			Y += ShelfH;
			ShelfH = 0;
		}
		Positions[i] = FIntPoint(X, Y);
		X += Size.X;
		UsedW = FMath::Max(UsedW, X);
		ShelfH = FMath::Max(ShelfH, Size.Y);
	}
	const int32 W = FMath::Max(UsedW, 1);
	const int32 H = FMath::Max(Y + ShelfH, 1);

	TArray<FColor> Pixels;
	Pixels.SetNumZeroed(W * H);
	for (int32 i = 0; i < Images.Num(); ++i)
	{
		const FRoSprImage& Img = *Images[i];
		for (int32 Row = 0; Row < Img.Height; ++Row)
		{
			FMemory::Memcpy(&Pixels[(Positions[i].Y + Row) * W + Positions[i].X], &Img.Pixels[Row * Img.Width], Img.Width * sizeof(FColor));
		}
		const FBox2f UV(FVector2f(float(Positions[i].X) / W, float(Positions[i].Y) / H),
		                FVector2f(float(Positions[i].X + Img.Width) / W, float(Positions[i].Y + Img.Height) / H));
		(i < Source.Spr.Indexed.Num() ? Source.IndexedUV : Source.RgbaUV).Add(UV);
	}

	UTexture2D* Tex = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
	if (!Tex)
	{
		return;
	}
	Tex->Filter = TF_Nearest; // pixel art sem filtro, como no original
	Tex->SRGB = true;
	Tex->NeverStream = true;
	Tex->LODGroup = TEXTUREGROUP_Pixels2D;
	FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FColor)); // FColor ja e BGRA na memoria
	Mip.BulkData.Unlock();
	Tex->UpdateResource();
	Source.Atlas = Tex;
}

// ------------------------------------------------------------------------------------------- estado

void UMythRoSpriteComponent::SetState(EMythSpriteState NewState, bool bOneShot)
{
	if (State == EMythSpriteState::Dead && NewState != EMythSpriteState::Idle)
	{
		return;
	}
	State = NewState;
	bOneShotPlaying = bOneShot;
	StateTime = 0.f;
	PlayRate = 1.f;
}

void UMythRoSpriteComponent::PlayAttack(float AttackDuration)
{
	SetState(EMythSpriteState::Attack, true);
	if (AttackDuration > 0.f && Body.IsValid())
	{
		const FRoActAction* Action = Body.Act.Actions.IsValidIndex(GetActionIndex()) ? &Body.Act.Actions[GetActionIndex()] : nullptr;
		if (Action)
		{
			// Acelera a animacao para caber no intervalo do ASPD (nunca desacelera), como o original.
			const float Total = Action->Frames.Num() * Action->IntervalMs / 1000.f;
			PlayRate = FMath::Max(1.f, Total / AttackDuration);
		}
	}
}

void UMythRoSpriteComponent::PlayHit() { SetState(EMythSpriteState::Hit, true); }
void UMythRoSpriteComponent::Die()     { SetState(EMythSpriteState::Dead, false); }

void UMythRoSpriteComponent::Revive()
{
	State = EMythSpriteState::Idle;
	bOneShotPlaying = false;
	StateTime = 0.f;
}

void UMythRoSpriteComponent::FaceLocation(const FVector& WorldLocation)
{
	if (const AActor* Owner = GetOwner())
	{
		Direction = URagnarokDirectionHelper::CalculateSpriteDirection(WorldLocation - Owner->GetActorLocation(), GetCameraRotation(), Direction);
	}
}

int32 UMythRoSpriteComponent::GetActionIndex() const
{
	const bool bMob = IsMonster();
	int32 Group = GroupIdle;
	switch (State)
	{
	case EMythSpriteState::Idle:   Group = GroupIdle; break;
	case EMythSpriteState::Walk:   Group = GroupWalk; break;
	case EMythSpriteState::Attack: Group = bMob ? MobGroupAttack : GroupAttack; break;
	case EMythSpriteState::Hit:    Group = bMob ? MobGroupHurt : GroupHurt; break;
	case EMythSpriteState::Dead:   Group = bMob ? MobGroupDead : GroupDead; break;
	}
	return Group * 8 + static_cast<int32>(Direction);
}

int32 UMythRoSpriteComponent::GetFrameIndex(const FRoActAction& Action, bool& bOutFinished) const
{
	bOutFinished = false;
	const int32 Count = Action.Frames.Num();
	if (Count == 0)
	{
		return 0;
	}
	if (State == EMythSpriteState::Idle && !IsMonster())
	{
		return 0; // jogador parado: quadro 0 = cabeca olhando para frente (monstros animam o idle em loop)
	}
	const int32 Step = FMath::FloorToInt(StateTime * PlayRate * 1000.f / FMath::Max(Action.IntervalMs, 1.f));
	if (bOneShotPlaying || State == EMythSpriteState::Dead)
	{
		bOutFinished = Step >= Count;
		return FMath::Min(Step, Count - 1);
	}
	return Step % Count;
}

// ------------------------------------------------------------------------------------------- tick

FRotator UMythRoSpriteComponent::GetCameraRotation() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const APlayerController* PC = World->GetFirstPlayerController())
		{
			if (PC->PlayerCameraManager)
			{
				return PC->PlayerCameraManager->GetCameraRotation();
			}
		}
	}
	return FRotator::ZeroRotator;
}

void UMythRoSpriteComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const AActor* Owner = GetOwner();
	if (!Owner || !Body.IsValid())
	{
		return;
	}
	StateTime += DeltaTime;
	const FRotator CamRot = GetCameraRotation();

	if (State != EMythSpriteState::Dead)
	{
		const FVector Velocity = Owner->GetVelocity();
		if (bOneShotPlaying)
		{
			bool bFinished = false;
			if (Body.Act.Actions.IsValidIndex(GetActionIndex()))
			{
				GetFrameIndex(Body.Act.Actions[GetActionIndex()], bFinished);
			}
			if (bFinished)
			{
				SetState(EMythSpriteState::Idle, false);
			}
		}
		else
		{
			Direction = ComputeDirection(Velocity, CamRot);
			const bool bMoving = Velocity.Size2D() > WalkSpeedThreshold;
			StillTime = bMoving ? 0.f : StillTime + DeltaTime;
			// So volta ao Idle depois de IdleDelay parado: quinas do caminho nao reiniciam o ciclo de caminhada.
			const EMythSpriteState NewState = bMoving ? EMythSpriteState::Walk : (StillTime >= IdleDelay ? EMythSpriteState::Idle : State);
			if (NewState != State)
			{
				SetState(NewState, false);
			}
		}
	}

	UpdateBillboard(CamRot);
	RebuildMesh(CamRot);
}

ERagnarokDirection UMythRoSpriteComponent::ComputeDirection(const FVector& Velocity, const FRotator& CamRot) const
{
	if (Velocity.SizeSquared2D() < FMath::Square(WalkSpeedThreshold))
	{
		return Direction;
	}
	// Mantem a direcao atual enquanto o movimento estiver ate 22,5 + histerese graus do centro dela.
	const float MoveAngle = FMath::RadiansToDegrees(FMath::Atan2(Velocity.Y, Velocity.X));
	const float Relative = FRotator::NormalizeAxis(MoveAngle - CamRot.Yaw);
	const float Center = URagnarokDirectionHelper::DirectionToAngle(Direction);
	if (FMath::Abs(FRotator::NormalizeAxis(Relative - Center)) <= 22.5f + DirectionHysteresisDeg)
	{
		return Direction;
	}
	return URagnarokDirectionHelper::CalculateSpriteDirection(Velocity, CamRot, Direction);
}

void UMythRoSpriteComponent::UpdateBillboard(const FRotator& CamRot)
{
	// Paralelo a tela (yaw e pitch da camera), como no cliente original, com os pes no ponto de apoio.
	// Para nao ser cortado por paredes/chao, o sprite e puxado AO LONGO DA LINHA DE VISAO da camera:
	// isso nao muda tamanho nem posicao na tela, so a profundidade.
	const FRotationMatrix CamMatrix(CamRot);
	const FVector CamRight = CamMatrix.GetUnitAxis(EAxis::Y);
	const FVector CamUp = CamMatrix.GetUnitAxis(EAxis::Z);
	const FVector CamForward = CamMatrix.GetUnitAxis(EAxis::X);
	SetWorldRotation(FRotationMatrix::MakeFromXZ(CamRight, CamUp).Rotator());
	if (const USceneComponent* Parent = GetAttachParent())
	{
		FVector Feet = Parent->GetComponentTransform().TransformPosition(FeetRelativeLocation);
		// A capsula sobe/desce nos degraus da malha do chao; suavizar a altura tira o "tremido" ao andar.
		const float Dt = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;
		if (!bFeetZInitialized || FeetHeightSmoothing <= 0.f || FMath::Abs(Feet.Z - SmoothedFeetZ) > 100.f)
		{
			SmoothedFeetZ = Feet.Z;
			bFeetZInitialized = true;
		}
		else
		{
			SmoothedFeetZ = FMath::FInterpTo(SmoothedFeetZ, Feet.Z, Dt, FeetHeightSmoothing);
		}
		Feet.Z = SmoothedFeetZ;
		// Puxado para a camera pela linha de visao; a escala compensa a perspectiva (mesmo tamanho na tela).
		// O puxao segue o RAIO camera->pes (nao o eixo da camera): assim o sprite fica exatamente sobre o ponto do
		// chao na tela, alinhado com a sombra redonda e com a area de clique, tambem nas bordas da tela.
		float Scale = 1.f;
		FVector PullDir = CamForward;
		if (const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			if (PC->PlayerCameraManager)
			{
				const FVector ToFeet = Feet - PC->PlayerCameraManager->GetCameraLocation();
				const float Dist = ToFeet.Size();
				Scale = Dist > CameraDepthOffset + 1.f ? (Dist - CameraDepthOffset) / Dist : 1.f;
				PullDir = Dist > KINDA_SMALL_NUMBER ? ToFeet / Dist : CamForward;
			}
		}
		SetWorldLocation(Feet - PullDir * CameraDepthOffset);
		SetWorldScale3D(FVector(Scale));
		UpdateShadowTransform(Feet);
		UpdateTint(Feet, Dt);
		if (BlobMesh)
		{
			// No cliente original o shadow.spr e desenhado direto na tela. Deitado no chao ele achatava de novo pela
			// inclinacao da camera e girava com ela: aqui fica alinhado a camera e a profundidade e esticada por
			// 1/sen(inclinacao), entao na tela a elipse tem exatamente a proporcao do sprite original.
			const float SinPitch = FMath::Max(FMath::Abs(FMath::Sin(FMath::DegreesToRadians(CamRot.Pitch))), 0.25f);
			// Apoia no chao DESENHADO (a malha visual so bloqueia o canal Camera); o chao de colisao do GAT pode ficar
			// alguns cm acima ou abaixo dele, e o blob sumia por baixo da calcada.
			FVector BlobAt = Feet + FVector(0.f, 0.f, 5.f);
			FHitResult GroundHit;
			FCollisionQueryParams BlobParams(TEXT("RoBlobGround"), false, GetOwner());
			if (GetWorld()->LineTraceSingleByChannel(GroundHit, Feet + FVector(0.f, 0.f, 60.f), Feet - FVector(0.f, 0.f, 60.f), ECC_Camera, BlobParams))
			{
				BlobAt.Z = GroundHit.ImpactPoint.Z + 3.f;
			}
			BlobMesh->SetWorldLocationAndRotation(BlobAt, FRotator(0.f, CamRot.Yaw, 0.f));
			BlobMesh->SetWorldScale3D(FVector(1.f / SinPitch, 1.f, 1.f));
		}
	}
}

void UMythRoSpriteComponent::RebuildMesh(const FRotator& CamRot)
{
	const int32 ActionIndex = GetActionIndex();
	if (!Body.Act.Actions.IsValidIndex(ActionIndex))
	{
		return;
	}
	bool bFinished = false;
	const FRoActAction& BodyAction = Body.Act.Actions[ActionIndex];
	const int32 FrameIndex = GetFrameIndex(BodyAction, bFinished);
	const FRoActFrame& BodyFrame = BodyAction.Frames[FrameIndex];

	PlayFrameEvent(BodyFrame, ActionIndex, FrameIndex);

	int32 Section = 0;
	FrameLocalBounds = FBox(ForceInit);
	AddLayerQuads(Body, BodyFrame, FIntPoint::ZeroValue, 0.f, Section);

	if (Head.IsValid() && Head.Act.Actions.IsValidIndex(ActionIndex) && Head.Act.Actions[ActionIndex].Frames.Num() > 0)
	{
		const FRoActAction& HeadAction = Head.Act.Actions[ActionIndex];
		const FRoActFrame& HeadFrame = HeadAction.Frames[FrameIndex % HeadAction.Frames.Num()];
		// A cabeca encaixa no corpo pela diferenca entre as ancoras dos dois quadros.
		FIntPoint Offset = FIntPoint::ZeroValue;
		if (BodyFrame.Anchors.Num() > 0 && HeadFrame.Anchors.Num() > 0)
		{
			Offset = BodyFrame.Anchors[0] - HeadFrame.Anchors[0];
		}
		AddLayerQuads(Head, HeadFrame, Offset, 2.f, Section);
	}

	for (int32 i = Section; i < SectionsCreated; ++i)
	{
		SetMeshSectionVisible(i, false);
		if (ShadowMesh)
		{
			ShadowMesh->SetMeshSectionVisible(i, false);
		}
	}
}

void UMythRoSpriteComponent::AddLayerQuads(const FMythRoSpriteSource& Source, const FRoActFrame& Frame, const FIntPoint& Offset, float DepthBase, int32& Section)
{
	// No espaco local (ver UpdateBillboard): X = direita da tela, Z = cima da tela, Y = Z x X = -Forward da camera.
	const float S = UnitsPerPixel;
	for (int32 LayerIndex = 0; LayerIndex < Frame.Layers.Num(); ++LayerIndex)
	{
		const FRoActLayer& L = Frame.Layers[LayerIndex];
		const FRoSprImage* Img = Source.Spr.Find(L.SprIndex, L.SprType);
		const TArray<FBox2f>& UVs = L.SprType == 1 ? Source.RgbaUV : Source.IndexedUV;
		if (!Img || !UVs.IsValidIndex(L.SprIndex))
		{
			continue;
		}
		const FBox2f UV = UVs[L.SprIndex];
		const float HalfW = Img->Width * L.Scale.X * 0.5f;
		const float HalfH = Img->Height * L.Scale.Y * 0.5f;
		const float CenterX = L.Offset.X + Offset.X;
		const float CenterY = L.Offset.Y + Offset.Y;
		const float Rad = FMath::DegreesToRadians(static_cast<float>(L.RotationDeg));
		const float C = FMath::Cos(Rad), Sn = FMath::Sin(Rad);
		// Cada camada fica um pouco mais perto da camera que a anterior (evita z-fighting).
		const float Depth = DepthBase + LayerIndex * 0.5f;

		// Cantos em pixels (Y para baixo): sup-esq, sup-dir, inf-dir, inf-esq.
		const FVector2f Corners[4] = { {-HalfW, -HalfH}, {HalfW, -HalfH}, {HalfW, HalfH}, {-HalfW, HalfH} };
		const float U0 = L.bMirror ? UV.Max.X : UV.Min.X;
		const float U1 = L.bMirror ? UV.Min.X : UV.Max.X;
		const FVector2D CornerUV[4] = { {U0, UV.Min.Y}, {U1, UV.Min.Y}, {U1, UV.Max.Y}, {U0, UV.Max.Y} };

		TArray<FVector> Verts;
		TArray<FVector2D> Tex;
		TArray<FColor> Colors;
		TArray<FVector> Normals;
		for (int32 k = 0; k < 4; ++k)
		{
			const float Px = CenterX + Corners[k].X * C - Corners[k].Y * Sn;
			const float Py = CenterY + Corners[k].X * Sn + Corners[k].Y * C;
			Verts.Add(FVector(Px * S, Depth, -Py * S));
			FrameLocalBounds += Verts.Last();
			Tex.Add(CornerUV[k]);
			// Cor da camada (.act) x luz calculada (dia/noite, postes, sombra de predio); o alpha fica o da camada.
			FColor Tinted = (FLinearColor(L.Color) * CurrentTint).ToFColor(false);
			Tinted.A = L.Color.A;
			Colors.Add(Tinted);
			// Normal inclinada para cima e para a camera: o sprite iluminado recebe sol/ceu como o chao
			// e nao escurece quando o sol esta atras dele.
			Normals.Add(FVector(0.f, 0.45f, 0.89f));
		}

		UMaterialInterface* Mat = Source.Material;
		// Dois triangulos nos dois sentidos: o quad aparece mesmo se o winding ficar invertido.
		static const TArray<int32> Tris = { 0, 1, 2, 0, 2, 3, 0, 2, 1, 0, 3, 2 };
		if (Section < SectionsCreated)
		{
			UpdateMeshSection(Section, Verts, Normals, Tex, Colors, TArray<FProcMeshTangent>());
			SetMeshSectionVisible(Section, true);
			if (ShadowMesh)
			{
				ShadowMesh->UpdateMeshSection(Section, Verts, Normals, Tex, Colors, TArray<FProcMeshTangent>());
				ShadowMesh->SetMeshSectionVisible(Section, true);
			}
		}
		else
		{
			CreateMeshSection(Section, Verts, Tris, Normals, Tex, Colors, TArray<FProcMeshTangent>(), false);
			if (ShadowMesh)
			{
				ShadowMesh->CreateMeshSection(Section, Verts, Tris, Normals, Tex, Colors, TArray<FProcMeshTangent>(), false);
			}
			++SectionsCreated;
		}
		if (GetMaterial(Section) != Mat)
		{
			SetMaterial(Section, Mat);
			if (ShadowMesh)
			{
				ShadowMesh->SetMaterial(Section, Mat); // mascarado: a sombra tem o recorte do sprite
			}
		}
		++Section;
	}
}
