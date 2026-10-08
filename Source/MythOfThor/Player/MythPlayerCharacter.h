#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MythPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float CameraPitch = -50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float DefaultArmLength = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float MinArmLength = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float MaxArmLength = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float ZoomStep = 100.f;

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
