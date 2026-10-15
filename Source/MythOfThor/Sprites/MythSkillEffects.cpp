#include "Sprites/MythSkillEffects.h"

#include "Sprites/MythRoSound.h"
#include "UI/MythOptions.h"
#include "Sprites/MythRoSpriteComponent.h"
#include "Sprites/MythStrEffect.h"
#include "Components/CapsuleComponent.h"
#include "Sprites/MythSprEffect.h"
#include "Sprites/MythProcEffect.h"
#include "Sprites/MythTrailEffect.h"
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
		/** Ids por estagio (EMythSkillEffectStage); algumas skills tem mais de um efeito no mesmo momento. */
		TArray<int32> Stages[static_cast<int32>(EMythSkillEffectStage::Count)];
		FString Element;
		bool bHideCastAura = false;
	};

	/** Nome do campo no skill_effects_full.json (tools/gen_skill_effects_ro.py) para cada estagio. */
	const TCHAR* StageField(int32 Stage)
	{
		static const TCHAR* Names[] = { TEXT("effect"), TEXT("hit"), TEXT("ground"), TEXT("cast"), TEXT("success"), TEXT("successCaster"),
			TEXT("release"), TEXT("beforeHit"), TEXT("beforeHitCaster"), TEXT("hitCaster"), TEXT("beginCast") };
		static_assert(UE_ARRAY_COUNT(Names) == static_cast<int32>(EMythSkillEffectStage::Count), "estagios");
		return Names[Stage];
	}

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
				for (int32 Stage = 0; Stage < static_cast<int32>(EMythSkillEffectStage::Count); ++Stage)
				{
					const TSharedPtr<FJsonValue> V = O->TryGetField(StageField(Stage));
					if (!V.IsValid())
					{
						continue;
					}
					if (V->Type == EJson::Array)
					{
						for (const TSharedPtr<FJsonValue>& Id : V->AsArray()) { S.Stages[Stage].Add(static_cast<int32>(Id->AsNumber())); }
					}
					else if (V->Type == EJson::Number)
					{
						S.Stages[Stage].Add(static_cast<int32>(V->AsNumber()));
					}
				}
				O->TryGetStringField(TEXT("element"), S.Element);
				O->TryGetBoolField(TEXT("hideCastAura"), S.bHideCastAura);
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

	struct FSkillElementInfo
	{
		FString Name;
		FString Element;
		FLinearColor Color;
	};

	const TMap<int32, FSkillElementInfo>& SkillElementTable()
	{
		static TMap<int32, FSkillElementInfo> Table;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			bLoaded = true;
			FString Text;
			if (FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("skill_elements.json"))))
			{
				TSharedPtr<FJsonObject> Root;
				if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
				{
					for (const auto& Pair : Root->Values)
					{
						const TSharedPtr<FJsonObject>* Obj = nullptr;
						if (Pair.Value->TryGetObject(Obj) && Obj && (*Obj).IsValid())
						{
							const int32 Id = (*Obj)->GetIntegerField(TEXT("id"));
							FSkillElementInfo& Info = Table.Add(Id);
							Info.Name = (*Obj)->GetStringField(TEXT("name"));
							Info.Element = (*Obj)->GetStringField(TEXT("element"));
							const TArray<TSharedPtr<FJsonValue>>* ColorArr = nullptr;
							if ((*Obj)->TryGetArrayField(TEXT("color"), ColorArr) && ColorArr && ColorArr->Num() >= 3)
							{
								Info.Color = FLinearColor(
									static_cast<float>((*ColorArr)[0]->AsNumber()),
									static_cast<float>((*ColorArr)[1]->AsNumber()),
									static_cast<float>((*ColorArr)[2]->AsNumber()),
									1.0f
								);
							}
							else
							{
								Info.Color = FLinearColor(0.88f, 0.88f, 0.95f, 1.0f);
							}
						}
					}
					UE_LOG(LogTemp, Log, TEXT("MythSkillEffects: %d skill elements carregados de Config/skill_elements.json"), Table.Num());
				}
			}
		}
		return Table;
	}
}

FLinearColor FMythSkillEffects::ElementColor(int32 SkillID)
{
	if (const FSkillElementInfo* Info = SkillElementTable().Find(SkillID))
	{
		return Info->Color;
	}
	const FSkillEntry* S = Tables().Skills.Find(SkillID);
	const FString Element = S ? S->Element : FString();
	// GDD Tecnico Magias HD-2D s.3 (Matriz Elemental):
	if (Element == TEXT("Fire"))   { return FLinearColor(1.0f, 0.35f, 0.02f); } // Laranja Incandescente
	if (Element == TEXT("Water"))  { return FLinearColor(0.05f, 0.55f, 1.0f); } // Azul Turquesa
	if (Element == TEXT("Wind"))   { return FLinearColor(0.1f, 0.9f, 0.95f); } // Azul Eletrico / Ciano
	if (Element == TEXT("Earth"))  { return FLinearColor(0.85f, 0.52f, 0.22f); } // Marrom Terra / Ocre
	if (Element == TEXT("Holy"))   { return FLinearColor(1.0f, 0.92f, 0.38f); } // Dourado Puro
	if (Element == TEXT("Dark") || Element == TEXT("Shadow")) { return FLinearColor(0.58f, 0.15f, 0.92f); } // Roxo Escuro / Violeta
	if (Element == TEXT("Ghost"))  { return FLinearColor(0.45f, 0.65f, 0.95f); } // Violeta Translucido / Etereo
	if (Element == TEXT("Poison")) { return FLinearColor(0.2f, 0.85f, 0.1f); } // Verde Toxico
	if (Element == TEXT("Undead")) { return FLinearColor(0.65f, 0.05f, 0.35f); } // Carmesim
	return FLinearColor(0.88f, 0.88f, 0.95f); // Branco Prateado (Neutro)
}

bool FMythSkillEffects::PlayEffectId(UObject* WorldContext, int32 EffectID, AActor* Target, const FVector& Location, AActor* Source)
{
	if (!FMythOptions::Get(TEXT("ShowEffects")))
	{
		return false;
	}
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
		// Particulas em sprite (3D com spriteName, ex.: o brilho da Bencao): varias copias do sprite subindo perto do alvo
		if (P.Type == TEXT("3D") && P.File.IsEmpty() && P.Raw.IsValid())
		{
			FString SpriteName;
			if (P.Raw->TryGetStringField(TEXT("spriteName"), SpriteName) && !SpriteName.IsEmpty() && !SpriteName.StartsWith(TEXT("..")))
			{
				double Copies = 1.0, Between = 0.0, FirstDelay = 0.0, SpreadX = 1.0;
				P.Raw->TryGetNumberField(TEXT("duplicate"), Copies);
				P.Raw->TryGetNumberField(TEXT("timeBetweenDupli"), Between);
				P.Raw->TryGetNumberField(TEXT("delayOffset"), FirstDelay);
				P.Raw->TryGetNumberField(TEXT("posxRand"), SpreadX);
				float Height = 100.f;
				if (const ACharacter* Char = Cast<ACharacter>(Target); Char && Char->GetCapsuleComponent()) { Height = Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.f; }
				UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
				const int32 N = FMath::Clamp(static_cast<int32>(Copies), 1, 12);
				for (int32 k = 0; k < N && World; ++k)
				{
					const float Delay = FMath::Max(0.001f, static_cast<float>(FirstDelay + Between * k) / 1000.f);
					const FVector Jitter(FMath::FRandRange(-1.f, 1.f) * static_cast<float>(SpreadX) * 30.f, FMath::FRandRange(-1.f, 1.f) * static_cast<float>(SpreadX) * 30.f, FMath::FRandRange(0.1f, 1.f) * Height);
					TWeakObjectPtr<AActor> WeakTarget(Target);
					FTimerHandle Handle;
					World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(WorldContext, [WorldContext, SpriteName, WeakTarget, Feet, Jitter]()
					{
						const FVector At = (WeakTarget.IsValid() ? FeetOf(WeakTarget.Get(), Feet) : Feet) + Jitter;
						AMythSprEffect::Play(WorldContext, SpriteName, At, nullptr, false, 0.f);
					}), Delay, false);
				}
				bAny = true;
				continue;
			}
		}
		// Desenhados pelo executavel (docs/ref/11): cilindros, particulas/projeteis e raios
		if (P.Type == TEXT("CYLINDER") || P.Type == TEXT("3D") || P.Type == TEXT("2D"))
		{
			const bool bProc = AMythProcEffect::Spawn(WorldContext, P.Raw, Target, Feet, Source);
			UE_LOG(LogTemp, Verbose, TEXT("MythSkillEffects: efeito %d %s '%s' %s"), EffectID, *P.Type, *P.File, bProc ? TEXT("ok") : TEXT("falhou"));
			bAny |= bProc;
		}
		// TRAIL (roBrowser Trail.js): pontas que nascem no caminho do conjurador ate o alvo (Rajada Congelante)
		if (P.Type == TEXT("TRAIL") && P.Raw.IsValid())
		{
			const FVector From = Source ? FeetOf(Source, Source->GetActorLocation()) : Feet;
			bAny |= AMythTrailEffect::Spawn(WorldContext, P.Raw, From, Feet);
		}
		// QuadHorn sozinho (pontas de gelo no alvo)
		if (P.Type == TEXT("QuadHorn") && P.Raw.IsValid())
		{
			bAny |= AMythQuadHorn::Spawn(WorldContext, P.Raw, Feet);
		}
		// FUNC quake: tremor de tela (Magnum Break 50 ms, Impacto Explosivo...); so se o efeito e perto do jogador
		if (P.Type == TEXT("FUNC") && P.Raw.IsValid() && P.Raw->GetStringField(TEXT("func")) == TEXT("quake"))
		{
			const APlayerController* PC = WorldContext ? WorldContext->GetWorld()->GetFirstPlayerController() : nullptr;
			if (AMythPlayerCharacter* Char = PC ? Cast<AMythPlayerCharacter>(PC->GetPawn()) : nullptr; Char && FVector::Dist2D(Char->GetActorLocation(), Feet) < 1500.f)
			{
				double Ms = 100.0;
				P.Raw->TryGetNumberField(TEXT("duration"), Ms);
				double Copies = 1.0, Between = 0.0; // Earthquake (666): 3 pulsos de 50 ms, 35 ms entre eles (docs/ref/13 5.2)
				P.Raw->TryGetNumberField(TEXT("duplicate"), Copies);
				P.Raw->TryGetNumberField(TEXT("timeBetweenDupli"), Between);
				Char->StartQuake(static_cast<float>(Ms) / 1000.f);
				for (int32 k = 1; k < static_cast<int32>(Copies); ++k)
				{
					FTimerHandle Handle;
					const float Pulse = static_cast<float>(Ms) / 1000.f;
					Char->GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(Char, [Char, Pulse]() { Char->StartQuake(Pulse); }), FMath::Max(0.001f, static_cast<float>(Between * k) / 1000.f), false);
				}
				bAny = true;
			}
		}
		if (P.Type == TEXT("SPR") && !P.File.IsEmpty())
		{
			bool bRepeat = false;
			double Ms = 0.0;
			if (P.Raw.IsValid())
			{
				P.Raw->TryGetBoolField(TEXT("repeat"), bRepeat);
				P.Raw->TryGetNumberField(TEXT("duration"), Ms);
			}
			AMythSprEffect* SprFx = AMythSprEffect::Play(WorldContext, P.File, Feet, P.bAttached ? Target : nullptr, bRepeat, static_cast<float>(Ms) / 1000.f);
			bool bHeadSpr = false;
			if (SprFx && Target && P.Raw.IsValid() && P.Raw->TryGetBoolField(TEXT("head"), bHeadSpr) && bHeadSpr)
			{
				double YOff = 0.0; // yOffset da tabela (pixels do sprite, negativo = para cima): sobe mais acima da cabeca
				P.Raw->TryGetNumberField(TEXT("yOffset"), YOff);
				SprFx->SetAboveHead(Target, FMath::Max(0.f, static_cast<float>(-YOff)) * 0.8f); // ex.: o anjo da Bencao fica acima da cabeca
			}
			const bool bSpr = SprFx != nullptr;
			UE_LOG(LogTemp, Verbose, TEXT("MythSkillEffects: efeito %d spr '%s' %s"), EffectID, *P.File, bSpr ? TEXT("ok") : TEXT("falhou"));
			bAny |= bSpr;
		}
		if (P.Type == TEXT("STR") && !P.File.IsEmpty())
		{
			const FString File = ResolveVariant(P.File);
			AMythStrEffect* StrFx = AMythStrEffect::Play(WorldContext, File, Feet, P.bAttached ? Target : nullptr);
			bool bHeadStr = false;
			if (StrFx && Target && P.bAttached && P.Raw.IsValid() && P.Raw->TryGetBoolField(TEXT("head"), bHeadStr) && bHeadStr)
			{
				float Half = 90.f;
				if (const ACharacter* HeadChar = Cast<ACharacter>(Target); HeadChar && HeadChar->GetCapsuleComponent()) { Half = HeadChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(); }
				StrFx->HeadOffset = Half * 1.7f; // efeito "head": acima da cabeca (Angelus, Signum Crucis...)
			}
			const bool bStr = StrFx != nullptr;
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
	const TArray<int32>& Ids = S->Stages[static_cast<int32>(Stage)];
	return Ids.Num() > 0 ? Ids[0] : 0;
}

bool FMythSkillEffects::SkillHasVisual(int32 SkillID)
{
	const FSkillEntry* S = Tables().Skills.Find(SkillID);
	if (!S)
	{
		return false;
	}
	static const TSet<FString> Visual = { TEXT("STR"), TEXT("SPR"), TEXT("3D"), TEXT("2D"), TEXT("CYLINDER"), TEXT("TRAIL"), TEXT("QuadHorn") };
	for (const TArray<int32>& Ids : S->Stages)
	{
		for (const int32 Id : Ids)
		{
			if (const TArray<FEffectPart>* Parts = Tables().Effects.Find(Id))
			{
				for (const FEffectPart& P : *Parts)
				{
					if (Visual.Contains(P.Type))
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

bool FMythSkillEffects::HidesCastAura(int32 SkillID)
{
	const FSkillEntry* S = Tables().Skills.Find(SkillID);
	return S && S->bHideCastAura;
}

bool FMythSkillEffects::Play(UObject* WorldContext, int32 SkillID, EMythSkillEffectStage Stage, AActor* Target, const FVector& Location, bool bFlashIfMissing, AActor* Source)
{
	const int32 EffectID = EffectIdFor(SkillID, Stage);
	bool bPlayed = false;
	if (const FSkillEntry* S = Tables().Skills.Find(SkillID))
	{
		for (const int32 Id : S->Stages[static_cast<int32>(Stage)])
		{
			bPlayed |= Id > 0 && PlayEffectId(WorldContext, Id, Target, Location, Source);
		}
	}
	UE_LOG(LogTemp, Verbose, TEXT("MythSkillEffects: skill %d estagio %d -> efeito %d (%s)"), SkillID, static_cast<int32>(Stage), EffectID, bPlayed ? TEXT("tocou") : TEXT("no effect"));
	// Luz Dinamica nas Habilidades por Elemento (GDD HD-2D Secao 4):
	// Instancia uma luz pontual colorida rapida (UPointLightComponent via AMythLightFlash)
	// durante os efeitos originais STR/SPR na cor exata do elemento da skill.
	const FLinearColor LightColor = ElementColor(SkillID);
	if (Stage == EMythSkillEffectStage::Hit || Stage == EMythSkillEffectStage::Effect || Stage == EMythSkillEffectStage::Release)
	{
		const FVector At = FeetOf(Target, Location) + FVector(0.f, 0.f, 75.f);
		AMythLightFlash::Flash(WorldContext, At, LightColor, 80.f, 0.45f, Target);
	}
	else if (Stage == EMythSkillEffectStage::Ground)
	{
		const FVector At = Location + FVector(0.f, 0.f, 40.f);
		AMythLightFlash::Flash(WorldContext, At, LightColor, 90.f, 0.65f, nullptr);
	}
	else if (Stage == EMythSkillEffectStage::Cast || Stage == EMythSkillEffectStage::BeginCast)
	{
		AActor* Caster = Source ? Source : Target;
		const FVector At = FeetOf(Caster, Location) + FVector(0.f, 0.f, 60.f);
		AMythLightFlash::Flash(WorldContext, At, LightColor, 45.f, 0.35f, Caster);
	}
	else if (!bPlayed && bFlashIfMissing)
	{
		AMythLightFlash::Flash(WorldContext, FeetOf(Target, Location) + FVector(0.f, 0.f, 70.f), LightColor, 55.f, 0.55f, Target);
	}
	return bPlayed;
}

int32 FMythSkillEffects::AreaSize(int32 SkillID, int32 Level)
{
	// Lado do quadrado, em celulas: Config/cast_size.json (roBrowser MagicTarget.js, por nivel) e, sem entrada, o raio de
	// Config/skill_area.json (2r+1); sem nada, 1 celula como o cliente
	static TMap<int32, TArray<int32>> Sizes;
	static TMap<int32, int32> Radius;
	static bool bLoaded = false;
	if (!bLoaded)
	{
		bLoaded = true;
		FString Text;
		TSharedPtr<FJsonObject> Root;
		if (FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("cast_size.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
		{
			for (const auto& Pair : Root->Values)
			{
				TArray<int32>& List = Sizes.Add(FCString::Atoi(*Pair.Key));
				for (const TSharedPtr<FJsonValue>& V : Pair.Value->AsArray()) { List.Add(static_cast<int32>(V->AsNumber())); }
			}
		}
		Root.Reset();
		if (FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("skill_area.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
		{
			for (const auto& Pair : Root->Values)
			{
				Radius.Add(FCString::Atoi(*Pair.Key), FMath::Clamp(static_cast<int32>(Pair.Value->AsNumber()), 0, 5)); // layouts especiais nao sao quadrados
			}
		}
	}
	if (const TArray<int32>* List = Sizes.Find(SkillID); List && List->Num() > 0)
	{
		return (*List)[FMath::Clamp(Level - 1, 0, List->Num() - 1)];
	}
	const int32* R = Radius.Find(SkillID);
	return R ? 2 * *R + 1 : 1;
}

void FMythSkillEffects::PlayCastAura(UObject* WorldContext, AActor* Caster, int32 SkillID, int32 CastMs, TArray<TWeakObjectPtr<AActor>>& OutActors)
{
	if (!Caster || CastMs <= 0 || HidesCastAura(SkillID) || !FMythOptions::Get(TEXT("ShowEffects")))
	{
		return;
	}
	// A propriedade do pacote e o elemento da skill (rAthena: skill_get_ele): 0 amarela, 1 agua, 2 terra, 3 fogo, 4 vento, 5 veneno, 6 sagrado, 7 sombra
	int32 Effect = 12;
	if (const FSkillEntry* S = Tables().Skills.Find(SkillID))
	{
		static const TMap<FString, int32> ByElement = { { TEXT("Water"), 54 }, { TEXT("Earth"), 57 }, { TEXT("Fire"), 55 }, { TEXT("Wind"), 56 },
			{ TEXT("Poison"), 59 }, { TEXT("Holy"), 58 }, { TEXT("Dark"), 454 }, { TEXT("Ghost"), 58 }, { TEXT("Undead"), 454 } };
		if (const int32* E = ByElement.Find(S->Element)) { Effect = *E; }
	}
	const TArray<FEffectPart>* Parts = Tables().Effects.Find(Effect);
	if (!Parts)
	{
		return;
	}
	for (const FEffectPart& P : *Parts)
	{
		if (!P.Raw.IsValid())
		{
			continue;
		}
		// Os aneis duram a conjuracao inteira (o cliente passa o tempo do pacote ao efeito)
		TSharedPtr<FJsonObject> Timed = MakeShared<FJsonObject>();
		Timed->Values = P.Raw->Values;
		Timed->SetNumberField(TEXT("duration"), CastMs);
		AActor* Spawned = nullptr;
		if (AMythProcEffect::Spawn(WorldContext, Timed, Caster, FeetOf(Caster, Caster->GetActorLocation()), nullptr, &Spawned) && Spawned)
		{
			OutActors.Add(Spawned);
		}
		if (!P.Wav.IsEmpty() && OutActors.Num() <= 1)
		{
			UMythRoSound::PlayWavAtLocation(WorldContext, P.Wav, Caster->GetActorLocation());
		}
	}
}

void FMythSkillEffects::PlayCastMarker(UObject* WorldContext, int32 SkillID, int32 Level, int32 CastMs, AActor* Target, const FVector& GroundLocation, TArray<TWeakObjectPtr<AActor>>& OutActors)
{
	if (CastMs <= 0 || !FMythOptions::Get(TEXT("ShowEffects")))
	{
		return;
	}
	const float Seconds = CastMs / 1000.f;
	if (Target)
	{
		if (AMythCastRing* Ring = AMythCastRing::Spawn(WorldContext, true, FeetOf(Target, Target->GetActorLocation()), 1.f, Seconds, Target))
		{
			OutActors.Add(Ring);
		}
	}
	else
	{
		// Skill de chao: o circulo cobre a area (raio em celulas -> quadrado de 2r+1; sem area conhecida, 1 celula como o cliente)
		const float Size = static_cast<float>(AreaSize(SkillID, Level));
		if (AMythCastRing* Ring = AMythCastRing::Spawn(WorldContext, false, GroundLocation, Size, Seconds, nullptr))
		{
			OutActors.Add(Ring);
		}
	}
}
