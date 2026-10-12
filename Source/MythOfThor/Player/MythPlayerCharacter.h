#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MythPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UMythRoSpriteComponent;
class UMythCellMoverComponent;
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

	// Camera do cliente original (docs/ref/02): distancias em unidades do RO (1 unidade = 10 uu; celula = 5), lente
	// estreita (FOV vertical ~15 graus, quase isometrica) e tudo suavizado em 10% por quadro

	/** Yaw padrao. -90 olha para -Y, que e o norte do RO nos mapas importados (longitude 0 do cliente). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float FixedYaw = -90.f;

	/** Inclinacao padrao (negativa = olhando para baixo): -45 fora, -40 em mapas internos. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float CameraPitch = -45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float MinPitch = -65.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float MaxPitch = -45.f;

	/** Distancia da camera (uu): padrao 500, de 150 a 700 unidades do RO fora; 350 / 150..420 dentro. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float DefaultArmLength = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float MinArmLength = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float MaxArmLength = 5000.f;

	/** Por entalhe da roda: 60 unidades do RO. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float ZoomStep = 600.f;

	/** Campo de visao horizontal (graus) para 16:9 com ~15 graus na vertical, como o cliente. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float CameraFOV = 26.4f;

	virtual void Tick(float DeltaSeconds) override;

	/** Tremor de tela (FUNC quake da EffectTable, docs/ref/11): sacode a camera por Seconds com a amplitude em uu. */
	void StartQuake(float Seconds, float AmplitudeUU = 15.f);

	/** Sprite do personagem composto em camadas (corpo + cabeca) a partir dos .spr/.act do cliente original. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprite")
	TObjectPtr<UMythRoSpriteComponent> Sprite;

	/** Online: anda pelas celulas confirmadas pelo servidor (sem fisica, como o cliente original). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myth")
	TObjectPtr<UMythCellMoverComponent> Mover;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myth|Stats")
	TObjectPtr<UMythStatsComponent> Stats;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myth|AutoBattle")
	TObjectPtr<UAutoBattleComponent> AutoBattle;

	/** Luz suave em volta do personagem a noite nos mapas externos (raio pela classe e DEX do servidor). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myth|Vision")
	TObjectPtr<class UMythVisionLightComponent> VisionLight;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Myth|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Myth|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	// O cliente original permite girar/inclinar/zoom sempre; o modo Fixo trava a camera no angulo padrao.
	EMythCameraMode CameraMode = EMythCameraMode::Free360;
	// Valores atuais (suavizados) e de destino, como cur/dest do CView do cliente
	float CameraYaw = -90.f;
	float CurrentPitch = -45.f;
	float CurrentArm = 5000.f;
	/** Distancia da camera depois do clamp do terreno (docs/ref/02 3.3): encurta o braco se a camera entraria no chao. */
	float ClampedArm = 5000.f;
	float DestYaw = -90.f;
	float DestPitch = -45.f;
	float DestArm = 5000.f;
	bool bCameraLockedByMap = false;
	float QuakeLeft = 0.f;
	float QuakeTotal = 0.f;
	float QuakeAmplitude = 0.f;

	void ApplyCamera();
};
