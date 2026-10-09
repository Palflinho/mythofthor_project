#include "World/MythNightAudioComponent.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWaveProcedural.h"

namespace
{
	constexpr int32 AmbienceSampleRate = 32000;
}

UMythNightAudioComponent::UMythNightAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = true; // transicao suave do filtro noturno
}

void UMythNightAudioComponent::BeginPlay()
{
	Super::BeginPlay();

	StartAmbience();
	ApplyNightFactor();

	if (PendingBgm)
	{
		USoundBase* Sound = PendingBgm;
		PendingBgm = nullptr;
		PlayBGM(Sound, PendingBgmVolume);
	}
}

void UMythNightAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopBGM();
	if (AmbienceComponent)
	{
		AmbienceComponent->Stop();
	}
	Super::EndPlay(EndPlayReason);
}

void UMythNightAudioComponent::PlayBGM(USoundBase* Sound, float Volume)
{
	if (!Sound)
	{
		return;
	}
	if (!HasBegunPlay())
	{
		// Chamado antes do BeginPlay (o GameMode cria o ator de dia/noite e ja pede a musica): adia.
		PendingBgm = Sound;
		PendingBgmVolume = Volume;
		return;
	}
	StopBGM();

	// O passa-baixa noturno e o do proprio componente de audio: um submix criado em runtime nao e registrado
	// no dispositivo de audio e silenciava a musica. O asset original nunca e alterado.
	BgmBaseVolume = Volume;
	BgmComponent = UGameplayStatics::CreateSound2D(this, Sound, 1.f, 1.f, 0.f, nullptr, /*bPersistAcrossLevelTransition=*/true, /*bAutoDestroy=*/false);
	if (BgmComponent)
	{
		BgmComponent->SetLowPassFilterEnabled(true);
		BgmComponent->SetLowPassFilterFrequency(DayCutoffHz);
		BgmComponent->FadeIn(BgmFadeInSeconds, 1.f);
		UE_LOG(LogTemp, Log, TEXT("MythNightAudio: BGM %s tocando (fade-in %.1f s)"), *Sound->GetName(), BgmFadeInSeconds);
	}
	LastAppliedCutoff = -1.f;
	ApplyNightFactor();
}

void UMythNightAudioComponent::StopBGM()
{
	if (BgmComponent)
	{
		BgmComponent->Stop();
		BgmComponent = nullptr;
	}
}

void UMythNightAudioComponent::SetNightFactor(float InNightFactor)
{
	TargetNightFactor = FMath::Clamp(InNightFactor, 0.f, 1.f);
}

void UMythNightAudioComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!FMath::IsNearlyEqual(NightFactor, TargetNightFactor, 0.001f))
	{
		const float Step = DeltaTime / FMath::Max(NightTransitionSeconds, 0.01f);
		NightFactor = FMath::Clamp(TargetNightFactor, NightFactor - Step, NightFactor + Step);
		ApplyNightFactor();
	}
}

void UMythNightAudioComponent::ApplyNightFactor()
{
	// Interpolacao exponencial do corte: soa linear para o ouvido.
	const float Day = FMath::Max(DayCutoffHz, 20.f);
	const float Night = FMath::Max(NightCutoffHz, 20.f);
	const float Cutoff = Day * FMath::Pow(Night / Day, NightFactor);

	if (LastAppliedCutoff < 0.f || FMath::Abs(Cutoff - LastAppliedCutoff) > LastAppliedCutoff * 0.01f)
	{
		LastAppliedCutoff = Cutoff;
		if (BgmComponent)
		{
			BgmComponent->SetLowPassFilterFrequency(Cutoff);
		}
	}

	if (BgmComponent)
	{
		BgmComponent->SetVolumeMultiplier(BgmBaseVolume * FMath::Lerp(1.f, NightBgmVolume, NightFactor));
	}
	if (AmbienceComponent)
	{
		AmbienceComponent->SetVolumeMultiplier(AmbienceVolume * NightFactor);
	}
}

void UMythNightAudioComponent::StartAmbience()
{
	USoundBase* Source = NightAmbienceOverride;

	if (!Source)
	{
		ProceduralAmbience = NewObject<USoundWaveProcedural>(this, TEXT("MythNightAmbience"));
		ProceduralAmbience->SetSampleRate(AmbienceSampleRate);
		ProceduralAmbience->NumChannels = 1;
		ProceduralAmbience->Duration = INDEFINITELY_LOOPING_DURATION;
		ProceduralAmbience->SoundGroup = SOUNDGROUP_Default;
		ProceduralAmbience->bLooping = false;
		ProceduralAmbience->OnSoundWaveProceduralUnderflow.BindUObject(this, &UMythNightAudioComponent::GenerateAmbience);
		Source = ProceduralAmbience;
	}

	AmbienceComponent = UGameplayStatics::CreateSound2D(this, Source, 0.f, 1.f, 0.f, nullptr, true, false);
	if (AmbienceComponent)
	{
		AmbienceComponent->Play();
	}
}

void UMythNightAudioComponent::GenerateAmbience(USoundWaveProcedural* Wave, int32 SamplesRequired)
{
	const int32 Count = FMath::Max(SamplesRequired, 2048);
	TArray<int16> Out;
	Out.SetNumUninitialized(Count);

	constexpr double Dt = 1.0 / AmbienceSampleRate;
	constexpr double TwoPi = 6.283185307179586;

	for (int32 i = 0; i < Count; ++i)
	{
		const double T = Synth.Time;
		Synth.Time += Dt;

		// Ruido branco (xorshift) -> dois passa-baixa de um polo (~350 Hz) = vento grave; rajadas lentas.
		Synth.Rng ^= Synth.Rng << 13; Synth.Rng ^= Synth.Rng >> 17; Synth.Rng ^= Synth.Rng << 5;
		const float White = (static_cast<float>(Synth.Rng) / 4294967295.f) * 2.f - 1.f;
		Synth.Lp1 += 0.065f * (White - Synth.Lp1);
		Synth.Lp2 += 0.065f * (Synth.Lp1 - Synth.Lp2);
		const float Gust = 0.55f + 0.30f * static_cast<float>(FMath::Sin(T * TwoPi * 0.07)) + 0.15f * static_cast<float>(FMath::Sin(T * TwoPi * 0.31));
		const float Wind = Synth.Lp2 * 6.f * Gust;

		// Grilos: 3 vozes, portadora ~4.3 kHz pulsada, em rajadas curtas.
		float Crickets = 0.f;
		for (int32 v = 0; v < 3; ++v)
		{
			const double Carrier = 4300.0 + v * 230.0;
			const double PulseRate = 22.0 + v * 3.0;
			const double Gate = FMath::Frac(T * (0.45 + 0.07 * v) + v * 0.31) < 0.35 ? 1.0 : 0.0;
			const double Pulse = FMath::Max(0.0, FMath::Sin(T * TwoPi * PulseRate));
			Crickets += static_cast<float>(FMath::Sin(T * TwoPi * Carrier) * Pulse * Pulse * Gate) * 0.05f;
		}

		const float Mix = FMath::Clamp(Wind * 0.55f + Crickets, -1.f, 1.f);
		Out[i] = static_cast<int16>(Mix * 32000.f);
	}

	Wave->QueueAudio(reinterpret_cast<const uint8*>(Out.GetData()), Count * sizeof(int16));
}
