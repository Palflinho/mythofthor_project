#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/RagnarokDirectionHelper.h"
#include "MythPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UMythSpriteAnimComponent;
class UPaperFlipbookComponent;
class UMythStatsComponent;
class UAutoBattleComponent;

/** Modos de câmera alternáveis em jogo (diretrizes, seção 4.B). */
UENUM(BlueprintType)
enum class EMythCameraMode : uint8
{
	FixedIsometric UMETA(DisplayName = "Isometrica Fixa 45"),
	Free360        UMETA(DisplayName = "Livre 360")
};

/**
 * Personagem jogável: capsula + SpringArm/Camera HD-2D.
 * O sprite 2D (PaperZD) sera anexado depois via Blueprint filho; este C++ cuida da camera.
 */
UCLASS()
class MYTHOFTHOR_API AMythPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMythPlayerCharacter();

	UFUNCTION(BlueprintCallable, Category = "Myth|Camera")
	void SetCameraMode(EMythCameraMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "Myth|Camera")
	void ToggleCameraMode();

	/** Gira a camera (graus). Ignorado no modo Isometrico Fixo. */
	UFUNCTION(BlueprintCallable, Category = "Myth|Camera")
	void RotateCamera(float DeltaYaw);

	/** Zoom (positivo aproxima). Ignorado no modo Isometrico Fixo. */
	UFUNCTION(BlueprintCallable, Category = "Myth|Camera")
	void ZoomCamera(float Delta);

	UFUNCTION(BlueprintPure, Category = "Myth|Camera")
	EMythCameraMode GetCameraMode() const { return CameraMode; }

	/** Yaw atual da camera; WASD e relativo a ele. */
	UFUNCTION(BlueprintPure, Category = "Myth|Camera")
	float GetCameraYaw() const { return CameraYaw; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float FixedYaw = 45.f;

	/** Pitch fixo isométrico clássico (GDD Técnico HD-2D: -30.0 a -35.0 graus). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float CameraPitch = -35.f;

	/** FOV de maquete / diorama (GDD Técnico HD-2D: 35.0 a 45.0 graus). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float FieldOfView = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float DefaultArmLength = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float MinArmLength = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float MaxArmLength = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float ZoomStep = 100.f;

	/** Sprite 2D do personagem (Corpo / Master Sprite Component). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprite")
	TObjectPtr<UMythSpriteAnimComponent> Sprite;

	/** Camadas de customizacao visual sincronizadas (GDD Tecnico HD-2D: Secao 3) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprite|Layers")
	TObjectPtr<UPaperFlipbookComponent> RobeLayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprite|Layers")
	TObjectPtr<UPaperFlipbookComponent> HairLayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprite|Layers")
	TObjectPtr<UPaperFlipbookComponent> HeadgearLowerLayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprite|Layers")
	TObjectPtr<UPaperFlipbookComponent> HeadgearMiddleLayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprite|Layers")
	TObjectPtr<UPaperFlipbookComponent> HeadgearTopLayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprite|Layers")
	TObjectPtr<UPaperFlipbookComponent> WeaponLayer;

	/** Ajusta dinamicamente a prioridade de transluscencia das camadas e inverte ao olhar para o Norte */
	UFUNCTION(BlueprintCallable, Category = "Sprite|Layers")
	void UpdateLayerSorting(ERagnarokDirection FacingDirection);

	/** Propaga o Hit-Flash emissivo de 1 frame simultaneamente em todas as 7 camadas (GDD Tecnico: Modulo 4.1) */
	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void TriggerMultiLayerHitFlash(float Duration = 0.05f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myth|Stats")
	TObjectPtr<UMythStatsComponent> Stats;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myth|AutoBattle")
	TObjectPtr<UAutoBattleComponent> AutoBattle;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Myth|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Myth|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	EMythCameraMode CameraMode = EMythCameraMode::FixedIsometric;
	float CameraYaw = 45.f;

	void ApplyCamera();
};
