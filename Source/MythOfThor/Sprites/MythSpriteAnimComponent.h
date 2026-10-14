#pragma once

#include "CoreMinimal.h"
#include "PaperFlipbookComponent.h"
#include "Core/RagnarokDirectionHelper.h"
#include "MythSpriteAnimComponent.generated.h"

class UMythSpriteSet;

/** Estados da maquina de animacao dos sprites 2D. */
UENUM(BlueprintType)
enum class EMythSpriteState : uint8
{
	Idle, Walk, Attack, Hit, Dead
};

/**
 * Renderiza o sprite 2D (Paper2D) e roda a maquina de estados:
 *   Idle <-> Walk (pela velocidade do dono), Attack/Hit (one-shot, voltam ao locomotion), Dead (final).
 * A direcao (8) vem de URagnarokDirectionHelper usando o yaw da camera do jogador;
 * o sprite e sempre virado para a camera (billboard so em yaw).
 *
 * Esta maquina e independente do PaperZD (plugin de marketplace, nao incluido no projeto):
 * um AnimInstance do PaperZD pode ler GetState()/GetDirection() sem mudar nada aqui.
 */
UCLASS(ClassGroup = (Myth), meta = (BlueprintSpawnableComponent))
class MYTHOFTHOR_API UMythSpriteAnimComponent : public UPaperFlipbookComponent
{
	GENERATED_BODY()

public:
	UMythSpriteAnimComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprite")
	TObjectPtr<UMythSpriteSet> SpriteSet;

	/** Velocidade (cm/s) acima da qual o estado vira Walk. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprite")
	float WalkSpeedThreshold = 10.f;

	/** AttackDuration > 0 acelera a animacao para caber no intervalo do ASPD (nunca a desacelera). */
	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void PlayAttack(float AttackDuration = 0.f);

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void PlayHit();

	/** Hitstop (Micro-congelamento): pausa o CustomTimeDilation por Duration segundos (GDD Tecnico HD-2D) */
	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void TriggerHitstop(float Duration = 0.03f);

	/** Hit-flash: pulso de emissao de 1 frame no sprite atingido */
	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void TriggerHitFlash(float Duration = 0.05f);

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void Die();

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void Revive();

	/** Vira o sprite para um ponto do mundo (ex.: alvo do ataque). */
	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void FaceLocation(const FVector& WorldLocation);

	/** Configura o Ghost Sprite Espelhado na agua (GDD Tecnico: Sistema de Agua e Reflexos HD-2D) */
	UFUNCTION(BlueprintCallable, Category = "Sprite|WaterReflection")
	void SetupWaterReflection(bool bEnable, float InWaterSurfaceZ = 0.0f, float InOpacity = 0.40f);

	UFUNCTION(BlueprintCallable, Category = "Sprite|WaterReflection")
	void SetWaterReflectionActive(bool bActive);

	UFUNCTION(BlueprintCallable, Category = "Sprite|WaterReflection")
	void SetWaterSurfaceZ(float InZ) { WaterSurfaceZ = InZ; }

	UFUNCTION(BlueprintPure, Category = "Sprite|WaterReflection")
	UPaperFlipbookComponent* GetWaterReflectionComponent() const { return WaterReflectionComponent; }

	UFUNCTION(BlueprintPure, Category = "Sprite")
	EMythSpriteState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Sprite")
	ERagnarokDirection GetDirection() const { return Direction; }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprite|WaterReflection")
	TObjectPtr<UPaperFlipbookComponent> WaterReflectionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprite|WaterReflection")
	bool bEnableWaterReflection = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprite|WaterReflection")
	float WaterSurfaceZ = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprite|WaterReflection")
	float ReflectionOpacity = 0.40f;

private:
	EMythSpriteState State = EMythSpriteState::Idle;
	ERagnarokDirection Direction = ERagnarokDirection::South;
	bool bOneShotPlaying = false;

	UFUNCTION()
	void HandleFinishedPlaying();

	void EnterOneShot(EMythSpriteState NewState);
	void ApplyFlipbook(bool bKeepPlaybackPosition);
	UPaperFlipbook* FindFlipbook(EMythSpriteState InState, ERagnarokDirection InDir) const;
	FRotator GetCameraRotation() const;

	FTimerHandle HitstopTimerHandle;
	FTimerHandle HitFlashTimerHandle;
	float SavedCustomTimeDilation = 1.0f;
	bool bIsHitstopped = false;

	void EndHitstop();
	void EndHitFlash();
};
