#include "Sprites/MythSpriteAnimComponent.h"

#include "Sprites/MythSpriteSet.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "PaperFlipbook.h"

UMythSpriteAnimComponent::UMythSpriteAnimComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetLooping(true);
}

void UMythSpriteAnimComponent::BeginPlay()
{
	Super::BeginPlay();
	OnFinishedPlaying.AddDynamic(this, &UMythSpriteAnimComponent::HandleFinishedPlaying);
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
	// Trocar so a direcao nao deve reiniciar o ciclo de caminhada.
	SetPlaybackPosition(bKeepPlaybackPosition ? FMath::Min(Position, Flipbook->GetTotalDuration()) : 0.f, false);
	Play();
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

void UMythSpriteAnimComponent::PlayAttack() { EnterOneShot(EMythSpriteState::Attack); }
void UMythSpriteAnimComponent::PlayHit()    { EnterOneShot(EMythSpriteState::Hit); }
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
