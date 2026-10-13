#include "UI/MythRoSkillTree.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	struct FTreeData
	{
		bool bLoaded = false;
		TMap<int32, TMap<int32, int32>> Jobs;
		TMap<int32, FMythTreeSkill> Skills;
	};

	FTreeData& Data()
	{
		static FTreeData D;
		if (D.bLoaded)
		{
			return D;
		}
		D.bLoaded = true;
		FString Text;
		TSharedPtr<FJsonObject> Root;
		if (!FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("skill_tree.json")))
			|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("MythRoSkillTree: Config/skill_tree.json ausente (rode tools/gen_skill_tree.py)"));
			return D;
		}
		const TSharedPtr<FJsonObject>* Jobs;
		if (Root->TryGetObjectField(TEXT("jobs"), Jobs))
		{
			for (const auto& Job : (*Jobs)->Values)
			{
				TMap<int32, int32>& Grid = D.Jobs.Add(FCString::Atoi(*Job.Key));
				for (const auto& Cell : Job.Value->AsObject()->Values)
				{
					Grid.Add(FCString::Atoi(*Cell.Key), static_cast<int32>(Cell.Value->AsNumber()));
				}
			}
		}
		const TSharedPtr<FJsonObject>* Skills;
		if (Root->TryGetObjectField(TEXT("skills"), Skills))
		{
			for (const auto& Entry : (*Skills)->Values)
			{
				const TSharedPtr<FJsonObject> S = Entry.Value->AsObject();
				FMythTreeSkill& Skill = D.Skills.Add(FCString::Atoi(*Entry.Key));
				Skill.Name = S->GetStringField(TEXT("name"));
				Skill.MaxLevel = static_cast<int32>(S->GetNumberField(TEXT("maxlv")));
				for (const TSharedPtr<FJsonValue>& Pair : S->GetArrayField(TEXT("need")))
				{
					const TArray<TSharedPtr<FJsonValue>>& P = Pair->AsArray();
					if (P.Num() >= 2)
					{
						Skill.Need.Add(FIntPoint(static_cast<int32>(P[0]->AsNumber()), static_cast<int32>(P[1]->AsNumber())));
					}
				}
			}
		}
		UE_LOG(LogTemp, Log, TEXT("MythRoSkillTree: %d classes, %d skills"), D.Jobs.Num(), D.Skills.Num());
		return D;
	}

	/** 1a classe de uma 2a classe (montarias contam como a classe a pe). */
	int32 FirstClassOf(int32 Second)
	{
		switch (Second)
		{
		case 7: case 13: case 14: case 21: return 1;
		case 9: case 16: return 2;
		case 11: case 19: case 20: return 3;
		case 8: case 15: return 4;
		case 10: case 18: return 5;
		case 12: case 17: return 6;
		default: return 0;
		}
	}

	int32 Unmounted(int32 Job)
	{
		switch (Job)
		{
		case 13: return 7;
		case 21: return 14;
		case 4014: return 4008;
		case 4022: return 4015;
		default: return Job;
		}
	}
}

const TMap<int32, int32>* FMythRoSkillTree::Grid(int32 JobClass)
{
	FTreeData& D = Data();
	if (const TMap<int32, int32>* G = D.Jobs.Find(Unmounted(JobClass)))
	{
		return G;
	}
	// Transclasse sem arvore propria: usa a da classe normal (4002 -> 1 ... 4021 -> 20)
	return JobClass > 4001 && JobClass <= 4022 ? D.Jobs.Find(Unmounted(JobClass) - 4001) : nullptr;
}

const FMythTreeSkill* FMythRoSkillTree::Skill(int32 SkillID)
{
	return Data().Skills.Find(SkillID);
}

TArray<int32> FMythRoSkillTree::JobChain(int32 JobClass)
{
	const int32 Job = Unmounted(JobClass);
	TArray<int32> Chain;
	if (Job == 23) // Super Aprendiz: arvore propria
	{
		Chain.Add(23);
		return Chain;
	}
	const bool bTrans = Job >= 4001 && Job <= 4022;
	const int32 Base = bTrans ? Job - 4001 : Job;
	const int32 Novice = bTrans ? 4001 : 0;
	Chain.Add(Novice);
	if (Base >= 1 && Base <= 6)
	{
		Chain.Add(bTrans ? Base + 4001 : Base);
	}
	else if (Base >= 7 && Base <= 21)
	{
		const int32 First = FirstClassOf(Base);
		Chain.Add(bTrans ? First + 4001 : First);
		Chain.Add(Base); // a 2a classe normal (Cavaleiro)...
		if (bTrans)
		{
			Chain.Add(Job); // ...e a transclasse (Lorde)
		}
	}
	return Chain;
}

FString FMythRoSkillTree::JobTabName(int32 JobClass)
{
	static const TMap<int32, FString> Names = {
		{ 0, TEXT("Aprendiz") }, { 1, TEXT("Espadachim") }, { 2, TEXT("Mago") }, { 3, TEXT("Arqueiro") },
		{ 4, TEXT("Novico") }, { 5, TEXT("Mercador") }, { 6, TEXT("Gatuno") }, { 7, TEXT("Cavaleiro") },
		{ 8, TEXT("Sacerdote") }, { 9, TEXT("Bruxo") }, { 10, TEXT("Ferreiro") }, { 11, TEXT("Cacador") },
		{ 12, TEXT("Mercenario") }, { 14, TEXT("Templario") }, { 15, TEXT("Monge") }, { 16, TEXT("Sabio") },
		{ 17, TEXT("Arruaceiro") }, { 18, TEXT("Alquimista") }, { 19, TEXT("Bardo") }, { 20, TEXT("Odalisca") },
		{ 23, TEXT("Super Aprendiz") }, { 4001, TEXT("Aprendiz T.") }, { 4008, TEXT("Lorde") },
		{ 4009, TEXT("Sumo Sacerdote") }, { 4010, TEXT("Arquimago") }, { 4011, TEXT("Mestre-Ferreiro") },
		{ 4012, TEXT("Atirador") }, { 4013, TEXT("Algoz") }, { 4015, TEXT("Paladino") }, { 4016, TEXT("Mestre") },
		{ 4017, TEXT("Professor") }, { 4018, TEXT("Desordeiro") }, { 4019, TEXT("Criador") }, { 4020, TEXT("Menestrel") },
		{ 4021, TEXT("Cigana") },
	};
	const int32 Job = Unmounted(JobClass);
	if (const FString* N = Names.Find(Job))
	{
		return *N;
	}
	if (Job >= 4002 && Job <= 4007)
	{
		return JobTabName(Job - 4001) + TEXT(" T.");
	}
	return FString::Printf(TEXT("Class %d"), Job);
}
