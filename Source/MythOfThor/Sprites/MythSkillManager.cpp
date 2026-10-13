#include "Sprites/MythSkillManager.h"

#include "Sprites/MythProcEffect.h"
#include "Sprites/MythTrailEffect.h"
#include "Sprites/MythStrEffect.h"
#include "UI/MythOptions.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	struct FVfxClass
	{
		FString Class;
		FString Element;
		int32 Area = 0;
		FString Niagara; // opcional: Niagara System da skill
	};

	struct FVfxTables
	{
		TMap<int32, FVfxClass> Skills;
		TMap<FString, FString> ClassNiagara; // classe -> Niagara System (Config/skill_vfx_overrides.json)
		bool bLoaded = false;
	};

	FVfxTables& Tables()
	{
		static FVfxTables T;
		if (T.bLoaded)
		{
			return T;
		}
		T.bLoaded = true;
		FString Text;
		TSharedPtr<FJsonObject> Root;
		if (FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("skill_vfx_classes.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
		{
			for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Root->Values)
			{
				const TSharedPtr<FJsonObject>* O = nullptr;
				if (!Pair.Value->TryGetObject(O) || !O)
				{
					continue;
				}
				FVfxClass C;
				C.Class = (*O)->GetStringField(TEXT("class"));
				C.Element = (*O)->GetStringField(TEXT("element"));
				C.Area = static_cast<int32>((*O)->GetNumberField(TEXT("area")));
				T.Skills.Add(FCString::Atoi(*Pair.Key), C);
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("MythSkillManager: Config/skill_vfx_classes.json nao encontrado ou invalido; so a tabela do cliente original sera usada"));
		}
		// Ajustes manuais opcionais: {"classes": {"buff_self": {"niagara": "/Game/..."}}, "skills": {"34": {"class": "heal", "niagara": "/Game/..."}}}
		Root.Reset();
		if (FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("skill_vfx_overrides.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
		{
			const TSharedPtr<FJsonObject>* Classes = nullptr;
			if (Root->TryGetObjectField(TEXT("classes"), Classes))
			{
				for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Classes)->Values)
				{
					const TSharedPtr<FJsonObject>* O = nullptr;
					FString Path;
					if (Pair.Value->TryGetObject(O) && O && (*O)->TryGetStringField(TEXT("niagara"), Path)) { T.ClassNiagara.Add(Pair.Key, Path); }
				}
			}
			const TSharedPtr<FJsonObject>* Skills = nullptr;
			if (Root->TryGetObjectField(TEXT("skills"), Skills))
			{
				for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Skills)->Values)
				{
					const TSharedPtr<FJsonObject>* O = nullptr;
					if (!Pair.Value->TryGetObject(O) || !O) { continue; }
					FVfxClass& C = T.Skills.FindOrAdd(FCString::Atoi(*Pair.Key));
					FString S;
					if ((*O)->TryGetStringField(TEXT("class"), S)) { C.Class = S; }
					if ((*O)->TryGetStringField(TEXT("element"), S)) { C.Element = S; }
					if ((*O)->TryGetStringField(TEXT("niagara"), S)) { C.Niagara = S; }
				}
			}
		}
		return T;
	}

	FLinearColor ElementTint(const FString& Element)
	{
		if (Element == TEXT("Fire"))   { return FLinearColor(1.f, 0.45f, 0.15f); }
		if (Element == TEXT("Water"))  { return FLinearColor(0.35f, 0.65f, 1.f); }
		if (Element == TEXT("Wind"))   { return FLinearColor(0.85f, 0.95f, 1.f); }
		if (Element == TEXT("Earth"))  { return FLinearColor(0.85f, 0.6f, 0.3f); }
		if (Element == TEXT("Holy"))   { return FLinearColor(1.f, 0.92f, 0.55f); }
		if (Element == TEXT("Dark"))   { return FLinearColor(0.6f, 0.3f, 0.95f); }
		if (Element == TEXT("Ghost"))  { return FLinearColor(0.75f, 0.65f, 1.f); }
		if (Element == TEXT("Poison")) { return FLinearColor(0.5f, 0.95f, 0.4f); }
		if (Element == TEXT("Undead")) { return FLinearColor(0.6f, 0.4f, 0.7f); }
		return FLinearColor(0.75f, 0.88f, 1.f); // neutro: azul claro
	}

	TSharedPtr<FJsonObject> Part(const TCHAR* Type, const TCHAR* File, const FLinearColor& Tint, std::initializer_list<TPair<const TCHAR*, double>> Numbers, bool bAttached = true)
	{
		TSharedPtr<FJsonObject> P = MakeShared<FJsonObject>();
		P->SetStringField(TEXT("type"), Type);
		P->SetStringField(TEXT("file"), File);
		P->SetStringField(TEXT("textureName"), File);
		P->SetBoolField(TEXT("attached"), bAttached);
		P->SetBoolField(TEXT("attachedEntity"), bAttached);
		P->SetNumberField(TEXT("red"), Tint.R);
		P->SetNumberField(TEXT("green"), Tint.G);
		P->SetNumberField(TEXT("blue"), Tint.B);
		for (const TPair<const TCHAR*, double>& N : Numbers)
		{
			P->SetNumberField(N.Key, N.Value);
		}
		return P;
	}

	/** Visual da tabela do cliente original: alguma parte de algum momento da skill desenha algo (nao so som)? */
	bool SkillHasVisual(int32 SkillID)
	{
		return FMythSkillEffects::SkillHasVisual(SkillID);
	}
}

FString UMythSkillManager::EffectClassOf(int32 SkillID)
{
	const FVfxClass* C = Tables().Skills.Find(SkillID);
	return C ? C->Class : FString();
}

bool UMythSkillManager::Play(UObject* WorldContext, int32 SkillID, EMythSkillEffectStage Stage, AActor* Target, const FVector& Location, bool bFlashIfMissing, AActor* Source)
{
	bool bPlayed = FMythSkillEffects::Play(WorldContext, SkillID, Stage, Target, Location, bFlashIfMissing, Source);
	if (!SkillHasVisual(SkillID) && FMythOptions::Get(TEXT("ShowEffects")))
	{
		bPlayed |= PlayGeneric(WorldContext, SkillID, Stage, Target, Location, Source);
	}
	return bPlayed;
}

bool UMythSkillManager::PlayGeneric(UObject* WorldContext, int32 SkillID, EMythSkillEffectStage Stage, AActor* Target, const FVector& Location, AActor* Source)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const FVfxClass* C = Tables().Skills.Find(SkillID);
	if (!World || !C || C->Class == TEXT("none"))
	{
		return false;
	}
	// Um unico visual generico por skill e alvo (os momentos da mesma skill chegam juntos)
	static TMap<uint64, double> Recent;
	const uint64 Key = (static_cast<uint64>(SkillID) << 32) ^ static_cast<uint64>(Target ? Target->GetUniqueID() : 0u) ^ (static_cast<uint64>(Stage) << 56);
	const double Now = World->GetTimeSeconds();
	if (const double* Last = Recent.Find(Key); Last && Now - *Last < 0.3)
	{
		return false;
	}
	Recent.Add(Key, Now);

	const FString& Cls = C->Class;
	FLinearColor Tint = ElementTint(C->Element);
	const FVector Feet = Target ? AMythStrEffect::FeetOf(Target) : Location;

	// Niagara pedido no JSON (da skill ou da classe): usa em vez do visual generico
	FString NiagaraPath = C->Niagara;
	if (NiagaraPath.IsEmpty())
	{
		if (const FString* ClassPath = Tables().ClassNiagara.Find(Cls)) { NiagaraPath = *ClassPath; }
	}
	const bool bBuffStage = Stage == EMythSkillEffectStage::Effect || Stage == EMythSkillEffectStage::Success;
	const bool bGroundStage = Stage == EMythSkillEffectStage::Ground;
	const bool bHitStage = Stage == EMythSkillEffectStage::Hit;
	const bool bWantedHere = ((Cls == TEXT("buff_self") || Cls == TEXT("buff_target") || Cls == TEXT("heal") || Cls == TEXT("debuff")) && bBuffStage)
		|| ((Cls == TEXT("ground_area") || Cls == TEXT("trap")) && bGroundStage)
		|| ((Cls == TEXT("melee_hit") || Cls == TEXT("ranged_hit") || Cls == TEXT("magic_hit")) && bHitStage);
	if (!bWantedHere)
	{
		return false;
	}
	if (!NiagaraPath.IsEmpty())
	{
		UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, *NiagaraPath);
		if (System)
		{
			UNiagaraComponent* Comp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, System, Feet, FRotator::ZeroRotator);
			if (Comp)
			{
				Comp->SetVariableLinearColor(TEXT("User.Color"), Tint); // so tem efeito se o sistema expoe esse parametro
				return true;
			}
		}
		UE_LOG(LogTemp, Error, TEXT("MythSkillManager: Niagara '%s' da skill %d (%s) nao existe; usando o efeito generico da classe"), *NiagaraPath, SkillID, *Cls);
	}

	bool bAny = false;
	auto Spawn = [&](const TSharedPtr<FJsonObject>& P) { bAny |= AMythProcEffect::Spawn(WorldContext, P, Target, Feet, Source); };
	if (Cls == TEXT("heal"))
	{
		Tint = FLinearColor(0.45f, 1.f, 0.55f);
	}
	if (Cls == TEXT("buff_self") || Cls == TEXT("buff_target") || Cls == TEXT("heal") || Cls == TEXT("debuff"))
	{
		const bool bDebuff = Cls == TEXT("debuff");
		if (bDebuff) { Tint = FLinearColor(0.7f, 0.35f, 0.95f); }
		// anel subindo em volta do alvo
		Spawn(Part(TEXT("CYLINDER"), bDebuff ? TEXT("ring_black") : TEXT("ring_white"), Tint,
			{ { TEXT("alphaMax"), 0.7 }, { TEXT("animation"), 1 }, { TEXT("blendMode"), 8 }, { TEXT("bottomSize"), 1.2 }, { TEXT("topSize"), 1.2 },
			  { TEXT("height"), 3.5 }, { TEXT("duration"), 1500 } }));
		// brilhos subindo
		TSharedPtr<FJsonObject> Sparks = Part(TEXT("3D"), TEXT("effect/ac_center2.tga"), Tint,
			{ { TEXT("alphaMax"), 1 }, { TEXT("duplicate"), 6 }, { TEXT("duration"), 1000 }, { TEXT("delayLate"), 250 }, { TEXT("posxRand"), 1.5 },
			  { TEXT("posyRand"), 1 }, { TEXT("poszEndRand"), 1 }, { TEXT("poszEndRandMiddle"), 5 }, { TEXT("poszStartRand"), 1 }, { TEXT("poszStartRandMiddle"), 1 },
			  { TEXT("sizeRandY"), 15 }, { TEXT("sizeRandYMiddle"), 40 }, { TEXT("sizeX"), 2.0 } });
		Sparks->SetBoolField(TEXT("fadeOut"), true);
		Spawn(Sparks);
	}
	else if (Cls == TEXT("ground_area") || Cls == TEXT("trap"))
	{
		const float Side = static_cast<float>(FMath::Max(1, C->Area > 0 ? C->Area : (Cls == TEXT("trap") ? 1 : FMythSkillEffects::AreaSize(SkillID, 1))));
		AMythCastRing::Spawn(WorldContext, false, Location, Side, 1.4f, nullptr);
		Spawn(Part(TEXT("CYLINDER"), TEXT("ring_white"), Tint,
			{ { TEXT("alphaMax"), 0.6 }, { TEXT("animation"), 1 }, { TEXT("blendMode"), 8 }, { TEXT("bottomSize"), Side * 0.5 }, { TEXT("topSize"), Side * 0.5 },
			  { TEXT("height"), 3.0 }, { TEXT("duration"), 1400 } }, false));
		bAny = true;
	}
	else // melee_hit, ranged_hit, magic_hit: clarao de impacto do elemento
	{
		TSharedPtr<FJsonObject> Burst = Part(TEXT("3D"), TEXT("effect/lens2.tga"), Tint,
			{ { TEXT("alphaMax"), 1 }, { TEXT("angle"), 90 }, { TEXT("duration"), 400 }, { TEXT("posz"), 1 }, { TEXT("sizeEndY"), 200 }, { TEXT("sizeStartY"), 10 }, { TEXT("sizeX"), 15 }, { TEXT("toAngle"), 0 } }, false);
		Burst->SetBoolField(TEXT("fadeOut"), true);
		Burst->SetBoolField(TEXT("rotate"), true);
		Spawn(Burst);
	}
	UE_LOG(LogTemp, Verbose, TEXT("MythSkillManager: skill %d sem visual no cliente -> efeito generico '%s' (%s)"), SkillID, *Cls, *C->Element);
	return bAny;
}
