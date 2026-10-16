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
	struct FVfxSkill
	{
		FString Class;
		FString Element;
		int32 Area = 0;
		FString Niagara; // opcional: Niagara System da skill
	};

	struct FVfxTables
	{
		TMap<int32, FVfxSkill> Skills;
		TMap<FString, FString> ClassNiagara; // classe -> Niagara System (Config/skill_vfx_overrides.json)
		bool bLoaded = false;
	};

	FVfxTables& VfxTables()
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
				FVfxSkill C;
				C.Class = (*O)->GetStringField(TEXT("class"));
				C.Element = (*O)->GetStringField(TEXT("element"));
				C.Area = static_cast<int32>((*O)->GetNumberField(TEXT("area")));
				T.Skills.Add(FCString::Atoi(*Pair.Key), C);
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("MythSkillManager: Config/skill_vfx_classes.json not found or invalid; only the original client table will be used"));
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
					FVfxSkill& C = T.Skills.FindOrAdd(FCString::Atoi(*Pair.Key));
					FString S;
					if ((*O)->TryGetStringField(TEXT("class"), S)) { C.Class = S; }
					if ((*O)->TryGetStringField(TEXT("element"), S)) { C.Element = S; }
					if ((*O)->TryGetStringField(TEXT("niagara"), S)) { C.Niagara = S; }
				}
			}
		}
		return T;
	}

	FLinearColor VfxElementTint(const FString& Element)
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

	TSharedPtr<FJsonObject> VfxPart(const TCHAR* Type, const TCHAR* File, const FLinearColor& Tint, std::initializer_list<TPair<const TCHAR*, double>> Numbers, bool bAttached = true)
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
	bool HasOriginalVisual(int32 SkillID)
	{
		return FMythSkillEffects::SkillHasVisual(SkillID);
	}
}

FString UMythSkillManager::EffectClassOf(int32 SkillID)
{
	const FVfxSkill* C = VfxTables().Skills.Find(SkillID);
	return C ? C->Class : FString();
}

bool UMythSkillManager::Play(UObject* WorldContext, int32 SkillID, EMythSkillEffectStage Stage, AActor* Target, const FVector& Location, bool bFlashIfMissing, AActor* Source)
{
	bool bPlayed = FMythSkillEffects::Play(WorldContext, SkillID, Stage, Target, Location, bFlashIfMissing, Source);
	if ((!bPlayed || !HasOriginalVisual(SkillID)) && FMythOptions::Get(TEXT("ShowEffects")))
	{
		bPlayed |= PlayGeneric(WorldContext, SkillID, Stage, Target, Location, Source);
	}
	return bPlayed;
}

bool UMythSkillManager::PlayGeneric(UObject* WorldContext, int32 SkillID, EMythSkillEffectStage Stage, AActor* Target, const FVector& Location, AActor* Source)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const FVfxSkill* C = VfxTables().Skills.Find(SkillID);
	if (!World || !C || C->Class == TEXT("none"))
	{
		return false;
	}
	// Um unico visual generico por skill e alvo (os momentos da mesma skill chegam juntos)
	static TMap<uint64, double> Recent;
	const uint64 Key = (static_cast<uint64>(SkillID) << 32) ^ static_cast<uint64>(Target ? Target->GetUniqueID() : 0u);
	const double Now = World->GetTimeSeconds();
	if (const double* Last = Recent.Find(Key); Last && Now - *Last < 0.25)
	{
		return false;
	}

	const FString& Cls = C->Class;
	FLinearColor Tint = VfxElementTint(C->Element);
	const FVector Feet = Target ? AMythStrEffect::FeetOf(Target) : Location;

	// Niagara pedido no JSON (da skill ou da classe): usa em vez do visual generico
	FString NiagaraPath = C->Niagara;
	if (NiagaraPath.IsEmpty())
	{
		if (const FString* ClassPath = VfxTables().ClassNiagara.Find(Cls)) { NiagaraPath = *ClassPath; }
	}
	const bool bBuffStage = Stage == EMythSkillEffectStage::Effect || Stage == EMythSkillEffectStage::Success || Stage == EMythSkillEffectStage::SuccessCaster;
	const bool bGroundStage = Stage == EMythSkillEffectStage::Ground || Stage == EMythSkillEffectStage::Effect;
	const bool bHitStage = Stage == EMythSkillEffectStage::Hit || Stage == EMythSkillEffectStage::Effect || Stage == EMythSkillEffectStage::Success;
	const bool bWantedHere = ((Cls == TEXT("buff_self") || Cls == TEXT("buff_target") || Cls == TEXT("heal") || Cls == TEXT("debuff")) && bBuffStage)
		|| ((Cls == TEXT("ground_area") || Cls == TEXT("trap")) && bGroundStage)
		|| ((Cls == TEXT("melee_hit") || Cls == TEXT("ranged_hit") || Cls == TEXT("magic_hit")) && bHitStage);
	if (!bWantedHere)
	{
		return false;
	}
	Recent.Add(Key, Now);

	if (!NiagaraPath.IsEmpty())
	{
		UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, *NiagaraPath);
		if (System)
		{
			UNiagaraComponent* Comp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, System, Feet, FRotator::ZeroRotator);
			if (Comp)
			{
				Comp->SetVariableLinearColor(TEXT("User.Color"), Tint);
				return true;
			}
		}
		UE_LOG(LogTemp, Error, TEXT("MythSkillManager: Niagara '%s' of skill %d (%s) does not exist; using the generic class effect"), *NiagaraPath, SkillID, *Cls);
	}

	bool bAny = false;
	auto Spawn = [&](const TSharedPtr<FJsonObject>& P) { bAny |= AMythProcEffect::Spawn(WorldContext, P, Target, Feet, Source); };
	if (Cls == TEXT("heal"))
	{
		Tint = FLinearColor(0.45f, 1.f, 0.55f);
		bAny |= AMythStrEffect::Play(WorldContext, TEXT("cure_min"), Feet, Target) != nullptr;
	}
	if (Cls == TEXT("buff_self") || Cls == TEXT("buff_target") || Cls == TEXT("heal") || Cls == TEXT("debuff"))
	{
		const bool bDebuff = Cls == TEXT("debuff");
		if (bDebuff)
		{
			Tint = FLinearColor(0.7f, 0.35f, 0.95f);
			bAny |= AMythStrEffect::Play(WorldContext, TEXT("slowp"), Feet, Target) != nullptr;
		}
		else if (Cls != TEXT("heal"))
		{
			bAny |= AMythStrEffect::Play(WorldContext, TEXT("concentration"), Feet, Target) != nullptr;
		}
		// anel subindo em volta do alvo + onda de choque na base
		Spawn(VfxPart(TEXT("CYLINDER"), bDebuff ? TEXT("ring_black") : TEXT("ring_white"), Tint,
			{ { TEXT("alphaMax"), 0.8 }, { TEXT("animation"), 1 }, { TEXT("blendMode"), 8 }, { TEXT("bottomSize"), 1.3 }, { TEXT("topSize"), 1.5 },
			  { TEXT("height"), 3.8 }, { TEXT("duration"), 1200 } }));
		Spawn(VfxPart(TEXT("CYLINDER"), TEXT("ring_white"), Tint,
			{ { TEXT("alphaMax"), 0.7 }, { TEXT("animation"), 4 }, { TEXT("blendMode"), 8 }, { TEXT("bottomSize"), 0.8 }, { TEXT("topSize"), 2.8 },
			  { TEXT("height"), 0.3 }, { TEXT("duration"), 650 } }));
		// brilhos subindo
		TSharedPtr<FJsonObject> Sparks = VfxPart(TEXT("3D"), TEXT("effect/ac_center2.tga"), Tint,
			{ { TEXT("alphaMax"), 1 }, { TEXT("duplicate"), 10 }, { TEXT("duration"), 950 }, { TEXT("delayLate"), 100 }, { TEXT("posxRand"), 1.5 },
			  { TEXT("posyRand"), 1 }, { TEXT("poszEndRand"), 1 }, { TEXT("poszEndRandMiddle"), 5 }, { TEXT("poszStartRand"), 1 }, { TEXT("poszStartRandMiddle"), 1 },
			  { TEXT("sizeRandY"), 15 }, { TEXT("sizeRandYMiddle"), 45 }, { TEXT("sizeX"), 2.5 } });
		Sparks->SetBoolField(TEXT("fadeOut"), true);
		Spawn(Sparks);
	}
	else if (Cls == TEXT("ground_area") || Cls == TEXT("trap"))
	{
		const float Side = static_cast<float>(FMath::Max(1, C->Area > 0 ? C->Area : (Cls == TEXT("trap") ? 2 : FMythSkillEffects::AreaSize(SkillID, 1))));
		AMythCastRing::Spawn(WorldContext, false, Location, Side, 1.8f, nullptr);
		Spawn(VfxPart(TEXT("CYLINDER"), TEXT("ring_white"), Tint,
			{ { TEXT("alphaMax"), 0.75 }, { TEXT("animation"), 1 }, { TEXT("blendMode"), 8 }, { TEXT("bottomSize"), Side * 0.55 }, { TEXT("topSize"), Side * 0.65 },
			  { TEXT("height"), 3.2 }, { TEXT("duration"), 1500 } }, false));
		Spawn(VfxPart(TEXT("CYLINDER"), TEXT("alpha_down"), Tint,
			{ { TEXT("alphaMax"), 0.55 }, { TEXT("animation"), 4 }, { TEXT("blendMode"), 2 }, { TEXT("bottomSize"), Side * 0.4 }, { TEXT("topSize"), Side * 0.8 },
			  { TEXT("height"), 1.5 }, { TEXT("duration"), 1200 } }, false));
		bAny = true;
	}
	else // melee_hit, ranged_hit, magic_hit: impacto elemental HD-2D em 3 camadas (.str + cilindro de choque + explosao radial)
	{
		const TCHAR* HitStr = TEXT("hit");
		if (C->Element == TEXT("Fire"))        { HitStr = TEXT("firehit1"); }
		else if (C->Element == TEXT("Water"))  { HitStr = TEXT("freeze"); }
		else if (C->Element == TEXT("Wind"))   { HitStr = TEXT("windhit1"); }
		else if (C->Element == TEXT("Earth"))  { HitStr = TEXT("earthhit"); }
		else if (C->Element == TEXT("Holy"))   { HitStr = TEXT("holyhit"); }
		else if (C->Element == TEXT("Poison")) { HitStr = TEXT("poison"); }
		else if (C->Element == TEXT("Dark") || C->Element == TEXT("Undead")) { HitStr = TEXT("devil"); }
		else if (C->Element == TEXT("Ghost") || Cls == TEXT("magic_hit"))    { HitStr = TEXT("magical"); }
		else if (Cls == TEXT("melee_hit"))     { HitStr = TEXT("shockwavehit"); }

		bAny |= AMythStrEffect::Play(WorldContext, HitStr, Feet, Target) != nullptr;

		// Onda de choque cilindrica na cor do elemento
		TSharedPtr<FJsonObject> Ring = VfxPart(TEXT("CYLINDER"), TEXT("ring_white"), Tint,
			{ { TEXT("alphaMax"), 0.8 }, { TEXT("animation"), 4 }, { TEXT("bottomSize"), 1.0 }, { TEXT("topSize"), 3.4 },
			  { TEXT("height"), 0.5 }, { TEXT("duration"), 550 } }, false);
		Ring->SetBoolField(TEXT("fade"), true);
		Ring->SetBoolField(TEXT("rotate"), true);
		Spawn(Ring);

		// Explosao radial de 12 feixes luminosos
		TSharedPtr<FJsonObject> Burst = VfxPart(TEXT("3D"), TEXT("effect/lens1.tga"), Tint,
			{ { TEXT("alphaMax"), 0.9 }, { TEXT("duplicate"), 12 }, { TEXT("duration"), 450 }, { TEXT("posxEndRand"), 26 }, { TEXT("posyEndRand"), 26 },
			  { TEXT("sizeStartX"), 12 }, { TEXT("sizeStartY"), 110 }, { TEXT("sizeEndX"), 8 }, { TEXT("sizeEndY"), 140 } }, false);
		Burst->SetBoolField(TEXT("fadeOut"), true);
		Burst->SetBoolField(TEXT("rotateToTarget"), true);
		Spawn(Burst);
	}
	UE_LOG(LogTemp, Verbose, TEXT("MythSkillManager: skill %d -> HD-2D generic effect '%s' (%s)"), SkillID, *Cls, *C->Element);
	return bAny;
}
