#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MythNightAudioComponent.generated.h"

class USoundBase;
class USoundSubmix;
class USoundSubmixBase;
class USoundWaveProcedural;
class UAudioComponent;
class USubmixEffectFilterPreset;

/**
 * Audio dinamico dia/noite (diretrizes, secao 6), sem alterar os arquivos originais:
 *  - cria em runtime um Submix "BGM" com um filtro passa-baixa (SubmixEffectFilter) e roteia a BGM por ele;
 *    a noite o corte desce (padrao 20 kHz -> 2.2 kHz) e o volume cai ~15%;
 *  - mistura uma ambiencia noturna (vento + grilos), sintetizada proceduralmente se nao houver asset.
 * O ator AMythDayNightActor chama SetNightFactor() todo frame (0 = dia, 1 = noite).
 */
UCLASS(ClassGroup = (Myth), meta = (BlueprintSpawnableComponent))
class MYTHOFTHOR_API UMythNightAudioComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMythNightAudioComponent();

	/** Toca a BGM em loop, roteada pelo submix com filtro. A pre-existente (se houver) e substituida. */
	UFUNCTION(BlueprintCallable, Category = "Myth|Audio")
	void PlayBGM(USoundBase* Sound, float Volume = 1.f);

	UFUNCTION(BlueprintCallable, Category = "Myth|Audio")
	void StopBGM();

	UFUNCTION(BlueprintCallable, Category = "Myth|Audio")
	void SetNightFactor(float InNightFactor);

	/** Corte do passa-baixa de dia (Hz); 20000 = sem efeito audivel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Audio")
	float DayCutoffHz = 20000.f;

	/** Corte do passa-baixa a noite (Hz). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Audio")
	float NightCutoffHz = 2200.f;

	/** Multiplicador de volume da BGM a noite (0.85 = -15%). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Audio")
	float NightBgmVolume = 0.85f;

	/** Volume da ambiencia noturna na plena noite. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Audio")
	float AmbienceVolume = 0.35f;

	/** Se definido, substitui o vento/grilos procedurais (use um loop). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Audio")
	TObjectPtr<USoundBase> NightAmbienceOverride;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY() TObjectPtr<USoundSubmix> BgmSubmix;
	UPROPERTY() TObjectPtr<USubmixEffectFilterPreset> FilterPreset;
	UPROPERTY() TObjectPtr<UAudioComponent> BgmComponent;
	UPROPERTY() TObjectPtr<UAudioComponent> AmbienceComponent;
	UPROPERTY() TObjectPtr<USoundWaveProcedural> ProceduralAmbience;

	// Som cujo SoundSubmixObject trocamos em memoria; restaurado no EndPlay (o asset em disco nunca muda).
	UPROPERTY() TObjectPtr<USoundBase> RoutedSound;
	UPROPERTY() TObjectPtr<USoundSubmixBase> RoutedSoundOriginalSubmix;

	float NightFactor = 0.f;
	float LastAppliedCutoff = -1.f;
	float BgmBaseVolume = 1.f;

	void ApplyNightFactor();
	void StartAmbience();
	void GenerateAmbience(USoundWaveProcedural* Wave, int32 SamplesRequired);

	// Estado do sintetizador (vento + grilos). Nao e UPROPERTY: o callback roda fora da game thread.
	struct FAmbienceState
	{
		double Time = 0.0;
		float Lp1 = 0.f, Lp2 = 0.f;
		uint32 Rng = 0x9E3779B9u;
	};
	FAmbienceState Synth;
};
