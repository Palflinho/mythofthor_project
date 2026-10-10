#include "Player/MythPlayerCharacter.h"
#include "World/MythCellMoverComponent.h"
#include "World/MythMapInfo.h"
#include "World/MythVisionLightComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Sprites/MythRoSpriteComponent.h"
#include "Gameplay/MythStatsComponent.h"
#include "Gameplay/AutoBattleComponent.h"

AMythPlayerCharacter::AMythPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true; // suavizacao da camera

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
	Mover = CreateDefaultSubobject<UMythCellMoverComponent>(TEXT("Mover"));
	Sprite->SetupAttachment(RootComponent);
	Sprite->SetRelativeLocation(FVector(0.f, 0.f, -90.f)); // pes do sprite no chao da capsula

	Stats = CreateDefaultSubobject<UMythStatsComponent>(TEXT("Stats"));
	AutoBattle = CreateDefaultSubobject<UAutoBattleComponent>(TEXT("AutoBattle"));

	// Raio de visao noturno (so a luz, so do proprio personagem): acima da cabeca para iluminar o chao em volta
	VisionLight = CreateDefaultSubobject<UMythVisionLightComponent>(TEXT("VisionLight"));
	VisionLight->SetupAttachment(RootComponent);
	VisionLight->SetRelativeLocation(FVector(0.f, 0.f, 160.f));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 6.3f; // foco segue o personagem a 10% por quadro (60 fps), como o cliente
	CameraBoom->TargetArmLength = DefaultArmLength;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void AMythPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	FollowCamera->SetFieldOfView(CameraFOV);
	// Mapa interno (indoorrswtable do cliente): mais perto, inclinacao limitada e giro travado a 45 graus
	if (const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld()); Info && Info->IsIndoor())
	{
		bCameraLockedByMap = true;
		FixedYaw = -90.f + 45.f;
		CameraPitch = -40.f;
		MinPitch = -55.f;
		MaxPitch = -35.f;
		DefaultArmLength = 3500.f;
		MaxArmLength = 4200.f;
	}
	DestArm = CurrentArm = DefaultArmLength;
	ResetCamera();
	CameraYaw = DestYaw;
	CurrentPitch = DestPitch;
	ApplyCamera();
}

void AMythPlayerCharacter::SetCameraMode(EMythCameraMode NewMode)
{
	CameraMode = NewMode;
	if (CameraMode == EMythCameraMode::FixedIsometric)
	{
		// Voltar ao modo fixo restaura o angulo classico e o zoom padrao.
		ResetCamera();
		DestArm = DefaultArmLength;
	}
}

void AMythPlayerCharacter::ResetCamera()
{
	// Duplo clique direito: so os angulos voltam ao padrao; o zoom fica (CView::ResetToDefaultOrientation)
	DestYaw = FixedYaw;
	DestPitch = FMath::Clamp(CameraPitch, MinPitch, MaxPitch);
}

void AMythPlayerCharacter::PitchCamera(float DeltaPitch)
{
	if (CameraMode != EMythCameraMode::Free360)
	{
		return;
	}
	DestPitch = FMath::Clamp(DestPitch + DeltaPitch, MinPitch, MaxPitch);
}

void AMythPlayerCharacter::ToggleCameraMode()
{
	if (bCameraLockedByMap)
	{
		return;
	}
	SetCameraMode(CameraMode == EMythCameraMode::FixedIsometric ? EMythCameraMode::Free360 : EMythCameraMode::FixedIsometric);
}

void AMythPlayerCharacter::RotateCamera(float DeltaYaw)
{
	if (CameraMode != EMythCameraMode::Free360 || bCameraLockedByMap)
	{
		return; // mapa interno: longitude travada
	}
	DestYaw = DestYaw + DeltaYaw;
}

void AMythPlayerCharacter::ZoomCamera(float Delta)
{
	if (CameraMode != EMythCameraMode::Free360)
	{
		return;
	}
	DestArm = FMath::Clamp(DestArm - Delta * ZoomStep, MinArmLength, MaxArmLength);
}

void AMythPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// cur += (dest - cur) * 0.1 por quadro do cliente (~60 por segundo), independente do FPS daqui
	const float Alpha = 1.f - FMath::Pow(0.9f, DeltaSeconds * 60.f);
	CameraYaw += FRotator::NormalizeAxis(DestYaw - CameraYaw) * Alpha; // menor arco
	CurrentPitch += (DestPitch - CurrentPitch) * Alpha;
	CurrentArm += (DestArm - CurrentArm) * Alpha;
	ApplyCamera();
}

void AMythPlayerCharacter::StartQuake(float Seconds, float AmplitudeUU)
{
	QuakeLeft = QuakeTotal = FMath::Max(Seconds, 0.03f);
	QuakeAmplitude = AmplitudeUU;
	UE_LOG(LogTemp, Verbose, TEXT("MythQuake: %.0f ms, %.0f uu"), Seconds * 1000.f, AmplitudeUU);
}

void AMythPlayerCharacter::ApplyCamera()
{
	// Tremor: deslocamento aleatorio no plano da tela (mais forte na vertical), que some no fim
	if (QuakeLeft > 0.f)
	{
		QuakeLeft -= GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;
		const float Fade = FMath::Clamp(QuakeLeft / QuakeTotal, 0.f, 1.f);
		CameraBoom->SocketOffset = FVector(0.f, FMath::FRandRange(-0.6f, 0.6f), FMath::FRandRange(-1.f, 1.f)) * QuakeAmplitude * Fade;
	}
	else if (!CameraBoom->SocketOffset.IsZero())
	{
		CameraBoom->SocketOffset = FVector::ZeroVector;
	}
	CameraBoom->TargetArmLength = CurrentArm;
	CameraBoom->SetWorldRotation(FRotator(CurrentPitch, CameraYaw, 0.f));
}
