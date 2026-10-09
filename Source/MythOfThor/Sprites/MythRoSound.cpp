#include "Sprites/MythRoSound.h"

#include "Sprites/MythRoSpriteComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWaveProcedural.h"

namespace
{
	struct FDecodedWav
	{
		TArray<uint8> Pcm16; // PCM 16 bits little-endian
		int32 SampleRate = 22050;
		int32 Channels = 1;
		bool bValid = false;
	};

	/** Le um WAV PCM (8 ou 16 bits) e converte para 16 bits. */
	FDecodedWav DecodeWav(const TArray<uint8>& Bytes)
	{
		FDecodedWav Out;
		if (Bytes.Num() < 44 || FMemory::Memcmp(Bytes.GetData(), "RIFF", 4) != 0 || FMemory::Memcmp(Bytes.GetData() + 8, "WAVE", 4) != 0)
		{
			return Out;
		}
		int32 Bits = 16;
		int32 Format = 1;
		int32 Pos = 12;
		while (Pos + 8 <= Bytes.Num())
		{
			const uint8* Chunk = Bytes.GetData() + Pos;
			const int32 Size = Chunk[4] | (Chunk[5] << 8) | (Chunk[6] << 16) | (Chunk[7] << 24);
			const int32 Body = Pos + 8;
			if (Size < 0 || Body + Size > Bytes.Num())
			{
				break;
			}
			if (FMemory::Memcmp(Chunk, "fmt ", 4) == 0 && Size >= 16)
			{
				const uint8* F = Bytes.GetData() + Body;
				Format = F[0] | (F[1] << 8);
				Out.Channels = F[2] | (F[3] << 8);
				Out.SampleRate = F[4] | (F[5] << 8) | (F[6] << 16) | (F[7] << 24);
				Bits = F[14] | (F[15] << 8);
			}
			else if (FMemory::Memcmp(Chunk, "data", 4) == 0)
			{
				if (Format != 1 || (Bits != 8 && Bits != 16) || Out.Channels < 1 || Out.Channels > 2)
				{
					return Out;
				}
				if (Bits == 16)
				{
					Out.Pcm16.Append(Bytes.GetData() + Body, Size & ~1);
				}
				else
				{
					Out.Pcm16.SetNumUninitialized(Size * 2);
					int16* Dst = reinterpret_cast<int16*>(Out.Pcm16.GetData());
					for (int32 i = 0; i < Size; ++i)
					{
						Dst[i] = static_cast<int16>((static_cast<int32>(Bytes[Body + i]) - 128) << 8);
					}
				}
				Out.bValid = Out.Pcm16.Num() > 0;
				return Out;
			}
			Pos = Body + Size + (Size & 1);
		}
		return Out;
	}

	TMap<FString, FDecodedWav>& RoWavCache()
	{
		static TMap<FString, FDecodedWav> Map;
		return Map;
	}
}

USoundAttenuation* UMythRoSound::GetAttenuation()
{
	static TWeakObjectPtr<USoundAttenuation> Attenuation;
	if (!Attenuation.IsValid())
	{
		USoundAttenuation* A = NewObject<USoundAttenuation>(GetTransientPackage(), TEXT("MythRoSoundAttenuation"));
		A->AddToRoot();
		A->Attenuation.bAttenuate = true;
		A->Attenuation.bSpatialize = true;
		A->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
		A->Attenuation.AttenuationShapeExtents = FVector(900.f, 0.f, 0.f); // volume cheio ate ~9 m
		A->Attenuation.FalloffDistance = 2200.f;
		Attenuation = A;
	}
	return Attenuation.Get();
}

bool UMythRoSound::PlayWavAtLocation(const UObject* WorldContext, const FString& FileName, const FVector& Location, float Volume)
{
	if (!WorldContext || FileName.IsEmpty())
	{
		return false;
	}
	const FString Key = FileName.ToLower();
	FDecodedWav* Wav = RoWavCache().Find(Key);
	if (!Wav)
	{
		TArray<uint8> Bytes;
		const FString Path = FPaths::Combine(UMythRoSpriteComponent::GetAssetsRoot(), TEXT("data"), TEXT("wav"), FileName);
		FDecodedWav Decoded;
		if (FFileHelper::LoadFileToArray(Bytes, *Path))
		{
			Decoded = DecodeWav(Bytes);
		}
		if (!Decoded.bValid)
		{
			UE_LOG(LogTemp, Verbose, TEXT("MythRoSound: som invalido ou ausente: %s"), *Path);
		}
		Wav = &RoWavCache().Add(Key, MoveTemp(Decoded));
	}
	if (!Wav->bValid)
	{
		return false;
	}

	// Uma onda procedural por reproducao (sons curtos): copia o PCM do cache e toca uma vez.
	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(GetTransientPackage());
	Wave->SetSampleRate(Wav->SampleRate);
	Wave->NumChannels = Wav->Channels;
	Wave->Duration = static_cast<float>(Wav->Pcm16.Num()) / (2.f * Wav->Channels * Wav->SampleRate);
	Wave->SoundGroup = SOUNDGROUP_Effects;
	Wave->bLooping = false;
	Wave->QueueAudio(Wav->Pcm16.GetData(), Wav->Pcm16.Num());
	UGameplayStatics::PlaySoundAtLocation(WorldContext, Wave, Location, Volume, 1.f, 0.f, GetAttenuation());
	return true;
}
