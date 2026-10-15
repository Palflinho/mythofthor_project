#include "Player/MythPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "PaperFlipbookComponent.h"
#include "Sprites/MythSpriteAnimComponent.h"
#include "Gameplay/MythStatsComponent.h"
#include "Gameplay/AutoBattleComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

AMythPlayerCharacter::AMythPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

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

	// Master Sprite Component (Corpo do personagem)
	Sprite = CreateDefaultSubobject<UMythSpriteAnimComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(RootComponent);
	Sprite->SetRelativeLocation(FVector(0.f, 0.f, -90.f)); // pes do sprite no chao da capsula
	Sprite->SetRenderCustomDepth(true);
	Sprite->SetCustomDepthStencilValue(1); // 1 = Jogador (GDD Tecnico HD-2D: Oclusao X-Ray)

	// Camadas aninhadas de customizacao visual (GDD Tecnico: Secao 3 - Layered Compositing)
	auto InitLayer = [this](const TCHAR* Name) -> UPaperFlipbookComponent*
	{
		UPaperFlipbookComponent* LayerComp = CreateDefaultSubobject<UPaperFlipbookComponent>(Name);
		LayerComp->SetupAttachment(Sprite);
		LayerComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LayerComp->SetCastShadow(true);
		LayerComp->SetRenderCustomDepth(true);
		LayerComp->SetCustomDepthStencilValue(1);
		LayerComp->SetLooping(true);
		return LayerComp;
	};

	RobeLayer = InitLayer(TEXT("RobeLayer"));
	HairLayer = InitLayer(TEXT("HairLayer"));
	HeadgearLowerLayer = InitLayer(TEXT("HeadgearLowerLayer"));
	HeadgearMiddleLayer = InitLayer(TEXT("HeadgearMiddleLayer"));
	HeadgearTopLayer = InitLayer(TEXT("HeadgearTopLayer"));
	WeaponLayer = InitLayer(TEXT("WeaponLayer"));

	UpdateLayerSorting(ERagnarokDirection::South);

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
	FollowCamera->SetFieldOfView(FieldOfView);
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
	if (FollowCamera)
	{
		FollowCamera->SetFieldOfView(FieldOfView);
	}
}

void AMythPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Sprite)
	{
		UpdateLayerSorting(Sprite->GetDirection());

		// Replica a linha do tempo e cadencia de frames para os subcomponentes aninhados
		const float PlaybackPos = Sprite->GetPlaybackPosition();
		auto SyncLayer = [PlaybackPos](UPaperFlipbookComponent* LayerComp)
		{
			if (LayerComp && LayerComp->GetFlipbook())
			{
				LayerComp->SetPlaybackPosition(PlaybackPos, false);
			}
		};

		SyncLayer(RobeLayer);
		SyncLayer(HairLayer);
		SyncLayer(HeadgearLowerLayer);
		SyncLayer(HeadgearMiddleLayer);
		SyncLayer(HeadgearTopLayer);
		SyncLayer(WeaponLayer);
	}
}

void AMythPlayerCharacter::UpdateLayerSorting(ERagnarokDirection FacingDirection)
{
	const bool bFacingNorth = (FacingDirection == ERagnarokDirection::North ||
	                           FacingDirection == ERagnarokDirection::NorthEast ||
	                           FacingDirection == ERagnarokDirection::NorthWest);

	// No espaco 3D, planos na mesma coordenada de profundidade sofrem oscilacao (Z-fighting).
	// A solucao adota Translucent Sort Priority incremental por camada.
	// Quando voltado para o Norte (costas para camera), a ordem e invertida programaticamente:
	// Capas e cabelos cobrem os elmos e peitorais conforme o comportamento visual canonico do RO.
	if (bFacingNorth)
	{
		if (WeaponLayer)         WeaponLayer->SetTranslucentSortPriority(10);
		if (Sprite)              Sprite->SetTranslucentSortPriority(20);
		if (HeadgearLowerLayer)  HeadgearLowerLayer->SetTranslucentSortPriority(30);
		if (HeadgearMiddleLayer) HeadgearMiddleLayer->SetTranslucentSortPriority(40);
		if (HeadgearTopLayer)    HeadgearTopLayer->SetTranslucentSortPriority(50);
		if (HairLayer)           HairLayer->SetTranslucentSortPriority(60);
		if (RobeLayer)           RobeLayer->SetTranslucentSortPriority(70);
	}
	else
	{
		if (RobeLayer)           RobeLayer->SetTranslucentSortPriority(10);
		if (Sprite)              Sprite->SetTranslucentSortPriority(20);
		if (HairLayer)           HairLayer->SetTranslucentSortPriority(30);
		if (HeadgearLowerLayer)  HeadgearLowerLayer->SetTranslucentSortPriority(40);
		if (HeadgearMiddleLayer) HeadgearMiddleLayer->SetTranslucentSortPriority(50);
		if (HeadgearTopLayer)    HeadgearTopLayer->SetTranslucentSortPriority(60);
		if (WeaponLayer)         WeaponLayer->SetTranslucentSortPriority(70);
	}
}

void AMythPlayerCharacter::TriggerMultiLayerHitFlash(float Duration)
{
	if (Sprite)
	{
		Sprite->TriggerHitFlash(Duration);
	}

	auto FlashLayer = [this, Duration](UPaperFlipbookComponent* LayerComp)
	{
		if (LayerComp && LayerComp->GetFlipbook())
		{
			UMaterialInterface* ExistingMat = LayerComp->GetMaterial(0);
			UMaterialInstanceDynamic* DynMat = ExistingMat ? Cast<UMaterialInstanceDynamic>(ExistingMat) : LayerComp->CreateDynamicMaterialInstance(0);
			if (DynMat)
			{
				DynMat->SetScalarParameterValue(TEXT("HitFlash"), 1.0f);
				if (UWorld* World = GetWorld())
				{
					FTimerHandle Handle;
					World->GetTimerManager().SetTimer(Handle, [LayerComp]()
					{
						if (LayerComp)
						{
							if (UMaterialInstanceDynamic* Mat = Cast<UMaterialInstanceDynamic>(LayerComp->GetMaterial(0)))
							{
								Mat->SetScalarParameterValue(TEXT("HitFlash"), 0.0f);
							}
						}
					}, Duration, false);
				}
			}
		}
	};

	FlashLayer(RobeLayer);
	FlashLayer(HairLayer);
	FlashLayer(HeadgearLowerLayer);
	FlashLayer(HeadgearMiddleLayer);
	FlashLayer(HeadgearTopLayer);
	FlashLayer(WeaponLayer);
}


