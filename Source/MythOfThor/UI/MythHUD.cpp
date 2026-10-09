#include "UI/MythHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "Player/MythPlayerController.h"
#include "Sprites/MythMonster.h"
#include "UI/MythRoUITextures.h"
#include "UI/MythRoClientTables.h"
#include "World/MythGroundItem.h"
#include "EngineUtils.h"
#include "Camera/PlayerCameraManager.h"
#include "CanvasItem.h"
#include "Engine/Texture2D.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	FLinearColor ColorFor(ECombatTextType Type)
	{
		switch (Type)
		{
		case ECombatTextType::Critical:    return FLinearColor(1.f, 0.9f, 0.25f);  // digitos amarelos sobre a estrela
		case ECombatTextType::DamageTaken: return FLinearColor(1.f, 0.3f, 0.3f);   // dano no jogador: vermelho
		case ECombatTextType::Miss:        return FLinearColor(0.3f, 0.55f, 1.f);
		case ECombatTextType::Heal:        return FLinearColor(0.3f, 1.f, 0.4f);
		default:                           return FLinearColor::White;              // dano causado: branco
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
	DrawTargetRing();
	DrawGroundItemLabels();
	DrawCastBarsAndShouts();
	DrawStatusIcons();

	if (!Canvas || Entries.IsEmpty())
	{
		return;
	}

	// Numeros de dano do cliente original: digitos de 이팩트/숫자.spr (0-9) e textos de 이팩트/msg.spr
	// (0 Miss, 1 Guard, 2 Critical, 3 estrela do critico). Os sprites sao brancos e recebem a cor do tipo.
	static const FString NumberSpr = TEXT("이팩트/숫자"); // 이팩트/숫자
	static const FString MsgSpr = TEXT("이팩트/msg");             // 이팩트/msg
	const float Dt = GetWorld()->GetDeltaSeconds();

	auto DrawSprite = [this](UTexture2D* Tex, float X, float Y, float Scale, const FLinearColor& Color)
	{
		if (!Tex || !Tex->GetResource())
		{
			return;
		}
		FCanvasTileItem Tile(FVector2D(X, Y), Tex->GetResource(), FVector2D(Tex->GetSizeX(), Tex->GetSizeY()) * Scale, Color);
		Tile.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Tile);
	};

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

		// Como no original: o dano causado sai num arco (sobe, desce passando do ponto de partida) indo para o
		// lado e encolhendo; dano sofrido e cura sobem em linha reta; Miss sobe.
		float OffsetX = E.JitterX;
		float OffsetY = 0.f;
		float Scale = BaseScale;
		if (bHops)
		{
			OffsetY = -FMath::Sin(-HALF_PI + PI * (0.5f + T * 1.5f)) * HopPixels - 20.f;
			OffsetX += T * 45.f;
			Scale = FMath::Lerp(BaseScale * 1.9f, BaseScale * 0.75f, T);
		}
		else
		{
			OffsetY = -RisePixels * T - 20.f;
		}
		if (E.Type == ECombatTextType::Critical)
		{
			Scale *= 1.2f;
		}

		FLinearColor Color = ColorFor(E.Type);
		Color.A = T < 0.7f ? 1.f : 1.f - (T - 0.7f) / 0.3f; // some no final

		if (E.Type == ECombatTextType::Miss)
		{
			UTexture2D* Miss = FMythRoUITextures::GetSpriteFrame(MsgSpr, 0);
			if (Miss)
			{
				DrawSprite(Miss, Screen2D.X + OffsetX - Miss->GetSizeX() * Scale * 0.5f, Screen2D.Y + OffsetY - Miss->GetSizeY() * Scale, Scale, FLinearColor(1.f, 1.f, 1.f, Color.A));
			}
			continue;
		}

		// Largura total dos digitos (sobreposicao de 1 px entre eles, como no cliente)
		TArray<UTexture2D*, TInlineAllocator<8>> Digits;
		float TotalW = 0.f, MaxH = 0.f;
		for (const TCHAR Ch : E.Text)
		{
			if (Ch < TEXT('0') || Ch > TEXT('9'))
			{
				continue;
			}
			UTexture2D* D = FMythRoUITextures::GetSpriteFrame(NumberSpr, Ch - TEXT('0'));
			if (D)
			{
				Digits.Add(D);
				TotalW += (D->GetSizeX() - 1) * Scale;
				MaxH = FMath::Max(MaxH, D->GetSizeY() * Scale);
			}
		}
		const float CX = Screen2D.X + OffsetX;
		const float Y = Screen2D.Y + OffsetY - MaxH;
		if (E.Type == ECombatTextType::Critical)
		{
			// Estrela vermelha atras do numero do critico
			if (UTexture2D* Star = FMythRoUITextures::GetSpriteFrame(MsgSpr, 3))
			{
				const float S = Scale * 0.55f;
				DrawSprite(Star, CX - Star->GetSizeX() * S * 0.5f, Y + MaxH * 0.5f - Star->GetSizeY() * S * 0.5f, S, FLinearColor(1.f, 1.f, 1.f, Color.A));
			}
		}
		float X = CX - TotalW * 0.5f;
		for (UTexture2D* D : Digits)
		{
			DrawSprite(D, X, Y + MaxH - D->GetSizeY() * Scale, Scale, Color);
			X += (D->GetSizeX() - 1) * Scale;
		}
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

void AMythHUD::DrawTargetRing()
{
	const AMythPlayerController* PC = Cast<AMythPlayerController>(PlayerOwner);
	const AMythMonster* Target = PC ? PC->GetFocusedTarget() : nullptr;
	if (!Canvas || !Target || Target->IsDead())
	{
		return;
	}

	// Circulo no chao, sob o alvo: projeta pontos do plano do piso para a tela (vira uma elipse).
	const UCapsuleComponent* Cap = Target->GetCapsuleComponent();
	FVector Center = Target->GetActorLocation();
	Center.Z -= Cap->GetScaledCapsuleHalfHeight() - 2.f;

	const float Pulse = 1.f + 0.06f * FMath::Sin(GetWorld()->GetTimeSeconds() * 6.f);
	const float BaseRadius = (Cap->GetScaledCapsuleRadius() + 22.f) * Pulse;

	constexpr int32 Segments = 40;
	struct FRing { float Scale; float Thickness; FLinearColor Color; };
	const FRing Rings[] = {
		{ 1.00f, 2.5f, FLinearColor(1.f, 0.25f, 0.2f, 0.95f) },
		{ 0.82f, 1.5f, FLinearColor(1.f, 0.9f, 0.9f, 0.8f) },
	};

	for (const FRing& Ring : Rings)
	{
		FVector2D Prev = FVector2D::ZeroVector;
		bool bHavePrev = false;
		for (int32 i = 0; i <= Segments; ++i)
		{
			const float A = (2.f * PI * i) / Segments;
			const FVector P = Center + FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * BaseRadius * Ring.Scale;
			FVector2D Screen;
			if (!PC->ProjectWorldLocationToScreen(P, Screen, false))
			{
				bHavePrev = false;
				continue;
			}
			if (bHavePrev)
			{
				DrawLine(Prev.X, Prev.Y, Screen.X, Screen.Y, Ring.Color, Ring.Thickness);
			}
			Prev = Screen;
			bHavePrev = true;
		}
	}

	// Placa do alvo: nome e nivel, barra fina de HP (verde sobre vermelho) e porcentagem.
	FVector2D Head;
	if (!PC->ProjectWorldLocationToScreen(Target->GetActorLocation() + FVector(0.f, 0.f, Cap->GetScaledCapsuleHalfHeight() + 70.f), Head, false))
	{
		return;
	}

	UFont* Font = GEngine->GetLargeFont();
	FFontRenderInfo Info;
	Info.bEnableShadow = true;

	const FString Name = Target->GetDisplayName();
	float TW = 0.f, TH = 0.f;
	Canvas->TextSize(Font, Name, TW, TH, 0.8f, 0.8f);
	Canvas->SetDrawColor(FColor::White);
	Canvas->DrawText(Font, Name, Head.X - TW * 0.5f, Head.Y - TH - 8.f, 0.8f, 0.8f, Info);

	constexpr float BarW = 64.f, BarH = 5.f;
	const float Frac = Target->GetHealthFraction();
	const float X = Head.X - BarW * 0.5f;
	const float Y = Head.Y;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.85f), X - 1.f, Y - 1.f, BarW + 2.f, BarH + 2.f);
	DrawRect(FLinearColor(0.85f, 0.1f, 0.1f, 1.f), X, Y, BarW, BarH);
	DrawRect(FLinearColor(0.2f, 0.9f, 0.25f, 1.f), X, Y, BarW * Frac, BarH);

	const FString Pct = FString::Printf(TEXT("%d%%"), FMath::CeilToInt(Frac * 100.f));
	float PW = 0.f, PH = 0.f;
	Canvas->TextSize(Font, Pct, PW, PH, 0.55f, 0.55f);
	Canvas->DrawText(Font, Pct, Head.X - PW * 0.5f, Y + BarH + 3.f, 0.55f, 0.55f, Info);
}

void AMythHUD::DrawGroundItemLabels()
{
	// Nome de cada item no chao, logo abaixo dele, na cor da raridade (estilo Diablo); contorno escuro para ler
	// sobre qualquer chao. Nomes que se sobreporiam sao empilhados para baixo. Itens longe da camera nao mostram nome.
	if (!Canvas || !PlayerOwner || !PlayerOwner->PlayerCameraManager)
	{
		return;
	}
	struct FLabel { FString Text; FLinearColor Color; FVector2D Pos; FVector2D Size; };
	TArray<FLabel> Labels;
	const FVector CamLoc = PlayerOwner->PlayerCameraManager->GetCameraLocation();
	UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
	const float TextScale = 1.15f;
	for (TActorIterator<AMythGroundItem> It(GetWorld()); It; ++It)
	{
		const FVector At = It->GetActorLocation();
		if (FVector::DistSquared(At, CamLoc) > FMath::Square(4500.f))
		{
			continue;
		}
		const FVector Screen = Project(At - FVector(0.f, 0.f, 8.f), true);
		if (Screen.Z <= 0.f)
		{
			continue;
		}
		FLabel L;
		L.Text = It->GetLabel();
		L.Color = FMythRoClientTables::RarityColor(It->Rarity);
		float W = 0.f, H = 0.f;
		Canvas->TextSize(Font, L.Text, W, H, TextScale, TextScale);
		L.Size = FVector2D(W + 6.f, H + 1.f);
		L.Pos = FVector2D(Screen.X - L.Size.X * 0.5f, Screen.Y + 6.f);
		Labels.Add(L);
	}
	Labels.Sort([](const FLabel& A, const FLabel& B) { return A.Pos.Y < B.Pos.Y; });
	TArray<FBox2D> Placed;
	for (FLabel& L : Labels)
	{
		for (int32 Guard = 0; Guard < 12; ++Guard)
		{
			const FBox2D Box(L.Pos, L.Pos + L.Size);
			const bool bHit = Placed.ContainsByPredicate([&Box](const FBox2D& O) { return O.Intersect(Box); });
			if (!bHit)
			{
				break;
			}
			L.Pos.Y += L.Size.Y;
		}
		Placed.Add(FBox2D(L.Pos, L.Pos + L.Size));
		FCanvasTextItem Text(L.Pos + FVector2D(3.f, 0.f), FText::FromString(L.Text), Font, L.Color);
		Text.Scale = FVector2D(TextScale, TextScale);
		Text.bOutlined = true;
		Text.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.9f);
		Canvas->DrawItem(Text);
	}
}

void AMythHUD::AddCastBar(AActor* Caster, float Seconds)
{
	if (!Caster || Seconds <= 0.05f)
	{
		return;
	}
	CastBars.RemoveAll([Caster](const FCastBar& B) { return B.Actor.Get() == Caster; });
	CastBars.Add({ Caster, static_cast<float>(GetWorld()->GetTimeSeconds()), Seconds });
}

void AMythHUD::AddSkillShout(AActor* Caster, const FString& SkillName)
{
	if (Caster && !SkillName.IsEmpty())
	{
		Shouts.RemoveAll([Caster](const FShout& S) { return S.Actor.Get() == Caster; });
		Shouts.Add({ Caster, SkillName + TEXT("!!"), static_cast<float>(GetWorld()->GetTimeSeconds()) });
	}
}

void AMythHUD::DrawCastBarsAndShouts()
{
	if (!Canvas)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	// Barra de conjuracao: moldura escura e preenchimento verde que enche ate o fim do tempo
	CastBars.RemoveAll([Now](const FCastBar& B) { return !B.Actor.IsValid() || Now - B.Start > B.Duration; });
	for (const FCastBar& B : CastBars)
	{
		const FVector Screen = Project(B.Actor->GetActorLocation() + FVector(0.f, 0.f, 150.f), true);
		if (Screen.Z <= 0.f)
		{
			continue;
		}
		const float W = 60.f, H = 7.f;
		const float Frac = FMath::Clamp((Now - B.Start) / B.Duration, 0.f, 1.f);
		const FVector2D Pos(Screen.X - W * 0.5f, Screen.Y);
		FCanvasTileItem Back(Pos - FVector2D(1.f, 1.f), FVector2D(W + 2.f, H + 2.f), FLinearColor(0.f, 0.f, 0.f, 0.75f));
		Back.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Back);
		FCanvasTileItem Fill(Pos, FVector2D(W * Frac, H), FLinearColor(0.35f, 0.85f, 0.35f, 1.f));
		Canvas->DrawItem(Fill);
	}
	// Grito da skill: texto amarelo com contorno sobre a cabeca, subindo e sumindo em 1,6 s
	Shouts.RemoveAll([Now](const FShout& S) { return !S.Actor.IsValid() || Now - S.Start > 1.6f; });
	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	for (const FShout& S : Shouts)
	{
		const float T = (Now - S.Start) / 1.6f;
		const FVector Screen = Project(S.Actor->GetActorLocation() + FVector(0.f, 0.f, 170.f), true);
		if (Screen.Z <= 0.f)
		{
			continue;
		}
		FCanvasTextItem Text(FVector2D(Screen.X, Screen.Y - 25.f * T), FText::FromString(S.Text), Font,
			FLinearColor(1.f, 0.92f, 0.45f, 1.f - FMath::Max(0.f, T - 0.7f) / 0.3f));
		Text.bCentreX = true;
		Text.bOutlined = true;
		Text.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.9f);
		Canvas->DrawItem(Text);
	}
}

// ------------------------------------------------------------------------------------------------ buffs

namespace
{
	struct FStatusIconInfo
	{
		FString Icon;
		FString Name;
		bool bDebuff = false;
	};

	/** Config/status_icons.json (tools/gen_status_icons.py): EFST -> icone em data/texture/effect e nome. */
	const TMap<int32, FStatusIconInfo>& StatusIconTable()
	{
		static TMap<int32, FStatusIconInfo> Table;
		static bool bLoaded = false;
		if (bLoaded)
		{
			return Table;
		}
		bLoaded = true;
		FString Text;
		TSharedPtr<FJsonObject> Root;
		if (FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("status_icons.json")))
			&& FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
		{
			for (const auto& Pair : Root->Values)
			{
				const TSharedPtr<FJsonObject> O = Pair.Value->AsObject();
				FStatusIconInfo& I = Table.Add(FCString::Atoi(*Pair.Key));
				I.Icon = O->GetStringField(TEXT("icon"));
				I.Name = O->GetStringField(TEXT("name"));
				I.bDebuff = O->GetBoolField(TEXT("debuff"));
			}
		}
		return Table;
	}

	FString FormatRemaining(float Seconds)
	{
		const int32 Sec = FMath::CeilToInt(Seconds);
		if (Sec >= 3600) { return FString::Printf(TEXT("%dh"), Sec / 3600); }
		if (Sec >= 60) { return FString::Printf(TEXT("%dm"), Sec / 60); }
		return FString::Printf(TEXT("%ds"), Sec);
	}
}

void AMythHUD::SetStatusIcon(int32 EfstID, bool bActive, int32 TotalMs, int32 RemainMs)
{
	StatusIcons.RemoveAll([EfstID](const FStatusIcon& I) { return I.Efst == EfstID; });
	if (!bActive || !StatusIconTable().Contains(EfstID))
	{
		return; // estados sem icone no cliente (internos do servidor) nao aparecem, como no RO
	}
	FStatusIcon& I = StatusIcons.AddDefaulted_GetRef();
	I.Efst = EfstID;
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	I.Total = FMath::Max(TotalMs, RemainMs) / 1000.f;
	I.End = RemainMs > 0 ? Now + RemainMs / 1000.f : 0.f;
}

void AMythHUD::DrawStatusIcons()
{
	if (!Canvas || StatusIcons.IsEmpty())
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	StatusIcons.RemoveAll([Now](const FStatusIcon& I) { return I.End > 0.f && Now >= I.End; });

	// Coluna da direita abaixo do minimapa, como no cliente original; a escala acompanha a altura da tela (base 900)
	const float Sc = FMath::Max(Canvas->ClipY / 900.f, 0.6f);
	const float IconSize = 28.f * Sc, Gap = 4.f * Sc, BarW = 44.f * Sc, BarH = 5.f * Sc;
	const float Right = Canvas->ClipX - 10.f * Sc;
	float Y = 200.f * Sc;
	float MouseX = -1.f, MouseY = -1.f;
	if (PlayerOwner)
	{
		PlayerOwner->GetMousePosition(MouseX, MouseY);
	}
	UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
	const FStatusIconInfo* Hovered = nullptr;
	float HoverY = 0.f;
	auto DrawLabel = [this, Font, Sc](const FString& Text, float RightX, float TopY, const FLinearColor& Color)
	{
		float W = 0.f, H = 0.f;
		Canvas->StrLen(Font, Text, W, H);
		FCanvasTextItem Item(FVector2D(RightX - W * Sc, TopY), FText::FromString(Text), Font, Color);
		Item.Scale = FVector2D(Sc, Sc);
		Item.bOutlined = true;
		Item.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.9f);
		Canvas->DrawItem(Item);
	};
	for (const FStatusIcon& I : StatusIcons)
	{
		const FStatusIconInfo* Info = StatusIconTable().Find(I.Efst);
		UTexture2D* Tex = Info ? FMythRoUITextures::Get(TEXT("../effect/") + Info->Icon) : nullptr;
		const FVector2D IconPos(Right - IconSize, Y);
		if (Tex && Tex->GetResource())
		{
			FCanvasTileItem Tile(IconPos, Tex->GetResource(), FVector2D(IconSize, IconSize), FLinearColor::White);
			Tile.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(Tile);
		}
		// Barra de duracao ao lado do icone: esvazia da esquerda para a direita e fica laranja no fim
		if (I.End > 0.f && I.Total > 0.f)
		{
			const float Remain = FMath::Max(I.End - Now, 0.f);
			const float Frac = FMath::Clamp(Remain / I.Total, 0.f, 1.f);
			const FVector2D BarPos(IconPos.X - Gap - BarW, Y + IconSize - BarH - 2.f * Sc);
			FCanvasTileItem Back(BarPos - FVector2D(1.f, 1.f), FVector2D(BarW + 2.f, BarH + 2.f), FLinearColor(0.f, 0.f, 0.f, 0.7f));
			Back.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(Back);
			const FLinearColor Full = Info && Info->bDebuff ? FLinearColor(0.95f, 0.3f, 0.3f) : FLinearColor(0.4f, 0.85f, 0.45f);
			const FLinearColor Ending(1.f, 0.6f, 0.2f);
			FCanvasTileItem Fill(BarPos + FVector2D(BarW * (1.f - Frac), 0.f), FVector2D(BarW * Frac, BarH),
				Frac < 0.2f ? FMath::Lerp(Ending, Full, Frac / 0.2f) : Full);
			Canvas->DrawItem(Fill);
			DrawLabel(FormatRemaining(Remain), BarPos.X + BarW, BarPos.Y - 13.f * Sc, FLinearColor(1.f, 0.85f, 0.6f));
		}
		if (Info && MouseX >= IconPos.X && MouseX <= IconPos.X + IconSize && MouseY >= Y && MouseY <= Y + IconSize)
		{
			Hovered = Info;
			HoverY = Y;
		}
		Y += IconSize + Gap;
		if (Y > Canvas->ClipY - 200.f * Sc)
		{
			break; // muitos estados: o resto nao cabe na coluna
		}
	}
	// Nome do estado ao passar o mouse (o cliente original mostra a descricao)
	if (Hovered)
	{
		DrawLabel(Hovered->Name, Right - IconSize - Gap - BarW - 8.f * Sc, HoverY + IconSize * 0.5f - 7.f * Sc,
			Hovered->bDebuff ? FLinearColor(1.f, 0.45f, 0.45f) : FLinearColor(0.65f, 0.9f, 0.65f));
	}
}
