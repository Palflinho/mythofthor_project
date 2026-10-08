#include "Gameplay/MythStatsComponent.h"

void UMythStatsComponent::SetIdentity(const FString& InName, int32 InBaseLevel)
{
	CharName = InName;
	BaseLevel = InBaseLevel;
	OnStatsChanged.Broadcast();
}

void UMythStatsComponent::SetVitals(int32 InHP, int32 InMaxHP, int32 InSP, int32 InMaxSP)
{
	MaxHP = FMath::Max(1, InMaxHP);
	MaxSP = FMath::Max(0, InMaxSP);
	HP = FMath::Clamp(InHP, 0, MaxHP);
	SP = FMath::Clamp(InSP, 0, MaxSP);
	OnStatsChanged.Broadcast();
}

void UMythStatsComponent::SetCombat(int32 InASPD, float InAttackRangeUU)
{
	ASPD = FMath::Clamp(InASPD, 0, 199);
	AttackRangeUU = FMath::Max(0.f, InAttackRangeUU);
	OnStatsChanged.Broadcast();
}

void UMythStatsComponent::SetExp(int64 InBaseExp, int64 InBaseExpToNext)
{
	BaseExpToNext = FMath::Max<int64>(1, InBaseExpToNext);
	BaseExp = FMath::Clamp<int64>(InBaseExp, 0, BaseExpToNext);
	OnStatsChanged.Broadcast();
}
