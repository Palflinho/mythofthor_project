#include "UI/MythHUD.h"
#include "UI/MythQuestTitles.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "Player/MythPlayerController.h"
#include "Sprites/MythMonster.h"
#include "Gameplay/MythStatsComponent.h"
#include "UI/MythRoUITextures.h"
#include "UI/MythRoClientTables.h"
#include "World/MythGroundItem.h"
#include "World/MythMapInfo.h"
#include "UI/MythRoHUDWidget.h"
#include "Network/RagnarokNetworkSubsystem.h"
#include "UI/MythOptions.h"
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
	E.JitterX = FMath::RandBool() ? 1.f : -1.f; // lado do arco (o cliente usa a rotacao do alvo +-90 graus)
}

void AMythHUD::AddDamageNumber(const FVector& WorldLocation, int32 Amount, ECombatTextType Type)
{
	AddCombatText(WorldLocation, Type == ECombatTextType::Miss ? TEXT("MISS") : FString::FromInt(Amount), Type);
}

void AMythHUD::DrawHUD()
{
	Super::DrawHUD();
	DrawCursorTile();
	DrawMapBanner();
	DrawChatRoomTitles();
	DrawQuestTracker();
	DrawTargetRing();
	DrawSelfBars();
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

	// Trajetorias do MsgEffect do cliente (docs/ref/05 3.1-3.3), em "ticks" de 1/24 s e unidades do RO (10 uu)
	const FVector CamRight = PlayerOwner && PlayerOwner->PlayerCameraManager
		? FRotationMatrix(PlayerOwner->PlayerCameraManager->GetCameraRotation()).GetUnitAxis(EAxis::Y) : FVector::RightVector;
	constexpr float RoUnit = 10.f;
	for (int32 i = Entries.Num() - 1; i >= 0; --i)
	{
		FEntry& E = Entries[i];
		E.Age += Dt;
		const float Tick = E.Age * 24.f;
		const bool bHeal = E.Type == ECombatTextType::Heal;
		const bool bMiss = E.Type == ECombatTextType::Miss;
		const float LifeTicks = bHeal ? 120.f : (bMiss ? 80.f : 70.f);
		if (Tick >= LifeTicks)
		{
			Entries.RemoveAtSwap(i);
			continue;
		}

		FVector World = E.Location;
		float Zoom = 1.2f;   // escala do cliente; 1,2 = tamanho estavel do dano (BaseScale na tela)
		float Alpha = 1.f;
		if (bHeal)
		{
			// Cura: sobe devagar em linha reta crescendo ate 3x; opaca ate o tick 90 e some rapido
			World.Z += Tick * 0.1f * RoUnit;
			Zoom = FMath::Min(3.f, 1.f + Tick * 0.18f);
			Alpha = Tick < 90.f ? 1.f : FMath::Max(0.f, 250.f - (Tick - 90.f) * 8.f) / 250.f;
		}
		else if (bMiss)
		{
			// Miss: sobe 0,54 unidade por tick, 80 ticks
			World.Z += Tick * 0.54f * RoUnit;
			Alpha = FMath::Max(0.f, 250.f - Tick * 3.1f) / 250.f;
		}
		else
		{
			// Dano: arco para o lado (t/3 + 3) e salto 8 - (2 - t/30) * t (Y do RO para baixo), sumindo em 70 ticks
			World += CamRight * E.JitterX * (Tick * 0.3333f + 3.f) * RoUnit;
			World.Z -= (8.f - (2.f - Tick * 0.0333f) * Tick) * RoUnit;
			const bool bCrit = E.Type == ECombatTextType::Critical;
			constexpr float OrgZoom = 3.f;
			Zoom = bCrit ? FMath::Max(1.f, OrgZoom - Tick * 0.144f) * 1.2f : FMath::Max(1.2f, OrgZoom - Tick * 0.24f);
			Alpha = FMath::Max(0.f, 250.f - Tick * 3.4f) / 250.f;
		}
		const FVector2D Screen2D = FVector2D(Project(World));
		const float OffsetX = 0.f;
		const float OffsetY = 0.f;
		const float Scale = BaseScale * Zoom / 1.2f;

		FLinearColor Color = ColorFor(E.Type);
		Color.A = Alpha;

		if (bMiss)
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
	if (!FMythOptions::Get(TEXT("Miss")))
	{
		return;
	}
	ShowDamage(WorldContextObject, WorldLocation, 0, ECombatTextType::Miss);
}

namespace
{
	/** Config/skill_area.json (tools/gen_skill_area.py): raio em celulas da area de cada skill de chao. */
	int32 SkillAreaRadius(int32 SkillID)
	{
		static TMap<int32, int32> Table;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			bLoaded = true;
			FString Text;
			TSharedPtr<FJsonObject> Root;
			if (FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("skill_area.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
			{
				for (const auto& Pair : Root->Values)
				{
					Table.Add(FCString::Atoi(*Pair.Key), FMath::Clamp(static_cast<int32>(Pair.Value->AsNumber()), 0, 5)); // limite: o raio de layouts especiais nao e uma area quadrada
				}
			}
		}
		const int32* R = Table.Find(SkillID);
		return R ? *R : 0;
	}
}

void AMythHUD::DrawCursorTile()
{
	AMythPlayerController* PC = Cast<AMythPlayerController>(PlayerOwner);
	if (!Canvas || !PC)
	{
		return;
	}
	if (PC->GetHud() && PC->GetHud()->IsPointerOverUi())
	{
		return; // o cursor esta numa janela: sem quadrado no chao
	}
	FHitResult Hit;
	if (!PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit) || !Hit.bBlockingHit)
	{
		return;
	}
	const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld());
	if (!Info)
	{
		return;
	}
	const FIntPoint Cell = PC->WorldToCell(Hit.ImpactPoint);
	const FVector Center = Info->CellToWorld(Cell, 0.f);
	const float Z = Hit.ImpactPoint.Z + 1.f;
	constexpr float Half = 25.f; // celula de 50 uu
	auto Project = [PC, Z](const FVector& P, FVector2D& Out) { return PC->ProjectWorldLocationToScreen(FVector(P.X, P.Y, Z), Out, false); };

	// Quadrado do tile: so os cantos (um terco de cada lado), em verde
	const FVector Corners[4] = { Center + FVector(-Half, -Half, 0.f), Center + FVector(Half, -Half, 0.f), Center + FVector(Half, Half, 0.f), Center + FVector(-Half, Half, 0.f) };
	const FLinearColor Green(0.25f, 0.95f, 0.35f, 0.95f);
	for (int32 i = 0; i < 4; ++i)
	{
		FVector2D A, B, C;
		if (Project(Corners[i], A) && Project(Corners[(i + 1) % 4], B) && Project(Corners[(i + 3) % 4], C))
		{
			DrawLine(A.X, A.Y, A.X + (B.X - A.X) * 0.33f, A.Y + (B.Y - A.Y) * 0.33f, Green, 2.f);
			DrawLine(A.X, A.Y, A.X + (C.X - A.X) * 0.33f, A.Y + (C.Y - A.Y) * 0.33f, Green, 2.f);
		}
	}

	// Mirando skill de area: anel branco cobrindo a area e o nivel da skill ao lado (imagem 24 do doc 12)
	if (PC->IsTargetingSkill() && (PC->GetPendingSkillInf() & 0x02))
	{
		const float Radius = (SkillAreaRadius(PC->GetPendingSkillID()) + 0.5f) * 50.f;
		constexpr int32 Segments = 40;
		FVector2D Prev, Right(0.f, 0.f);
		bool bHavePrev = false;
		for (int32 i = 0; i <= Segments; ++i)
		{
			const float A = (2.f * PI * i) / Segments;
			FVector2D S;
			if (!Project(Center + FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * Radius, S))
			{
				bHavePrev = false;
				continue;
			}
			if (bHavePrev)
			{
				DrawLine(Prev.X, Prev.Y, S.X, S.Y, FLinearColor(1.f, 1.f, 1.f, 0.95f), 2.f);
			}
			if (S.X > Right.X) { Right = S; }
			Prev = S;
			bHavePrev = true;
		}
		if (UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr; Font && Right.X > 0.f)
		{
			FFontRenderInfo Info2;
			Info2.bEnableShadow = true;
			Canvas->SetDrawColor(FColor(255, 240, 80));
			Canvas->DrawText(Font, FString::FromInt(PC->GetPendingSkillLevel()), Right.X + 6.f, Right.Y - 10.f, 1.2f, 1.2f, Info2);
		}
	}
}

void AMythHUD::ShowMapBanner(const FString& MapId)
{
	if (!FMythOptions::Get(TEXT("MapName")))
	{
		return;
	}
	static TMap<FString, FString> MapDisplayNames;
	static bool bLoaded = false;
	if (!bLoaded)
	{
		bLoaded = true;
		FString Text;
		TSharedPtr<FJsonObject> Root;
		if (FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("maps.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
		{
			for (const auto& Pair : Root->Values)
			{
				const TSharedPtr<FJsonObject>* Obj = nullptr;
				FString Name;
				if (Pair.Value->TryGetObject(Obj) && (*Obj)->TryGetStringField(TEXT("name"), Name))
				{
					Name.TrimStartAndEndInline();
					MapDisplayNames.Emplace(Pair.Key, Name);
				}
			}
		}
	}
	const FString* Found = MapDisplayNames.Find(MapId.ToLower());
	BannerText = Found ? *Found : MapId;
	BannerStart = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

void AMythHUD::DrawMapBanner()
{
	if (!Canvas || BannerText.IsEmpty() || !GetWorld())
	{
		return;
	}
	const float T = static_cast<float>(GetWorld()->GetTimeSeconds() - BannerStart);
	constexpr float FadeIn = 0.6f, Hold = 2.6f, FadeOut = 1.2f;
	if (T < 0.f || T > FadeIn + Hold + FadeOut)
	{
		return;
	}
	const float Alpha = T < FadeIn ? T / FadeIn : (T > FadeIn + Hold ? 1.f - (T - FadeIn - Hold) / FadeOut : 1.f);
	UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;
	if (!Font)
	{
		return;
	}
	const float Scale = 2.2f;
	float TW = 0.f, TH = 0.f;
	Canvas->TextSize(Font, BannerText, TW, TH, Scale, Scale);
	FFontRenderInfo Info;
	Info.bEnableShadow = true;
	// faixa escura suave atras (o cliente usa uma sombra sob o nome) e o nome em branco
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.35f * Alpha), Canvas->SizeX * 0.5f - TW * 0.5f - 40.f, 70.f, TW + 80.f, TH + 14.f);
	Canvas->SetDrawColor(FColor(255, 255, 255, static_cast<uint8>(255.f * Alpha)));
	Canvas->DrawText(Font, BannerText, Canvas->SizeX * 0.5f - TW * 0.5f, 76.f, Scale, Scale, Info);
}

void AMythHUD::DrawQuestTracker()
{
	const URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr;
	UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
	if (!Canvas || !Net || !Font)
	{
		return;
	}
	// Rastreador de missoes: abaixo do minimapa, so as quests ativas (olho ligado na janela Alt+U)
	float Y = 190.f;
	const float Right = Canvas->SizeX - 12.f;
	FFontRenderInfo Info;
	Info.bEnableShadow = true;
	int32 Shown = 0;
	for (const FRoQuest& Q : Net->GetQuests())
	{
		if (!Q.bActive || Shown >= 4)
		{
			continue;
		}
		++Shown;
		TArray<FString> Lines;
		Lines.Add(MythQuests::TitleFor(Q.Id));
		for (const FRoQuestObjective& O : Q.Objectives)
		{
			Lines.Add(FString::Printf(TEXT("  %s %d/%d"), *O.Name, O.Count, O.Total));
		}
		for (int32 i = 0; i < Lines.Num(); ++i)
		{
			float TW = 0.f, TH = 0.f;
			Canvas->TextSize(Font, Lines[i], TW, TH);
			Canvas->SetDrawColor(i == 0 ? FColor(255, 240, 140) : FColor(235, 235, 235));
			Canvas->DrawText(Font, Lines[i], Right - TW, Y, 1.f, 1.f, Info);
			Y += TH + 2.f;
		}
		Y += 6.f;
	}
}

void AMythHUD::DrawChatRoomTitles()
{
	RoomHits.Reset();
	AMythPlayerController* PC = Cast<AMythPlayerController>(PlayerOwner);
	const URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr;
	if (!Canvas || !PC || !Net || Net->GetChatRooms().Num() == 0)
	{
		return;
	}
	UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
	if (!Font)
	{
		return;
	}
	for (TActorIterator<AMythMonster> It(GetWorld()); It; ++It)
	{
		if (!It->IsNetUnit() || It->NetObjectType != 0)
		{
			continue;
		}
		for (const TPair<int32, FRoChatRoom>& Pair : Net->GetChatRooms())
		{
			const FRoChatRoom& Room = Pair.Value;
			if (Room.OwnerId != It->NetID)
			{
				continue;
			}
			FVector2D Head;
			if (!PC->ProjectWorldLocationToScreen(It->GetActorLocation() + FVector(0.f, 0.f, 150.f), Head, false))
			{
				continue;
			}
			const FString Text = FString::Printf(TEXT("%s (%d/%d)"), *Room.Title, Room.Users, Room.Limit);
			float TW = 0.f, TH = 0.f;
			Canvas->TextSize(Font, Text, TW, TH, 1.f, 1.f);
			const FVector2D Pos(Head.X - TW * 0.5f - 6.f, Head.Y - TH - 10.f);
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), Pos.X, Pos.Y, TW + 12.f, TH + 6.f);
			Canvas->SetDrawColor(Room.Type == 0 ? FColor(255, 180, 120) : FColor(255, 240, 140));
			Canvas->DrawText(Font, Text, Pos.X + 6.f, Pos.Y + 3.f);
			FRoomHit Hit;
			Hit.Box = FBox2D(Pos, Pos + FVector2D(TW + 12.f, TH + 6.f));
			Hit.Id = Room.Id;
			Hit.Type = Room.Type;
			RoomHits.Add(Hit);
		}
	}
}

int32 AMythHUD::ChatRoomAt(const FVector2D& CanvasPos, int32& OutType) const
{
	for (const FRoomHit& H : RoomHits)
	{
		if (H.Box.IsInside(CanvasPos))
		{
			OutType = H.Type;
			return H.Id;
		}
	}
	return 0;
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
	// Barra de HP do monstro atingido ABAIXO dos pes (docs/ref/12): verde sobre fundo roxo escuro
	FVector2D FeetScreen;
	if (!PC->ProjectWorldLocationToScreen(Center, FeetScreen, false))
	{
		return;
	}
	const float X = FeetScreen.X - BarW * 0.5f;
	const float Y = FeetScreen.Y + 8.f;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.85f), X - 1.f, Y - 1.f, BarW + 2.f, BarH + 2.f);
	DrawRect(FLinearColor(0.22f, 0.08f, 0.3f, 1.f), X, Y, BarW, BarH);
	DrawRect(FLinearColor(0.2f, 0.9f, 0.25f, 1.f), X, Y, BarW * Frac, BarH);
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

void AMythHUD::AddSpeechBubble(AActor* Speaker, const FString& Text)
{
	if (Speaker && !Text.IsEmpty())
	{
		Bubbles.RemoveAll([Speaker](const FBubble& B) { return B.Actor.Get() == Speaker; });
		Bubbles.Add({ Speaker, Text, static_cast<float>(GetWorld()->GetTimeSeconds()) });
	}
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
	// Baloes de fala: caixa branca com borda escura e texto preto, 4,5 s, seguem quem falou
	Bubbles.RemoveAll([Now](const FBubble& B) { return !B.Actor.IsValid() || Now - B.Start > 4.5f; });
	UFont* BubbleFont = GEngine ? GEngine->GetSmallFont() : nullptr;
	for (const FBubble& B : Bubbles)
	{
		const FVector Screen = Project(B.Actor->GetActorLocation() + FVector(0.f, 0.f, 135.f), true);
		if (Screen.Z <= 0.f || !BubbleFont)
		{
			continue;
		}
		// Quebra em linhas de ~26 caracteres
		TArray<FString> Lines;
		FString Rest = B.Text;
		while (Rest.Len() > 26)
		{
			int32 Space = INDEX_NONE;
			Rest.Left(26).FindLastChar(TEXT(' '), Space);
			if (Space <= 0) { Space = 26; }
			Lines.Add(Rest.Left(Space));
			Rest = Rest.Mid(Space).TrimStart();
		}
		Lines.Add(Rest);
		float W = 0.f, H = 0.f, MaxW = 0.f;
		for (const FString& L : Lines)
		{
			float LW = 0.f, LH = 0.f;
			Canvas->TextSize(BubbleFont, L, LW, LH, 1.f, 1.f);
			MaxW = FMath::Max(MaxW, LW);
			H = LH;
		}
		W = MaxW + 12.f;
		const float TotalH = Lines.Num() * H + 8.f;
		const float X = Screen.X - W * 0.5f;
		const float Y = Screen.Y - TotalH - 6.f;
		DrawRect(FLinearColor(0.1f, 0.1f, 0.1f, 0.9f), X - 1.f, Y - 1.f, W + 2.f, TotalH + 2.f);
		DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.95f), X, Y, W, TotalH);
		for (int32 i = 0; i < Lines.Num(); ++i)
		{
			FCanvasTextItem Text(FVector2D(X + 6.f, Y + 4.f + i * H), FText::FromString(Lines[i]), BubbleFont, FLinearColor::Black);
			Canvas->DrawItem(Text);
		}
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

void AMythHUD::DrawSelfBars()
{
	// Barras do proprio jogador: HP (verde; amarela abaixo de 50%, vermelha abaixo de 25%) e SP (azul), ~60x4, sempre visiveis
	const AMythPlayerController* PC = Cast<AMythPlayerController>(PlayerOwner);
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	const UMythStatsComponent* Stats = Pawn ? Pawn->FindComponentByClass<UMythStatsComponent>() : nullptr;
	if (!Canvas || !Stats || Stats->GetMaxHP() <= 0)
	{
		return;
	}
	FVector Feet = Pawn->GetActorLocation();
	if (const UCapsuleComponent* Cap = Pawn->FindComponentByClass<UCapsuleComponent>())
	{
		Feet.Z -= Cap->GetScaledCapsuleHalfHeight();
	}
	FVector2D Screen;
	if (!PC->ProjectWorldLocationToScreen(Feet, Screen, false))
	{
		return;
	}
	constexpr float BarW = 60.f, BarH = 4.f;
	const float X = Screen.X - BarW * 0.5f;
	const float Y = Screen.Y + 6.f;
	const float HpFrac = FMath::Clamp(static_cast<float>(Stats->GetHP()) / Stats->GetMaxHP(), 0.f, 1.f);
	const float SpFrac = Stats->GetMaxSP() > 0 ? FMath::Clamp(static_cast<float>(Stats->GetSP()) / Stats->GetMaxSP(), 0.f, 1.f) : 0.f;
	const FLinearColor HpColor = HpFrac < 0.25f ? FLinearColor(0.9f, 0.15f, 0.1f) : (HpFrac < 0.5f ? FLinearColor(0.95f, 0.8f, 0.15f) : FLinearColor(0.2f, 0.85f, 0.25f));
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.85f), X - 1.f, Y - 1.f, BarW + 2.f, BarH * 2.f + 3.f);
	DrawRect(FLinearColor(0.15f, 0.15f, 0.18f, 1.f), X, Y, BarW, BarH);
	DrawRect(HpColor, X, Y, BarW * HpFrac, BarH);
	DrawRect(FLinearColor(0.15f, 0.15f, 0.18f, 1.f), X, Y + BarH + 1.f, BarW, BarH);
	DrawRect(FLinearColor(0.2f, 0.45f, 0.95f, 1.f), X, Y + BarH + 1.f, BarW * SpFrac, BarH);
}
