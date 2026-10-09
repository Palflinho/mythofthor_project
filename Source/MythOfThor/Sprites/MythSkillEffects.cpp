#include "Sprites/MythSkillEffects.h"

#include "Sprites/MythRoSound.h"
#include "Sprites/MythRoSpriteComponent.h"
#include "Sprites/MythStrEffect.h"

#include "Dom/JsonObject.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	struct FEffectPart
	{
		FString Type; // STR, SPR, WAV
		FString File;
		FString Wav;
		bool bAttached = true;
	};

	struct FSkillEntry
	{
		int32 Effect = 0;
		int32 Hit = 0;
		int32 Ground = 0;
		FString Element;
	};

	struct FTables
	{
		TMap<int32, TArray<FEffectPart>> Effects;
		TMap<int32, FSkillEntry> Skills;
	};

	const FTables& Tables()
	{
		static FTables T;
		static bool bLoaded = false;
		if (bLoaded)
		{
			return T;
		}
		bLoaded = true;
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("skill_effects.json"))))
		{
			UE_LOG(LogTemp, Warning, TEXT("MythSkillEffects: Config/skill_effects.json ausente"));
			return T;
		}
		TSharedPtr<FJsonObject> Root;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
		{
			return T;
		}
		if (const TSharedPtr<FJsonObject>* Effects; Root->TryGetObjectField(TEXT("effects"), Effects))
		{
			for (const auto& Pair : (*Effects)->Values)
			{
				TArray<FEffectPart>& Parts = T.Effects.Add(FCString::Atoi(*Pair.Key));
				for (const TSharedPtr<FJsonValue>& V : Pair.Value->AsArray())
				{
					const TSharedPtr<FJsonObject> O = V->AsObject();
					FEffectPart& P = Parts.AddDefaulted_GetRef();
					P.Type = O->GetStringField(TEXT("type"));
					P.File = O->GetStringField(TEXT("file"));
					P.Wav = O->GetStringField(TEXT("wav"));
					P.bAttached = O->GetBoolField(TEXT("attached"));
				}
			}
		}
		if (const TSharedPtr<FJsonObject>* Skills; Root->TryGetObjectField(TEXT("skills"), Skills))
		{
			for (const auto& Pair : (*Skills)->Values)
			{
				const TSharedPtr<FJsonObject> O = Pair.Value->AsObject();
				FSkillEntry& S = T.Skills.Add(FCString::Atoi(*Pair.Key));
				O->TryGetNumberField(TEXT("effect"), S.Effect);
				O->TryGetNumberField(TEXT("hit"), S.Hit);
				O->TryGetNumberField(TEXT("ground"), S.Ground);
				O->TryGetStringField(TEXT("element"), S.Element);
			}
		}
		UE_LOG(LogTemp, Log, TEXT("MythSkillEffects: %d efeitos, %d skills"), T.Effects.Num(), T.Skills.Num());
		return T;
	}

	FString EffectDir()
	{
		return FPaths::Combine(UMythRoSpriteComponent::GetAssetsRoot(), TEXT("data"), TEXT("texture"), TEXT("effect"));
	}

	/** "firehit%d" -> uma das variacoes que existem no disco (firehit1..3), como o cliente sorteia. */
	FString ResolveVariant(const FString& File)
	{
		if (!File.Contains(TEXT("%d")))
		{
			return File;
		}
		TArray<FString> Options;
		for (int32 i = 1; i <= 9; ++i)
		{
			const FString Name = File.Replace(TEXT("%d"), *FString::FromInt(i));
			if (IFileManager::Get().FileExists(*FPaths::Combine(EffectDir(), Name + TEXT(".str"))))
			{
				Options.Add(Name);
			}
		}
		return Options.Num() > 0 ? Options[FMath::RandHelper(Options.Num())] : File.Replace(TEXT("%d"), TEXT(""));
	}

	FVector FeetOf(const AActor* Target, const FVector& Fallback)
	{
		return Target ? AMythStrEffect::FeetOf(Target) : Fallback;
	}
}

FLinearColor FMythSkillEffects::ElementColor(int32 SkillID)
{
	const FSkillEntry* S = Tables().Skills.Find(SkillID);
	const FString Element = S ? S->Element : FString();
	if (Element == TEXT("Fire"))   { return FLinearColor(1.f, 0.42f, 0.12f); }
	if (Element == TEXT("Water"))  { return FLinearColor(0.3f, 0.65f, 1.f); }
	if (Element == TEXT("Wind"))   { return FLinearColor(0.85f, 0.9f, 1.f); }
	if (Element == TEXT("Earth"))  { return FLinearColor(0.85f, 0.6f, 0.3f); }
	if (Element == TEXT("Holy"))   { return FLinearColor(1.f, 0.92f, 0.65f); }
	if (Element == TEXT("Dark"))   { return FLinearColor(0.55f, 0.25f, 0.9f); }
	if (Element == TEXT("Ghost"))  { return FLinearColor(0.75f, 0.6f, 1.f); }
	if (Element == TEXT("Poison")) { return FLinearColor(0.45f, 0.95f, 0.35f); }
	if (Element == TEXT("Undead")) { return FLinearColor(0.5f, 0.35f, 0.6f); }
	return FLinearColor(1.f, 0.88f, 0.72f);
}

bool FMythSkillEffects::PlayEffectId(UObject* WorldContext, int32 EffectID, AActor* Target, const FVector& Location)
{
	const TArray<FEffectPart>* Parts = Tables().Effects.Find(EffectID);
	if (!Parts)
	{
		return false;
	}
	const FVector Feet = FeetOf(Target, Location);
	bool bAny = false;
	for (const FEffectPart& P : *Parts)
	{
		if (P.Type == TEXT("STR") && !P.File.IsEmpty())
		{
			const FString File = ResolveVariant(P.File);
			const bool bStr = AMythStrEffect::Play(WorldContext, File, Feet, P.bAttached ? Target : nullptr) != nullptr;
			UE_LOG(LogTemp, Verbose, TEXT("MythSkillEffects: efeito %d str '%s' %s"), EffectID, *File, bStr ? TEXT("ok") : TEXT("falhou"));
			bAny |= bStr;
		}
		if (!P.Wav.IsEmpty())
		{
			bAny |= UMythRoSound::PlayWavAtLocation(WorldContext, P.Wav + TEXT(".wav"), Feet);
		}
	}
	return bAny;
}

bool FMythSkillEffects::Play(UObject* WorldContext, int32 SkillID, EMythSkillEffectStage Stage, AActor* Target, const FVector& Location, bool bFlashIfMissing)
{
	const FSkillEntry* S = Tables().Skills.Find(SkillID);
	int32 EffectID = 0;
	if (S)
	{
		EffectID = Stage == EMythSkillEffectStage::Hit ? S->Hit : Stage == EMythSkillEffectStage::Ground ? S->Ground : S->Effect;
	}
	const bool bPlayed = EffectID > 0 && PlayEffectId(WorldContext, EffectID, Target, Location);
	UE_LOG(LogTemp, Verbose, TEXT("MythSkillEffects: skill %d estagio %d -> efeito %d (%s)"), SkillID, static_cast<int32>(Stage), EffectID, bPlayed ? TEXT("tocou") : TEXT("sem efeito"));
	// O .str ja traz a propria luz; sem ele, o clarao do elemento no impacto
	if (!bPlayed && bFlashIfMissing && Stage != EMythSkillEffectStage::Ground)
	{
		AMythLightFlash::Flash(WorldContext, FeetOf(Target, Location) + FVector(0.f, 0.f, 70.f), ElementColor(SkillID), 55.f, 0.55f, Target);
	}
	return bPlayed;
}
