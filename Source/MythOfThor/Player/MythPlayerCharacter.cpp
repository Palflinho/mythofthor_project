#include "Player/MythPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

AMythPlayerCharacter::AMythPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// A camera nunca deve girar junto com o controle: o yaw e controlado manualmente.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->InitCapsuleSize(35.f, 90.f);

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = false; // o sprite 2D usa URagnarokDirectionHelper, nao a rotacao do corpo
	Move->MaxWalkSpeed = 400.f;
	Move->bConstrainToPlane = true;
	Move->bSnapToPlaneAtStart = true;

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
	CameraYaw = FixedYaw;
	CameraBoom->TargetArmLength = DefaultArmLength;
	ApplyCamera();
}

void AMythPlayerCharacter::SetCameraMode(EMythCameraMode NewMode)
{
	CameraMode = NewMode;
	if (CameraMode == EMythCameraMode::FixedIsometric)
	{
		// Voltar ao modo fixo restaura o angulo classico e o zoom padrao.
		CameraYaw = FixedYaw;
		CameraBoom->TargetArmLength = DefaultArmLength;
	}
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
	CameraBoom->SetWorldRotation(FRotator(CameraPitch, CameraYaw, 0.f));
}
