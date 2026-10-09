#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MythPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UMythRoSpriteComponent;
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

	/** Inclina a camera (graus), limitada a [MinPitch, MaxPitch] (Shift + botao direito no RO). Ignorado no modo Fixo. */
	UFUNCTION(BlueprintCallable, Category = "Myth|Camera")
	void PitchCamera(float DeltaPitch);

	/** Volta ao angulo, inclinacao e zoom padrao (duplo clique direito). */
	UFUNCTION(BlueprintCallable, Category = "Myth|Camera")
	void ResetCamera();

	UFUNCTION(BlueprintPure, Category = "Myth|Camera")
	EMythCameraMode GetCameraMode() const { return CameraMode; }

	/** Yaw atual da camera; WASD e relativo a ele. */
	UFUNCTION(BlueprintPure, Category = "Myth|Camera")
	float GetCameraYaw() const { return CameraYaw; }

	/** Yaw padrao. -90 olha para -Y, que e o norte do RO nos mapas importados (norte no topo da tela, como no original). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float FixedYaw = -90.f;

	/** Inclinacao padrao (negativa = olhando para baixo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float CameraPitch = -50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float MinPitch = -70.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float MaxPitch = -30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float DefaultArmLength = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float MinArmLength = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float MaxArmLength = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float ZoomStep = 100.f;

	/** Sprite do personagem composto em camadas (corpo + cabeca) a partir dos .spr/.act do cliente original. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprite")
	TObjectPtr<UMythRoSpriteComponent> Sprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myth|Stats")
	TObjectPtr<UMythStatsComponent> Stats;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myth|AutoBattle")
	TObjectPtr<UAutoBattleComponent> AutoBattle;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Myth|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Myth|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	// O cliente original permite girar/inclinar/zoom sempre; o modo Fixo trava a camera no angulo padrao.
	EMythCameraMode CameraMode = EMythCameraMode::Free360;
	float CameraYaw = -90.f;
	float CurrentPitch = -50.f;
	bool bCameraLockedByMap = false;

	void ApplyCamera();
};
