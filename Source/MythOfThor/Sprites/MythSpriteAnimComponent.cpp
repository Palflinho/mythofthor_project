#include "Sprites/MythSpriteAnimComponent.h"

#include "Sprites/MythSpriteSet.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "PaperFlipbook.h"
#include "TimerManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "World/MythMapInfo.h"

UMythSpriteAnimComponent::UMythSpriteAnimComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetLooping(true);
	SetCastShadow(true); // GDD Técnico HD-2D: sprites projetam sombras sob luz solar e tochas
}

void UMythSpriteAnimComponent::BeginPlay()
{
	Super::BeginPlay();
	OnFinishedPlaying.AddDynamic(this, &UMythSpriteAnimComponent::HandleFinishedPlaying);

	// Detecta automaticamente se o mapa atual possui plano de agua configurado
	if (const UWorld* World = GetWorld())
	{
		if (const AMythMapInfo* MapInfo = AMythMapInfo::Find(World))
		{
			if (MapInfo->bHasWater)
			{
				bEnableWaterReflection = true;
				WaterSurfaceZ = MapInfo->WaterLevelUU;
			}
		}
	}

	if (bEnableWaterReflection && !WaterReflectionComponent)
	{
		if (AActor* Owner = GetOwner())
		{
			WaterReflectionComponent = NewObject<UPaperFlipbookComponent>(Owner, TEXT("WaterReflectionSpriteComponent"));
			if (WaterReflectionComponent)
			{
				WaterReflectionComponent->SetupAttachment(Owner->GetRootComponent());
				WaterReflectionComponent->RegisterComponent();
				WaterReflectionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				WaterReflectionComponent->SetCastShadow(false);
				WaterReflectionComponent->SetTranslucentSortPriority(-5);
				WaterReflectionComponent->SetLooping(true);
				WaterReflectionComponent->SetRelativeScale3D(FVector(1.f, 1.f, -1.f)); // Espelhamento no eixo Z
			}
		}
	}

	ApplyFlipbook(false);
}

FRotator UMythSpriteAnimComponent::GetCameraRotation() const
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

UPaperFlipbook* UMythSpriteAnimComponent::FindFlipbook(EMythSpriteState InState, ERagnarokDirection InDir) const
{
	if (!SpriteSet)
	{
		return nullptr;
	}
	const TArray<TObjectPtr<UPaperFlipbook>>* List = nullptr;
	switch (InState)
	{
	case EMythSpriteState::Idle:   List = &SpriteSet->Idle; break;
	case EMythSpriteState::Walk:   List = &SpriteSet->Walk; break;
	case EMythSpriteState::Attack: List = &SpriteSet->Attack; break;
	case EMythSpriteState::Hit:    List = &SpriteSet->Hit; break;
	case EMythSpriteState::Dead:   List = &SpriteSet->Die; break;
	}
	const int32 Index = static_cast<int32>(InDir);
	return (List && List->IsValidIndex(Index)) ? (*List)[Index].Get() : nullptr;
}

void UMythSpriteAnimComponent::ApplyFlipbook(bool bKeepPlaybackPosition)
{
	UPaperFlipbook* Flipbook = FindFlipbook(State, Direction);
	if (!Flipbook)
	{
		return;
	}

	const float Position = GetPlaybackPosition();
	const bool bLoop = (State == EMythSpriteState::Idle || State == EMythSpriteState::Walk);

	if (GetFlipbook() != Flipbook)
	{
		SetFlipbook(Flipbook);
	}
	SetLooping(bLoop);
	if (State != EMythSpriteState::Attack)
	{
		SetPlayRate(1.f);
	}
	// Trocar so a direcao nao deve reiniciar o ciclo de caminhada.
	SetPlaybackPosition(bKeepPlaybackPosition ? FMath::Min(Position, Flipbook->GetTotalDuration()) : 0.f, false);
	Play();

	// Sincroniza o Ghost Sprite Espelhado
	if (WaterReflectionComponent)
	{
		if (WaterReflectionComponent->GetFlipbook() != Flipbook)
		{
			WaterReflectionComponent->SetFlipbook(Flipbook);
		}
		WaterReflectionComponent->SetLooping(bLoop);
		WaterReflectionComponent->SetPlaybackPosition(GetPlaybackPosition(), false);
		WaterReflectionComponent->Play();
	}
}

void UMythSpriteAnimComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const FRotator CamRot = GetCameraRotation();

	// Billboard em yaw: o plano do sprite (normal -Y) fica de frente para a camera.
	SetWorldRotation(FRotator(0.f, CamRot.Yaw - 90.f, 0.f));

	// Atualiza o Ghost Sprite Espelhado de agua (GDD Tecnico HD-2D)
	if (WaterReflectionComponent && bEnableWaterReflection)
	{
		const bool bReflectVisible = IsVisible() && (State != EMythSpriteState::Dead);
		WaterReflectionComponent->SetVisibility(bReflectVisible);
		if (bReflectVisible)
		{
			const FVector SpriteLoc = GetComponentLocation();
			// Espelha no eixo Z atraves do plano d'agua (Z_refletido = 2 * WaterZ - SpriteZ)
			const float ReflectedZ = 2.f * WaterSurfaceZ - SpriteLoc.Z;
			WaterReflectionComponent->SetWorldLocation(FVector(SpriteLoc.X, SpriteLoc.Y, ReflectedZ));
			WaterReflectionComponent->SetWorldRotation(FRotator(0.f, CamRot.Yaw - 90.f, 0.f));
			WaterReflectionComponent->SetWorldScale3D(FVector(GetComponentScale().X, GetComponentScale().Y, -GetComponentScale().Z));
			WaterReflectionComponent->SetPlaybackPosition(GetPlaybackPosition(), false);
		}
	}

	if (State == EMythSpriteState::Dead)
	{
		return;
	}

	const FVector Velocity = Owner->GetVelocity();

	// Direcao e estado de locomotion so mudam fora de one-shots (ataque/hit mantem a direcao inicial).
	if (bOneShotPlaying)
	{
		return;
	}

	const ERagnarokDirection NewDir = URagnarokDirectionHelper::CalculateSpriteDirection(Velocity, CamRot, Direction);
	const EMythSpriteState NewState = Velocity.Size2D() > WalkSpeedThreshold ? EMythSpriteState::Walk : EMythSpriteState::Idle;

	const bool bDirChanged = NewDir != Direction;
	const bool bStateChanged = NewState != State;
	Direction = NewDir;
	State = NewState;

	if (bStateChanged || bDirChanged)
	{
		ApplyFlipbook(/*bKeepPlaybackPosition=*/!bStateChanged);
	}
}

void UMythSpriteAnimComponent::EnterOneShot(EMythSpriteState NewState)
{
	if (State == EMythSpriteState::Dead)
	{
		return;
	}
	State = NewState;
	bOneShotPlaying = (NewState != EMythSpriteState::Dead);
	ApplyFlipbook(false);
}

void UMythSpriteAnimComponent::HandleFinishedPlaying()
{
	if (bOneShotPlaying)
	{
		// Volta ao locomotion; o proximo Tick escolhe Idle/Walk.
		bOneShotPlaying = false;
		State = EMythSpriteState::Idle;
		ApplyFlipbook(false);
	}
	// Dead permanece no ultimo frame (flipbook nao-loop termina parado).
}

void UMythSpriteAnimComponent::PlayAttack(float AttackDuration)
{
	EnterOneShot(EMythSpriteState::Attack);
	if (AttackDuration > 0.f)
	{
		if (const UPaperFlipbook* FB = GetFlipbook())
		{
			const float Total = FB->GetTotalDuration();
			if (Total > AttackDuration)
			{
				SetPlayRate(Total / AttackDuration);
			}
		}
	}
}
void UMythSpriteAnimComponent::PlayHit()
{
	EnterOneShot(EMythSpriteState::Hit);
	TriggerHitstop(0.04f); // 2 a 3 frames (0.03s a 0.05s) conforme GDD Tecnico HD-2D
	TriggerHitFlash(0.05f);
}

void UMythSpriteAnimComponent::TriggerHitstop(float Duration)
{
	if (Duration <= 0.f)
	{
		return;
	}

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}

	if (!bIsHitstopped)
	{
		SavedCustomTimeDilation = Owner->CustomTimeDilation;
		bIsHitstopped = true;
	}

	// Pausa fisica quase completa no tempo do ator para imprimir peso ao impacto
	Owner->CustomTimeDilation = 0.001f;

	World->GetTimerManager().ClearTimer(HitstopTimerHandle);
	World->GetTimerManager().SetTimer(HitstopTimerHandle, this, &UMythSpriteAnimComponent::EndHitstop, Duration, false);
}

void UMythSpriteAnimComponent::EndHitstop()
{
	if (bIsHitstopped)
	{
		if (AActor* Owner = GetOwner())
		{
			Owner->CustomTimeDilation = SavedCustomTimeDilation;
		}
		bIsHitstopped = false;
	}
}

void UMythSpriteAnimComponent::TriggerHitFlash(float Duration)
{
	if (UMaterialInstanceDynamic* DynMat = GetMaterial(0) ? Cast<UMaterialInstanceDynamic>(GetMaterial(0)) : CreateDynamicMaterialInstance(0))
	{
		DynMat->SetScalarParameterValue(TEXT("HitFlash"), 1.0f);
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(HitFlashTimerHandle);
			World->GetTimerManager().SetTimer(HitFlashTimerHandle, this, &UMythSpriteAnimComponent::EndHitFlash, Duration, false);
		}
	}
}

void UMythSpriteAnimComponent::EndHitFlash()
{
	if (UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(GetMaterial(0)))
	{
		DynMat->SetScalarParameterValue(TEXT("HitFlash"), 0.0f);
	}
}

void UMythSpriteAnimComponent::Die()        { EnterOneShot(EMythSpriteState::Dead); }

void UMythSpriteAnimComponent::Revive()
{
	bOneShotPlaying = false;
	State = EMythSpriteState::Idle;
	ApplyFlipbook(false);
}

void UMythSpriteAnimComponent::FaceLocation(const FVector& WorldLocation)
{
	if (const AActor* Owner = GetOwner())
	{
		Direction = URagnarokDirectionHelper::CalculateSpriteDirection(WorldLocation - Owner->GetActorLocation(), GetCameraRotation(), Direction);
		ApplyFlipbook(true);
	}
}

void UMythSpriteAnimComponent::SetupWaterReflection(bool bEnable, float InWaterSurfaceZ, float InOpacity)
{
	bEnableWaterReflection = bEnable;
	WaterSurfaceZ = InWaterSurfaceZ;
	ReflectionOpacity = InOpacity;
	if (WaterReflectionComponent)
	{
		WaterReflectionComponent->SetVisibility(bEnable);
	}
}

void UMythSpriteAnimComponent::SetWaterReflectionActive(bool bActive)
{
	bEnableWaterReflection = bActive;
	if (WaterReflectionComponent)
	{
		WaterReflectionComponent->SetVisibility(bActive);
	}
}

