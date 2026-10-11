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
	Idle, Walk, Attack, Hit, Dead, Sit, Pickup, Ready
};

/**
 * Renderiza o sprite 2D (Paper2D) e roda a maquina de estados:
 *   Idle <-> Walk (pela velocidade do dono), Attack/Hit (one-shot, voltam ao locomotion), Dead (final).
 * A direcao (8) vem de URagnarokDirectionHelper usando o yaw da camera do jogador;
 * o sprite fica sempre paralelo a tela (yaw e pitch da camera), como no cliente original.
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

	/** Quanto (cm) o sprite e puxado em direcao a camera para nao ser cortado por paredes/chao atras dele. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprite")
	float CameraDepthOffset = 20.f;

	/** AttackDuration > 0 acelera a animacao para caber no intervalo do ASPD (nunca a desacelera). */
	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void PlayAttack(float AttackDuration = 0.f);

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void PlayHit();

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void Die();

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void Revive();

	/** Vira o sprite para um ponto do mundo (ex.: alvo do ataque). */
	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void FaceLocation(const FVector& WorldLocation);

	UFUNCTION(BlueprintPure, Category = "Sprite")
	EMythSpriteState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Sprite")
	ERagnarokDirection GetDirection() const { return Direction; }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	EMythSpriteState State = EMythSpriteState::Idle;
	ERagnarokDirection Direction = ERagnarokDirection::South;
	bool bOneShotPlaying = false;
	/** Posicao relativa dos pes (pivo do sprite) definida pelo dono; o billboard parte dela a cada tick. */
	FVector FeetRelativeLocation = FVector::ZeroVector;

	void UpdateBillboard(const FRotator& CamRot);

	UFUNCTION()
	void HandleFinishedPlaying();

	void EnterOneShot(EMythSpriteState NewState);
	void ApplyFlipbook(bool bKeepPlaybackPosition);
	UPaperFlipbook* FindFlipbook(EMythSpriteState InState, ERagnarokDirection InDir) const;
	FRotator GetCameraRotation() const;
};
