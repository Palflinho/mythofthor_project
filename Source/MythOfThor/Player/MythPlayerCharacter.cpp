#include "Player/MythPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Sprites/MythRoSpriteComponent.h"
#include "Gameplay/MythStatsComponent.h"
#include "Gameplay/AutoBattleComponent.h"

AMythPlayerCharacter::AMythPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// A camera nunca deve girar junto com o controle: o yaw e controlado manualmente.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Raio pequeno: no RO o personagem e um ponto numa celula de 50 cm; uma capsula larga nao passa nas faixas de 1 celula do .gat.
	GetCapsuleComponent()->InitCapsuleSize(20.f, 90.f);

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = false; // o sprite 2D usa URagnarokDirectionHelper, nao a rotacao do corpo
	Move->MaxWalkSpeed = 400.f;
	// Bases e degraus de pedra dos predios do RO passam de 45 cm; no original quem decide o caminhavel e o .gat.
	Move->MaxStepHeight = 60.f;
	// Sem trava de plano (herdada do template TopDown): o personagem precisa subir calcadas e degraus.
	Move->bConstrainToPlane = false;
	Move->bSnapToPlaneAtStart = false;

	Sprite = CreateDefaultSubobject<UMythRoSpriteComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(RootComponent);
	Sprite->SetRelativeLocation(FVector(0.f, 0.f, -90.f)); // pes do sprite no chao da capsula

	Stats = CreateDefaultSubobject<UMythStatsComponent>(TEXT("Stats"));
	AutoBattle = CreateDefaultSubobject<UAutoBattleComponent>(TEXT("AutoBattle"));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.f;
	CameraBoom->TargetArmLength = DefaultArmLength;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void AMythPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	ResetCamera();
}

void AMythPlayerCharacter::SetCameraMode(EMythCameraMode NewMode)
{
	CameraMode = NewMode;
	if (CameraMode == EMythCameraMode::FixedIsometric)
	{
		// Voltar ao modo fixo restaura o angulo classico e o zoom padrao.
		ResetCamera();
		return;
	}
	ApplyCamera();
}

void AMythPlayerCharacter::ResetCamera()
{
	CameraYaw = FixedYaw;
	CurrentPitch = FMath::Clamp(CameraPitch, MinPitch, MaxPitch);
	CameraBoom->TargetArmLength = DefaultArmLength;
	ApplyCamera();
}

void AMythPlayerCharacter::PitchCamera(float DeltaPitch)
{
	if (CameraMode != EMythCameraMode::Free360)
	{
		return;
	}
	CurrentPitch = FMath::Clamp(CurrentPitch + DeltaPitch, MinPitch, MaxPitch);
	ApplyCamera();
}

void AMythPlayerCharacter::ToggleCameraMode()
{
	SetCameraMode(CameraMode == EMythCameraMode::FixedIsometric ? EMythCameraMode::Free360 : EMythCameraMode::FixedIsometric);
}

void AMythPlayerCharacter::RotateCamera(float DeltaYaw)
{
	if (CameraMode != EMythCameraMode::Free360)
	{
		return;
	}
	CameraYaw = FRotator::NormalizeAxis(CameraYaw + DeltaYaw);
	ApplyCamera();
}

void AMythPlayerCharacter::ZoomCamera(float Delta)
{
	if (CameraMode != EMythCameraMode::Free360)
	{
		return;
	}
	CameraBoom->TargetArmLength = FMath::Clamp(CameraBoom->TargetArmLength - Delta * ZoomStep, MinArmLength, MaxArmLength);
}

void AMythPlayerCharacter::ApplyCamera()
{
	CameraBoom->SetWorldRotation(FRotator(CurrentPitch, CameraYaw, 0.f));
}
