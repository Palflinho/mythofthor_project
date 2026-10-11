#include "Sprites/MythSkillEffects.h"

#include "Sprites/MythRoSound.h"
#include "Sprites/MythRoSpriteComponent.h"
#include "Sprites/MythStrEffect.h"
#include "Sprites/MythProcEffect.h"
#include "Player/MythPlayerCharacter.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

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
		/** Parte inteira (campos dos efeitos procedurais: CYLINDER, 3D, 2D). */
		TSharedPtr<FJsonObject> Raw;
	};

	struct FSkillEntry
	{
		int32 Effect = 0;
		int32 Hit = 0;
		int32 Ground = 0;
		int32 Cast = 0;
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
		// skill_effects_full.json (com os efeitos procedurais, coord 009) quando existir; senao a tabela antiga
		if (!FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("skill_effects_full.json")))
			&& !FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("skill_effects.json"))))
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
					O->TryGetStringField(TEXT("type"), P.Type);
					O->TryGetStringField(TEXT("file"), P.File);
					O->TryGetStringField(TEXT("wav"), P.Wav);
					O->TryGetBoolField(TEXT("attached"), P.bAttached);
					P.Raw = O;
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
				O->TryGetNumberField(TEXT("cast"), S.Cast);
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

bool FMythSkillEffects::PlayEffectId(UObject* WorldContext, int32 EffectID, AActor* Target, const FVector& Location, AActor* Source)
{
	// Tremores que a tabela de efeitos nao traz (Config/effect_quakes.json, docs/ref/11): Magnum Break, Asura, Cart Termination
	static TMap<int32, TPair<float, float>> ExtraQuakes;
	static bool bQuakesLoaded = false;
	if (!bQuakesLoaded)
	{
		bQuakesLoaded = true;
		FString QuakeText;
		TSharedPtr<FJsonObject> QuakeRoot;
		if (FFileHelper::LoadFileToString(QuakeText, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("effect_quakes.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(QuakeText), QuakeRoot) && QuakeRoot.IsValid())
		{
			for (const auto& Pair : QuakeRoot->Values)
			{
				const TSharedPtr<FJsonObject>* Obj = nullptr;
				if (Pair.Value->TryGetObject(Obj))
				{
					ExtraQuakes.Add(FCString::Atoi(*Pair.Key), TPair<float, float>(static_cast<float>((*Obj)->GetNumberField(TEXT("ms"))) / 1000.f, static_cast<float>((*Obj)->GetNumberField(TEXT("amp")))));
				}
			}
		}
	}
	if (const TPair<float, float>* Quake = ExtraQuakes.Find(EffectID))
	{
		const APlayerController* QuakePC = WorldContext && WorldContext->GetWorld() ? WorldContext->GetWorld()->GetFirstPlayerController() : nullptr;
		const FVector At = FeetOf(Target, Location);
		if (AMythPlayerCharacter* QuakeChar = QuakePC ? Cast<AMythPlayerCharacter>(QuakePC->GetPawn()) : nullptr; QuakeChar && FVector::Dist2D(QuakeChar->GetActorLocation(), At) < 1500.f)
		{
			QuakeChar->StartQuake(Quake->Key, Quake->Value);
		}
	}
	const TArray<FEffectPart>* Parts = Tables().Effects.Find(EffectID);
	if (!Parts)
	{
		return false;
	}
	const FVector Feet = FeetOf(Target, Location);
	bool bAny = false;
	for (const FEffectPart& P : *Parts)
	{
		// Desenhados pelo executavel (docs/ref/11): cilindros, particulas/projeteis e raios
		if (P.Type == TEXT("CYLINDER") || P.Type == TEXT("3D") || P.Type == TEXT("2D"))
		{
			const bool bProc = AMythProcEffect::Spawn(WorldContext, P.Raw, Target, Feet, Source);
			UE_LOG(LogTemp, Verbose, TEXT("MythSkillEffects: efeito %d %s '%s' %s"), EffectID, *P.Type, *P.File, bProc ? TEXT("ok") : TEXT("falhou"));
			bAny |= bProc;
		}
		// FUNC quake: tremor de tela (Magnum Break 50 ms, Impacto Explosivo...); so se o efeito e perto do jogador
		if (P.Type == TEXT("FUNC") && P.Raw.IsValid() && P.Raw->GetStringField(TEXT("func")) == TEXT("quake"))
		{
			const APlayerController* PC = WorldContext ? WorldContext->GetWorld()->GetFirstPlayerController() : nullptr;
			if (AMythPlayerCharacter* Char = PC ? Cast<AMythPlayerCharacter>(PC->GetPawn()) : nullptr; Char && FVector::Dist2D(Char->GetActorLocation(), Feet) < 1500.f)
			{
				double Ms = 100.0;
				P.Raw->TryGetNumberField(TEXT("duration"), Ms);
				Char->StartQuake(static_cast<float>(Ms) / 1000.f);
				bAny = true;
			}
		}
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

float FMythSkillEffects::StrLength(const FString& StrFile)
{
	static TMap<FString, float> Cache;
	if (const float* Found = Cache.Find(StrFile))
	{
		return *Found;
	}
	FRoStr Str;
	const float Length = Str.Load(FPaths::Combine(EffectDir(), StrFile + TEXT(".str"))) && Str.Fps > 0 ? static_cast<float>(Str.MaxKey) / Str.Fps : 0.f;
	Cache.Add(StrFile, Length);
	return Length;
}

float FMythSkillEffects::EffectLength(int32 EffectID)
{
	const TArray<FEffectPart>* Parts = Tables().Effects.Find(EffectID);
	float Longest = 0.f;
	if (Parts)
	{
		for (const FEffectPart& P : *Parts)
		{
			if (P.Type == TEXT("STR"))
			{
				Longest = FMath::Max(Longest, StrLength(ResolveVariant(P.File)));
			}
			else if (P.Raw.IsValid() && (P.Type == TEXT("CYLINDER") || P.Type == TEXT("3D") || P.Type == TEXT("2D")))
			{
				double Ms = 1000.0, Between = 0.0, Count = 1.0;
				P.Raw->TryGetNumberField(TEXT("duration"), Ms);
				P.Raw->TryGetNumberField(TEXT("timeBetweenDupli"), Between);
				P.Raw->TryGetNumberField(TEXT("duplicate"), Count);
				Longest = FMath::Max(Longest, static_cast<float>(Ms + Between * FMath::Max(0.0, Count - 1.0)) / 1000.f);
			}
		}
	}
	return Longest;
}

int32 FMythSkillEffects::EffectIdFor(int32 SkillID, EMythSkillEffectStage Stage)
{
	const FSkillEntry* S = Tables().Skills.Find(SkillID);
	if (!S)
	{
		return 0;
	}
	switch (Stage)
	{
	case EMythSkillEffectStage::Hit: return S->Hit;
	case EMythSkillEffectStage::Ground: return S->Ground;
	case EMythSkillEffectStage::Cast: return S->Cast;
	default: return S->Effect;
	}
}

bool FMythSkillEffects::Play(UObject* WorldContext, int32 SkillID, EMythSkillEffectStage Stage, AActor* Target, const FVector& Location, bool bFlashIfMissing, AActor* Source)
{
	const int32 EffectID = EffectIdFor(SkillID, Stage);
	const bool bPlayed = EffectID > 0 && PlayEffectId(WorldContext, EffectID, Target, Location, Source);
	UE_LOG(LogTemp, Verbose, TEXT("MythSkillEffects: skill %d estagio %d -> efeito %d (%s)"), SkillID, static_cast<int32>(Stage), EffectID, bPlayed ? TEXT("tocou") : TEXT("sem efeito"));
	// O .str ja traz a propria luz; sem ele, o clarao do elemento no impacto
	if (!bPlayed && bFlashIfMissing && Stage != EMythSkillEffectStage::Ground)
	{
		AMythLightFlash::Flash(WorldContext, FeetOf(Target, Location) + FVector(0.f, 0.f, 70.f), ElementColor(SkillID), 55.f, 0.55f, Target);
	}
	return bPlayed;
}
