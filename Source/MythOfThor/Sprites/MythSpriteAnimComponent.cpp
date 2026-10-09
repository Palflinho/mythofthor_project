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
	FeetRelativeLocation = GetRelativeLocation();
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

void UMythSpriteAnimComponent::UpdateBillboard(const FRotator& CamRot)
{
	// Como no cliente original: o sprite fica paralelo a tela (acompanha yaw E pitch da camera),
	// entao inclinar a camera nao o achata. O pivo do sprite e embaixo-centro, logo os pes nao saem do lugar.
	const FRotationMatrix CamMatrix(CamRot);
	const FVector CamRight = CamMatrix.GetUnitAxis(EAxis::Y);
	const FVector CamUp = CamMatrix.GetUnitAxis(EAxis::Z);
	const FVector CamForward = CamMatrix.GetUnitAxis(EAxis::X);
	// O X local do sprite aponta para -Right (mesma convencao do billboard antigo, Yaw - 90) e o Z local para o Up da camera.
	SetWorldRotation(FRotationMatrix::MakeFromXZ(-CamRight, CamUp).Rotator());

	// Avanca o sprite um pouco em direcao a camera para nao atravessar paredes e o chao atras dele.
	if (const USceneComponent* Parent = GetAttachParent())
	{
		const FVector Feet = Parent->GetComponentTransform().TransformPosition(FeetRelativeLocation);
		SetWorldLocation(Feet - CamForward * CameraDepthOffset);
	}
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
	UpdateBillboard(CamRot);

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
