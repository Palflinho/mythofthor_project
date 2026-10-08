#include "UI/MythHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	FLinearColor ColorFor(ECombatTextType Type)
	{
		switch (Type)
		{
		case ECombatTextType::Critical:    return FLinearColor(1.f, 0.85f, 0.1f);
		case ECombatTextType::DamageTaken: return FLinearColor(1.f, 0.15f, 0.15f);
		case ECombatTextType::Miss:        return FLinearColor(0.3f, 0.55f, 1.f);
		case ECombatTextType::Heal:        return FLinearColor(0.3f, 1.f, 0.4f);
		default:                           return FLinearColor(1.f, 1.f, 0.85f);
		}
	}
}

void AMythHUD::AddCombatText(const FVector& WorldLocation, const FString& Text, ECombatTextType Type)
{
	FEntry& E = Entries.AddDefaulted_GetRef();
	E.Location = WorldLocation;
	E.Text = Text;
	E.Type = Type;
	E.JitterX = FMath::FRandRange(-18.f, 18.f); // evita que golpes simultaneos se sobreponham
}

void AMythHUD::AddDamageNumber(const FVector& WorldLocation, int32 Amount, ECombatTextType Type)
{
	AddCombatText(WorldLocation, Type == ECombatTextType::Miss ? TEXT("MISS") : FString::FromInt(Amount), Type);
}

void AMythHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas || Entries.IsEmpty())
	{
		return;
	}

	UFont* Font = GEngine->GetLargeFont();
	const float Dt = GetWorld()->GetDeltaSeconds();
	FFontRenderInfo Info;
	Info.bEnableShadow = true;
	Info.GlowInfo.bEnableGlow = false;

	for (int32 i = Entries.Num() - 1; i >= 0; --i)
	{
		FEntry& E = Entries[i];
		E.Age += Dt;
		if (E.Age >= Lifetime)
		{
			Entries.RemoveAtSwap(i);
			continue;
		}

		const FVector2D Screen2D = FVector2D(Project(E.Location));
		const float T = E.Age / Lifetime;
		const bool bHops = E.Type == ECombatTextType::DamageDealt || E.Type == ECombatTextType::Critical;

		// Dano causado salta (arco) e depois sobe devagar; os demais sobem direto.
		float OffsetY = -RisePixels * T;
		if (bHops)
		{
			const float HopT = FMath::Clamp(E.Age / 0.25f, 0.f, 1.f);
			OffsetY -= HopPixels * FMath::Sin(HopT * PI);
		}

		float Scale = BaseScale;
		if (E.Type == ECombatTextType::Critical)
		{
			Scale *= 1.8f - 0.5f * FMath::Min(E.Age / 0.2f, 1.f); // "pop" inicial do critico
		}

		FLinearColor Color = ColorFor(E.Type);
		Color.A = T < 0.6f ? 1.f : 1.f - (T - 0.6f) / 0.4f; // fade no final

		float W = 0.f, H = 0.f;
		Canvas->TextSize(Font, E.Text, W, H, Scale, Scale);
		Canvas->SetDrawColor(Color.ToFColor(true));
		Canvas->DrawText(Font, E.Text, Screen2D.X - W * 0.5f + E.JitterX, Screen2D.Y - H + OffsetY, Scale, Scale, Info);
	}
}

static AMythHUD* GetMythHUD(const UObject* WorldContextObject)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	return PC ? Cast<AMythHUD>(PC->GetHUD()) : nullptr;
}

void UMythCombatText::ShowDamage(const UObject* WorldContextObject, const FVector& WorldLocation, int32 Amount, ECombatTextType Type)
{
	if (AMythHUD* HUD = GetMythHUD(WorldContextObject))
	{
		HUD->AddDamageNumber(WorldLocation, Amount, Type);
	}
}

void UMythCombatText::ShowMiss(const UObject* WorldContextObject, const FVector& WorldLocation)
{
	ShowDamage(WorldContextObject, WorldLocation, 0, ECombatTextType::Miss);
}
