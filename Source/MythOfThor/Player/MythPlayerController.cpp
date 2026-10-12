#include "Player/MythPlayerController.h"

#include "Player/MythPlayerCharacter.h"
#include "Network/RagnarokNetworkSubsystem.h"
#include "UI/MythHUD.h"
#include "World/MythMapInfo.h"
#include "World/MythNetWorld.h"
#include "World/MythGroundItem.h"
#include "World/MythDayNightActor.h"
#include "World/MythWeatherActor.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "Sprites/MythMonster.h"
#include "Sprites/MythPoring.h"
#include "Sprites/MythAura99.h"
#include "Sprites/MythRoSpriteComponent.h"
#include "Sprites/MythRoSound.h"
#include "Components/CapsuleComponent.h"
#include "UI/MythRoHUDWidget.h"
#include "Gameplay/MythStatsComponent.h"
#include "Gameplay/AutoBattleComponent.h"
#include "Blueprint/UserWidget.h"

#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "NavigationSystem.h"
#include "UnrealClient.h"
#include "TimerManager.h"
#include "GameFramework/PlayerInput.h"
#include "Sprites/MythSkillEffects.h"
#include "World/MythCellMoverComponent.h"
#include "World/MythWarpPortal.h"
#include "Sprites/MythStrEffect.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

AMythPlayerController::AMythPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
}

void AMythPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		// Atalhos de depuracao da Unreal (BaseInput.ini: F1-F5 viewmode, F9 print, Ctrl+Tab, Shift, ";" camera)
		// brigam com os do RO (F1-F9 da barra, Ctrl+Tab do minimapa). Fora do jogo final eles existem: desliga.
		if (PlayerInput)
		{
			PlayerInput->DebugExecBindings.Empty();
		}
		SetupRoCursors();
		if (URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr)
		{
			Net->OnPlayerMoveConfirmed.AddUniqueDynamic(this, &AMythPlayerController::HandleMoveConfirmed);
			Net->OnAttackOutOfRange.AddUniqueDynamic(this, &AMythPlayerController::HandleAttackOutOfRange);
		}
		HudWidget = CreateWidget<UMythRoHUDWidget>(this, UMythRoHUDWidget::StaticClass());
		if (HudWidget)
		{
			HudWidget->AddToViewport(0);
			HudWidget->BindToPawn(GetPawn());
		}
	}
}

void AMythPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);
	if (HudWidget)
	{
		HudWidget->BindToPawn(InPawn);
	}
}

FIntPoint AMythPlayerController::WorldToCell(const FVector& WorldLocation) const
{
	if (const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld()))
	{
		return Info->WorldToCell(WorldLocation);
	}
	const float Size = FMath::Max(CellSizeUU, 1.f);
	// O pacote do rAthena usa 10 bits por eixo (0..1023).
	return FIntPoint(
		FMath::Clamp(FMath::RoundToInt(WorldLocation.X / Size), 0, 1023),
		FMath::Clamp(FMath::RoundToInt(WorldLocation.Y / Size), 0, 1023));
}

AMythPlayerCharacter* AMythPlayerController::GetMythCharacter() const
{
	return Cast<AMythPlayerCharacter>(GetPawn());
}

void AMythPlayerController::BuildInputAssets()
{
	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Myth"));

	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = Type;
		return Action;
	};

	MoveAction         = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	ClickAction        = MakeAction(TEXT("IA_Click"), EInputActionValueType::Boolean);
	RotateMouseAction  = MakeAction(TEXT("IA_RotateMouse"), EInputActionValueType::Axis1D);
	PitchMouseAction   = MakeAction(TEXT("IA_PitchMouse"), EInputActionValueType::Axis1D);
	RotateKeyAction    = MakeAction(TEXT("IA_RotateKey"), EInputActionValueType::Axis1D);
	ZoomAction         = MakeAction(TEXT("IA_Zoom"), EInputActionValueType::Axis1D);
	ToggleCameraAction = MakeAction(TEXT("IA_ToggleCamera"), EInputActionValueType::Boolean);

	// WASD -> Axis2D (X = direita, Y = frente). W/S usam Swizzle YXZ para ir ao eixo Y.
	auto MapMove = [this](const FKey& Key, bool bSwizzle, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = MappingContext->MapKey(MoveAction, Key);
		if (bSwizzle)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(this);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(this));
		}
	};
	// Como no RO, o personagem anda so pelo mouse: o WASD fica desligado (as teclas ficam livres para atalhos).
	// Para religar, mapear W/S (swizzle, S negado) e D/A (A negado) com MapMove.
	(void)MapMove;

	MappingContext->MapKey(ClickAction, EKeys::LeftMouseButton);
	MappingContext->MapKey(RotateMouseAction, EKeys::MouseX);
	MappingContext->MapKey(PitchMouseAction, EKeys::MouseY);
	MappingContext->MapKey(ZoomAction, EKeys::MouseWheelAxis);
	MappingContext->MapKey(ToggleCameraAction, EKeys::C);

	MappingContext->MapKey(RotateKeyAction, EKeys::E);
	FEnhancedActionKeyMapping& RotateLeft = MappingContext->MapKey(RotateKeyAction, EKeys::Q);
	RotateLeft.Modifiers.Add(NewObject<UInputModifierNegate>(this));
}

void AMythPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	BuildInputAssets();

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->AddMappingContext(MappingContext, 0);
		}
	}

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EIC->BindAction(ClickAction, ETriggerEvent::Started, this, &AMythPlayerController::OnClickStarted);
		EIC->BindAction(ClickAction, ETriggerEvent::Triggered, this, &AMythPlayerController::OnClickTriggered);
		EIC->BindAction(MoveAction, ETriggerEvent::Started, this, &AMythPlayerController::OnMoveStarted);
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMythPlayerController::OnMove);
		EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &AMythPlayerController::OnMoveCompleted);
		EIC->BindAction(MoveAction, ETriggerEvent::Canceled, this, &AMythPlayerController::OnMoveCompleted);
		EIC->BindAction(RotateMouseAction, ETriggerEvent::Triggered, this, &AMythPlayerController::OnRotateMouse);
		EIC->BindAction(PitchMouseAction, ETriggerEvent::Triggered, this, &AMythPlayerController::OnPitchMouse);
		EIC->BindAction(RotateKeyAction, ETriggerEvent::Triggered, this, &AMythPlayerController::OnRotateKey);
		EIC->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AMythPlayerController::OnZoom);
		EIC->BindAction(ToggleCameraAction, ETriggerEvent::Started, this, &AMythPlayerController::OnToggleCamera);
	}

	// Atalhos de janela do cliente original (Alt+A atributos, Alt+E itens, Alt+Q equipamento, Alt+S habilidades,
	// Alt+V informacoes basicas, Ctrl+Tab minimapa, Esc fecha a ultima janela aberta)
	InputComponent->BindKey(FInputChord(EKeys::A, false, false, true, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyStatus);
	InputComponent->BindKey(FInputChord(EKeys::E, false, false, true, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyItems);
	InputComponent->BindKey(FInputChord(EKeys::Q, false, false, true, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyEquip);
	InputComponent->BindKey(FInputChord(EKeys::S, false, false, true, false), IE_Pressed, this, &AMythPlayerController::OnHotkeySkills);
	InputComponent->BindKey(FInputChord(EKeys::V, false, false, true, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyBasic);
	InputComponent->BindKey(FInputChord(EKeys::Z, false, false, true, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyParty);
	InputComponent->BindKey(FInputChord(EKeys::G, false, false, true, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyGuild);
	InputComponent->BindKey(FInputChord(EKeys::J, false, false, true, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyPet);
	InputComponent->BindKey(FInputChord(EKeys::L, false, false, true, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyEmotions);
	InputComponent->BindKey(FInputChord(EKeys::M, false, false, true, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyMacros);
	InputComponent->BindKey(FInputChord(EKeys::H, false, false, true, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyFriends);
	InputComponent->BindKey(FInputChord(EKeys::C, false, false, true, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyChatRoom);
	InputComponent->BindKey(FInputChord(EKeys::U, false, false, true, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyQuests);
	InputComponent->BindKey(EKeys::F12, IE_Pressed, this, &AMythPlayerController::OnHotkeyShortcuts);
	InputComponent->BindKey(FInputChord(EKeys::R, false, false, true, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyMail);
	InputComponent->BindKey(FInputChord(EKeys::T, false, false, true, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyHomunStandby);
	InputComponent->BindKey(FInputChord(EKeys::N, false, false, true, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyNavi);
	InputComponent->BindKey(FInputChord(EKeys::Tilde, false, true, false, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyWorldMap); // Ctrl+` como no cliente
	InputComponent->BindKey(FInputChord(EKeys::W, false, false, true, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyEquipSwitch);
	InputComponent->BindKey(FInputChord(EKeys::Y, false, false, true, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyAchievements);
	InputComponent->BindKey(FInputChord(EKeys::B, false, true, false, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyBank);
	InputComponent->BindKey(FInputChord(EKeys::Tab, false, true, false, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyMinimap);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AMythPlayerController::OnHotkeyEscape);
	InputComponent->BindKey(EKeys::Insert, IE_Pressed, this, &AMythPlayerController::OnHotkeySit);
	// F1..F9: atalhos da barra (itens e skills guardados no servidor)
	const FKey HotkeyKeys[9] = { EKeys::F1, EKeys::F2, EKeys::F3, EKeys::F4, EKeys::F5, EKeys::F6, EKeys::F7, EKeys::F8, EKeys::F9 };
	for (int32 i = 0; i < 9; ++i)
	{
		FInputKeyBinding Binding(FInputChord(HotkeyKeys[i]), IE_Pressed);
		Binding.KeyDelegate.GetDelegateForManualSet().BindWeakLambda(this, [this, i]() { UseHotkey((HudWidget ? HudWidget->HotbarActiveBase() : 0) + i); });
		InputComponent->KeyBindings.Add(Binding);
	}
}

// ---------------------------------------------------------------- Point & Click

void AMythPlayerController::OnClickStarted(const FInputActionValue&)
{
	bForceNextMove = true; // clique novo: pede mesmo que seja a mesma celula
	MoveToCursor();
	bForceNextMove = false;
}

void AMythPlayerController::OnClickTriggered(const FInputActionValue&)
{
	// Segurar o botao continua seguindo o cursor (comportamento classico do RO).
	// Nao dispara enquanto o botao direito gira a camera.
	if (!IsInputKeyDown(EKeys::RightMouseButton))
	{
		MoveToCursor();
	}
}

void AMythPlayerController::MoveToCursor()
{
	// Clique no titulo de uma sala de chat sobre a cabeca de alguem: entra nela (pede a senha se for privada)
	if (const AMythHUD* RoomHud = Cast<AMythHUD>(GetHUD()))
	{
		float MX = 0.f, MY = 0.f;
		int32 RoomType = 1;
		if (GetMousePosition(MX, MY))
		{
			if (const int32 RoomId = RoomHud->ChatRoomAt(FVector2D(MX, MY), RoomType))
			{
				if (HudWidget && bForceNextMove)
				{
					if (RoomType == 2) { if (URagnarokNetworkSubsystem* VNet = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>()) { VNet->SendVendingListReq(RoomId); } } // placa de loja
					else if (RoomType == 0) { HudWidget->AskChatRoomPassword(RoomId); }
					else if (URagnarokNetworkSubsystem* RNet = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>()) { RNet->SendEnterChatRoom(RoomId, FString()); }
				}
				return;
			}
		}
	}
	if (const AMythPlayerCharacter* Self = GetMythCharacter(); Self && Self->Sprite && Self->Sprite->GetState() == EMythSpriteState::Dead)
	{
		return; // morto: nao anda nem ataca (a janela de morte oferece voltar ao ponto de salvamento)
	}
	if (HudWidget && HudWidget->IsPointerOverUi())
	{
		return; // clique em janela ou atalho: nao anda nem ataca (o clique da interface e tratado la)
	}
	if (HudWidget && HudWidget->IsNpcDialogOpen())
	{
		return; // com dialogo de NPC aberto o clique e da janela, nao do mundo
	}
	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit) && Hit.bBlockingHit)
	{
		if (URagnarokNetworkSubsystem* CapNet = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>(); CapNet && CapNet->IsCapturing())
		{
			// Captura de mascote: o clique escolhe o monstro (fora dele, desiste)
			const AMythMonster* Mob = Cast<AMythMonster>(Hit.GetActor());
			CapNet->SendCatchPet(Mob && Mob->IsNetUnit() && Mob->NetObjectType == 5 ? static_cast<int32>(Mob->NetID) : 0);
			CurrentMouseCursor = EMouseCursor::Default;
			return;
		}
		if (PendingSkillID != 0 && HandleSkillTargetClick(Hit))
		{
			return; // clique de mira de skill
		}
		if (AMythMonster* Monster = Cast<AMythMonster>(Hit.GetActor()); Monster && Monster->ActorHasTag(TEXT("MythWarp")))
		{
			// Portal: clicar nele e andar ate a celula (o servidor teleporta ao pisar), nunca CZ_CONTACTNPC
			PendingPickup = nullptr;
			ClearFocusedTarget();
			MoveToLocation(Monster->GetActorLocation(), bForceNextMove);
			return;
		}
		if (AMythMonster* Monster = Cast<AMythMonster>(Hit.GetActor()))
		{
			if (!Monster->IsDead())
			{
				if (!bForceNextMove && (Monster->IsNpc() || FocusedTarget.Get() == Monster))
				{
					return; // botao segurado sobre o mesmo alvo/NPC: nao repete o pedido a cada quadro
				}
				SetFocusedTarget(Monster);
				return;
			}
		}
		if (AMythGroundItem* Item = Cast<AMythGroundItem>(Hit.GetActor()))
		{
			// Item no chao: anda ate ele e, perto o bastante, pede ao servidor para pegar (CZ_ITEM_PICKUP)
			if (!bForceNextMove)
			{
				return; // segurando o botao sobre o item: nao refaz o pedido
			}
			ClearFocusedTarget();
			PendingPickup = Item;
			PickupTries = 0;
			LastPickupTryTime = -1000.0;
			UpdatePickup(); // ja do lado: pega sem andar
			if (PendingPickup.IsValid() && PickupTries == 0)
			{
				MoveToLocation(Item->GetActorLocation(), /*bForceSend=*/true);
			}
			return;
		}
		if (IsShiftDown())
		{
			// Shift+clique: so vira para a celula, sem andar (docs/ref/01 3.7), e avisa o servidor (CZ_CHANGE_DIRECTION)
			if (bForceNextMove)
			{
				TurnToward(Hit.ImpactPoint);
			}
			return;
		}
		PendingPickup = nullptr;
		ClearFocusedTarget(); // clicar no chao cancela o alvo
		// Segurando o botao: so pede de novo quando a celula sob o cursor muda (nao inunda o servidor)
		MoveToLocation(Hit.ImpactPoint, bForceNextMove);
	}
}

void AMythPlayerController::MoveToLocation(const FVector& Destination, bool bForceSend)
{
	UWorld* World = GetWorld();
	const UNavigationSystemV1* Nav = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;

	// Conectado: como no cliente original, so pede a rota (CZ_REQUEST_MOVE) e anda quando o servidor confirmar
	// (ZC_NOTIFY_PLAYERMOVE -> AMythNetWorld). O rAthena recusa rotas acima de max_walk_path (17 celulas,
	// conf/battle/client.conf): cliques longe demais viram um passo de ate 14 celulas na mesma direcao.
	const URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr;
	if (Net && Net->GetNetworkState() == ERagnarokNetworkState::ConnectedMap && GetPawn())
	{
		FVector Dest = Destination;
		const FVector From = GetPawn()->GetActorLocation();
		const float MaxReach = 14.f * 50.f;
		if (FVector::Dist2D(From, Dest) > MaxReach)
		{
			const FVector Dir = (Dest - From).GetSafeNormal2D();
			Dest = From + Dir * MaxReach;
		}
		bHasDirectTarget = false;
		SendCellToServer(Dest, bForceSend);
		return;
	}

	if (Nav && Nav->GetDefaultNavDataInstance())
	{
		bHasDirectTarget = false;
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, Destination);
	}
	else
	{
		// Mapas montados pelo assembler ainda sem NavMesh: anda em linha reta ate o ponto.
		bHasDirectTarget = true;
		DirectTarget = Destination;
	}

	SendCellToServer(Destination, bForceSend);
}

// ------------------------------------------------------------------------- WASD

void AMythPlayerController::OnMoveStarted(const FInputActionValue&)
{
	// Teclado assume o controle imediatamente, cancelando o movimento por clique e o alvo.
	ClearFocusedTarget();
	StopMovement();
	bHasDirectTarget = false;
	bWasdActive = true;
	TimeSinceWasdSend = WasdSendInterval; // envia ja no primeiro frame
}

void AMythPlayerController::OnMove(const FInputActionValue& Value)
{
	if (IsAltDown())
	{
		return; // Alt+A/S e atalho de janela, nao movimento
	}
	AMythPlayerCharacter* Char = GetMythCharacter();
	if (!Char)
	{
		return;
	}

	const FVector2D Input = Value.Get<FVector2D>();
	const FRotator YawRot(0.f, Char->GetCameraYaw(), 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	// Conectado: o WASD so escolhe a direcao; quem anda e o servidor (PlayerTick pede o destino a frente)
	const URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr;
	if (Net && Net->GetNetworkState() == ERagnarokNetworkState::ConnectedMap)
	{
		WasdDirection = (Forward * Input.Y + Right * Input.X).GetSafeNormal2D();
		return;
	}
	Char->AddMovementInput(Forward, Input.Y);
	Char->AddMovementInput(Right, Input.X);
}

void AMythPlayerController::OnMoveCompleted(const FInputActionValue&)
{
	bWasdActive = false;
	WasdDirection = FVector::ZeroVector;
	AMythPlayerCharacter* Char = GetMythCharacter();
	if (!Char)
	{
		return;
	}
	// Conectado: soltar nao manda nada. O pedido em curso e de 1 celula a frente, entao o servidor termina o
	// passo e para; pedir a celula atual fazia ele voltar para ela (o "anda e volta", confirmado no rAthena)
	bHasPendingMove = false;
	const URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr;
	if (!(Net && Net->GetNetworkState() == ERagnarokNetworkState::ConnectedMap))
	{
		SendCellToServer(Char->GetActorLocation(), /*bForce=*/true);
	}
}

void AMythPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	AMythPlayerCharacter* Char = GetMythCharacter();
	if (!Char)
	{
		return;
	}

	UpdateCombat(DeltaTime);
	UpdatePickup();
	UpdateHomunAI(DeltaTime);
	if (PendingSkillID != 0 && WasInputKeyJustPressed(EKeys::RightMouseButton))
	{
		CancelSkillTargeting(); // botao direito cancela a mira, como no cliente original
	}
	if (WasInputKeyJustPressed(EKeys::RightMouseButton))
	{
		if (URagnarokNetworkSubsystem* CapNet = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>(); CapNet && CapNet->IsCapturing())
		{
			CapNet->SendCatchPet(0); // botao direito desiste da captura
			CurrentMouseCursor = EMouseCursor::Default;
		}
	}
	// Botao direito rapido (sem arrastar a camera) sobre outro jogador: menu Trocar / Convidar / Cochichar
	if (WasInputKeyJustPressed(EKeys::RightMouseButton))
	{
		RightClickHeldSince = GetWorld()->GetTimeSeconds();
	}
	if (WasInputKeyJustReleased(EKeys::RightMouseButton) && GetWorld()->GetTimeSeconds() - RightClickHeldSince < 0.25 && PendingSkillID == 0
		&& HudWidget && !HudWidget->IsPointerOverUi())
	{
		FHitResult RightHit;
		if (GetHitResultUnderCursor(ECC_Visibility, false, RightHit) && RightHit.bBlockingHit)
		{
			const bool bHomunCommand = (IsInputKeyDown(EKeys::LeftAlt) || IsInputKeyDown(EKeys::RightAlt)) && HandleHomunCommandClick(RightHit);
			const URagnarokNetworkSubsystem* RNet = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr;
			if (const AMythMonster* Other = Cast<AMythMonster>(RightHit.GetActor()); !bHomunCommand && Other && RNet && Other->IsNetUnit() && Other->NetObjectType == 0 && Other->NetID != RNet->GetSelfID())
			{
				HudWidget->ShowPlayerMenu(Other->NetID, Other->GetDisplayName(), FSlateApplication::Get().GetCursorPos());
			}
		}
	}
	UpdateCameraReset();
	// Enter: com dialogo de NPC aberto avanca/escolhe/fecha; senao abre o chat (como no cliente original).
	if (HudWidget && WasInputKeyJustPressed(EKeys::Enter) && !HudWidget->IsChatFocused() && !HudWidget->NpcDialogConfirm())
	{
		HudWidget->FocusChat();
	}
	// Dialogo de NPC (docs/ref/06 3.9): Espaco tambem avanca; no menu, 1..9 escolhem e as setas movem a selecao
	if (HudWidget && HudWidget->IsNpcDialogOpen() && !HudWidget->IsChatFocused())
	{
		if (WasInputKeyJustPressed(EKeys::SpaceBar))
		{
			HudWidget->NpcDialogConfirm();
		}
		const FKey Digits[9] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
		const FKey NumPad[9] = { EKeys::NumPadOne, EKeys::NumPadTwo, EKeys::NumPadThree, EKeys::NumPadFour, EKeys::NumPadFive, EKeys::NumPadSix, EKeys::NumPadSeven, EKeys::NumPadEight, EKeys::NumPadNine };
		for (int32 i = 0; i < 9; ++i)
		{
			if (WasInputKeyJustPressed(Digits[i]) || WasInputKeyJustPressed(NumPad[i]))
			{
				HudWidget->NpcMenuChoose(i);
			}
		}
		if (WasInputKeyJustPressed(EKeys::Up)) { HudWidget->NpcMenuMove(-1); }
		if (WasInputKeyJustPressed(EKeys::Down)) { HudWidget->NpcMenuMove(1); }
	}

	UpdateHeadTurn();
	// Alt+1..9 e Alt+0: emocoes /e1../e10 (docs/ref/18 4)
	if (HudWidget && IsAltDown() && !HudWidget->IsChatFocused())
	{
		static const FKey Digits[10] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine, EKeys::Zero };
		for (int32 i = 0; i < 10; ++i)
		{
			if (WasInputKeyJustPressed(Digits[i]))
			{
				HudWidget->SubmitChat(FString::Printf(TEXT("/e%d"), i + 1));
			}
		}
	}
	if (bAutoDoridori)
	{
		DoridoriTimer -= DeltaTime;
		const AMythPlayerCharacter* DoriChar = GetMythCharacter();
		if (DoriChar && DoriChar->Sprite && (DoriChar->Sprite->GetState() == EMythSpriteState::Idle || DoriChar->Sprite->GetState() == EMythSpriteState::Sit))
		{
			if (DoridoriTimer <= 0.f)
			{
				DoridoriTimer = 0.2f;
				SetHeadTurn(DoriChar->Sprite->GetHeadDir() == 1 ? 2 : 1);
			}
		}
	}
	FlushPendingMove();
	UpdateNetChase(DeltaTime);
	UpdateRoCursor();
	// Como o cliente, quem "ouve" e o personagem (a camera fica a ~50 m e deixaria tudo mudo); a orientacao segue a
	// camera para o som da esquerda sair na esquerda da tela
	SetAudioListenerOverride(Char->GetRootComponent(), FVector::ZeroVector, FRotator(0.f, Char->GetCameraYaw(), 0.f));

	if (bWasdActive)
	{
		const URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr;
		const bool bOnline = Net && Net->GetNetworkState() == ERagnarokNetworkState::ConnectedMap;
		if (bOnline)
		{
			// Pede so 1 celula alem do passo em andamento, renovando a cada passo: o servidor troca de rota no fim
			// do passo (change_walk_target) e, ao soltar, termina o passo e para sem nenhum pacote (coord 003)
			const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld());
			if (!WasdDirection.IsNearlyZero() && Info && Char->Mover)
			{
				const FIntPoint Base = Char->Mover->GetNextCell();
				const float MaxAxis = FMath::Max(FMath::Abs(WasdDirection.X), FMath::Abs(WasdDirection.Y));
				const FVector Step = WasdDirection / MaxAxis * CellSizeUU; // diagonal = 1 celula em cada eixo
				const FIntPoint Target = Info->WorldToCell(Info->CellToWorld(Base, 0.f) + Step);
				if (Target != LastSentCell && Target != Base)
				{
					PendingMoveCell = Target;
					bHasPendingMove = true;
					FlushPendingMove();
				}
			}
		}
		else
		{
			TimeSinceWasdSend += DeltaTime;
			const FVector Dir = Char->GetVelocity().GetSafeNormal2D();
			if (TimeSinceWasdSend >= WasdSendInterval && !Dir.IsNearlyZero())
			{
				TimeSinceWasdSend = 0.f;
				SendCellToServer(Char->GetActorLocation() + Dir * (WasdLookaheadCells * CellSizeUU), /*bForce=*/false);
			}
		}
	}
	else if (bHasDirectTarget)
	{
		const FVector ToTarget = DirectTarget - Char->GetActorLocation();
		if (ToTarget.Size2D() < 15.f)
		{
			bHasDirectTarget = false;
		}
		else
		{
			Char->AddMovementInput(ToTarget.GetSafeNormal2D(), 1.f);
		}
	}
}

// ----------------------------------------------------------------------- Camera

bool AMythPlayerController::IsShiftDown() const
{
	return IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
}

bool AMythPlayerController::IsAltDown() const
{
	return IsInputKeyDown(EKeys::LeftAlt) || IsInputKeyDown(EKeys::RightAlt);
}

void AMythPlayerController::ToggleHudWindow(EMythRoWindow Window)
{
	if (HudWidget)
	{
		HudWidget->ToggleWindow(Window);
	}
}

void AMythPlayerController::OnHotkeyEmotions() { ToggleHudWindow(EMythRoWindow::Emotions); }
void AMythPlayerController::OnHotkeyAchievements() { ToggleHudWindow(EMythRoWindow::Achievements); }
void AMythPlayerController::OnHotkeyEquipSwitch() { ToggleHudWindow(EMythRoWindow::EquipSwitch); }
void AMythPlayerController::OnHotkeyNavi() { ToggleHudWindow(EMythRoWindow::Navi); }
void AMythPlayerController::OnHotkeyWorldMap() { ToggleHudWindow(EMythRoWindow::WorldMap); }
void AMythPlayerController::OnHotkeyMail()
{
	// Alt+R e a janela do homunculo no cliente classico; sem homunculo chamado fica o correio
	const URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr;
	ToggleHudWindow(Net && Net->HasHomun() ? EMythRoWindow::Homun : EMythRoWindow::Mail);
}
void AMythPlayerController::OnHotkeyBank() { ToggleHudWindow(EMythRoWindow::Bank); }
void AMythPlayerController::OnHotkeyShortcuts() { ToggleHudWindow(EMythRoWindow::Shortcuts); }
void AMythPlayerController::OnHotkeyQuests() { ToggleHudWindow(EMythRoWindow::Quests); }
void AMythPlayerController::OnHotkeyChatRoom() { ToggleHudWindow(EMythRoWindow::ChatRoom); }
void AMythPlayerController::OnHotkeyFriends() { ToggleHudWindow(EMythRoWindow::Friends); }
void AMythPlayerController::OnHotkeyMacros() { ToggleHudWindow(EMythRoWindow::Macros); }
void AMythPlayerController::OnHotkeyParty() { ToggleHudWindow(EMythRoWindow::Party); }
void AMythPlayerController::OnHotkeyGuild() { ToggleHudWindow(EMythRoWindow::Guild); }
void AMythPlayerController::OnHotkeyPet() { ToggleHudWindow(EMythRoWindow::Pet); }

void AMythPlayerController::BeginCapture()
{
	CurrentMouseCursor = EMouseCursor::Crosshairs;
	if (HudWidget)
	{
		HudWidget->AddChatLine(TEXT("Clique no monstro que deseja capturar (botão direito desiste)."), FLinearColor(1.f, 0.85f, 0.4f));
	}
}

void AMythPlayerController::MythCaptureNearest()
{
	URagnarokNetworkSubsystem* Net = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>();
	if (!Net || !Net->IsCapturing() || !GetPawn())
	{
		return;
	}
	const AMythMonster* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();
	for (TActorIterator<AMythMonster> It(GetWorld()); It; ++It)
	{
		const float D = FVector::DistSquared(It->GetActorLocation(), GetPawn()->GetActorLocation());
		if (It->IsNetUnit() && It->NetObjectType == 5 && !It->IsDead() && D < BestDist)
		{
			BestDist = D;
			Best = *It;
		}
	}
	Net->SendCatchPet(Best ? static_cast<int32>(Best->NetID) : 0);
	CurrentMouseCursor = EMouseCursor::Default;
}
void AMythPlayerController::OnHotkeyStatus() { ToggleHudWindow(EMythRoWindow::Status); }
void AMythPlayerController::OnHotkeyItems() { ToggleHudWindow(EMythRoWindow::Items); }
void AMythPlayerController::OnHotkeyEquip() { ToggleHudWindow(EMythRoWindow::Equip); }
void AMythPlayerController::OnHotkeySkills() { ToggleHudWindow(EMythRoWindow::Skills); }
void AMythPlayerController::OnHotkeyBasic() { ToggleHudWindow(EMythRoWindow::Basic); }
void AMythPlayerController::OnHotkeyMinimap() { ToggleHudWindow(EMythRoWindow::Minimap); }

void AMythPlayerController::UpdateHeadTurn()
{
	// Doridori (docs/ref/03 4.3): parado ou sentado, <- e -> viram a cabeca (CZ_CHANGE_DIRECTION com head dir 2 e 1);
	// o corpo fica onde esta. Andar, bater e apanhar endireitam a cabeca (SetState do sprite).
	const bool bLeft = WasInputKeyJustPressed(EKeys::Left);
	const bool bRight = WasInputKeyJustPressed(EKeys::Right);
	if ((!bLeft && !bRight) || IsAltDown() || (HudWidget && (HudWidget->IsChatFocused() || HudWidget->IsNpcDialogOpen())))
	{
		return;
	}
	AMythPlayerCharacter* Char = GetMythCharacter();
	UMythRoSpriteComponent* Sprite = Char ? Char->Sprite.Get() : nullptr;
	if (!Sprite || (Sprite->GetState() != EMythSpriteState::Idle && Sprite->GetState() != EMythSpriteState::Sit))
	{
		return;
	}
	SetHeadTurn(bLeft ? 2 : 1);
}

void AMythPlayerController::SetHeadTurn(uint8 HeadDir)
{
	AMythPlayerCharacter* Char = GetMythCharacter();
	if (!Char || !Char->Sprite)
	{
		return;
	}
	Char->Sprite->SetHeadDir(HeadDir);
	if (URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr)
	{
		Net->SendChangeDirection(HeadDir, Char->Sprite->GetFacingRoDir());
	}
}

void AMythPlayerController::OnHotkeySit()
{
	// Insert: senta/levanta como no cliente (CZ_REQUEST_ACT 2/3); quem muda o sprite e a resposta do servidor.
	// Sentado o rAthena recupera HP/SP mais rapido
	const AMythPlayerCharacter* Char = GetMythCharacter();
	URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr;
	if (Char && Char->Sprite && Net && (!HudWidget || !HudWidget->IsChatFocused()))
	{
		StopMovement();
		Net->SendActionRequest(0, Char->Sprite->IsSitting() ? 3 : 2);
	}
}

void AMythPlayerController::OnHotkeyEscape()
{
	if (PendingSkillID != 0)
	{
		CancelSkillTargeting(); // Esc cancela a mira antes de fechar janelas
		return;
	}
	// Como no cliente original: Esc fecha a janela de cima; sem janela aberta, abre o menu do jogo
	if (HudWidget && !HudWidget->CloseTopWindow())
	{
		HudWidget->ToggleGameMenu();
	}
}

void AMythPlayerController::OnRotateMouse(const FInputActionValue& Value)
{
	// Botao direito + arrastar gira; com Shift, o arrasto inclina (OnPitchMouse), como no cliente original.
	if (IsInputKeyDown(EKeys::RightMouseButton) && !IsShiftDown())
	{
		if (AMythPlayerCharacter* Char = GetMythCharacter())
		{
			Char->RotateCamera(Value.Get<float>() * MouseRotateSensitivity); // 0,4 grau por pixel (View.cpp)
		}
	}
}

void AMythPlayerController::OnPitchMouse(const FInputActionValue& Value)
{
	if (IsInputKeyDown(EKeys::RightMouseButton) && IsShiftDown())
	{
		if (AMythPlayerCharacter* Char = GetMythCharacter())
		{
			// Como o cliente (-0,3 grau por pixel com Y da tela para baixo): arrastar para baixo deixa a camera mais
			// de cima; para cima, mais deitada. O MouseY da Unreal e positivo para cima.
			Char->PitchCamera(Value.Get<float>() * 0.3f);
		}
	}
}

void AMythPlayerController::UpdateCameraReset()
{
	if (!WasInputKeyJustPressed(EKeys::RightMouseButton))
	{
		return;
	}
	const double Now = GetWorld()->GetRealTimeSeconds();
	if (Now - LastRightClickTime <= DoubleClickSeconds)
	{
		if (AMythPlayerCharacter* Char = GetMythCharacter())
		{
			Char->ResetCamera();
		}
		LastRightClickTime = -1.0;
		return;
	}
	LastRightClickTime = Now;
}

void AMythPlayerController::OnRotateKey(const FInputActionValue& Value)
{
	if (IsAltDown())
	{
		return; // Alt+Q/E abrem janelas
	}
	if (AMythPlayerCharacter* Char = GetMythCharacter())
	{
		Char->RotateCamera(Value.Get<float>() * KeyRotateSpeed * GetWorld()->GetDeltaSeconds());
	}
}

void AMythPlayerController::OnZoom(const FInputActionValue& Value)
{
	if (AMythPlayerCharacter* Char = GetMythCharacter())
	{
		Char->ZoomCamera(Value.Get<float>());
	}
}

void AMythPlayerController::OnToggleCamera(const FInputActionValue&)
{
	if (AMythPlayerCharacter* Char = GetMythCharacter())
	{
		Char->ToggleCameraMode();
	}
}

// ---------------------------------------------------------------------- Network

void AMythPlayerController::SendCellToServer(const FVector& WorldLocation, bool bForce)
{
	const FIntPoint Cell = WorldToCell(WorldLocation);
	// Segurando o botao / WASD: destino a ate 2 celulas do ultimo pedido mantem o movimento atual
	if (!bForce && FMath::Max(FMath::Abs(Cell.X - LastSentCell.X), FMath::Abs(Cell.Y - LastSentCell.Y)) <= 2)
	{
		return;
	}
	PendingMoveCell = Cell;
	bHasPendingMove = true;
	FlushPendingMove();
}

void AMythPlayerController::HandleMoveConfirmed(int32 TargetX, int32 TargetY)
{
	bAwaitingMoveAck = false;
	FlushPendingMove();
}

void AMythPlayerController::FlushPendingMove()
{
	if (!bHasPendingMove)
	{
		return;
	}
	const double Now = FPlatformTime::Seconds();
	if (Now - LastMoveSendTime < 0.075 || (bAwaitingMoveAck && Now - LastMoveSendTime < 1.0))
	{
		return; // sai no PlayerTick, quando o intervalo passar ou a confirmacao chegar
	}
	const FIntPoint Cell = PendingMoveCell;
	bHasPendingMove = false;
	LastSentCell = Cell;
	LastMoveSendTime = Now;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (URagnarokNetworkSubsystem* Net = GI->GetSubsystem<URagnarokNetworkSubsystem>())
		{
			// Retorna false se nao houver sessao; o jogo segue jogavel offline para testes.
			const bool bSent = Net->SendMoveRequest(Cell.X, Cell.Y);
			// O rAthena so responde quando aceita a rota: recusada, o timeout de 1 s libera o proximo pedido
			bAwaitingMoveAck = bSent;
			UE_LOG(LogTemp, Log, TEXT("MythNet: CZ_REQUEST_MOVE (%d, %d) %s"), Cell.X, Cell.Y, bSent ? TEXT("enviado") : TEXT("offline"));
		}
	}
}

void AMythPlayerController::MythTestDamage()
{
	if (APawn* P = GetPawn())
	{
		const FVector Loc = P->GetActorLocation() + FVector(0, 0, 90);
		UMythCombatText::ShowDamage(this, Loc, 137, ECombatTextType::DamageDealt);
		UMythCombatText::ShowDamage(this, Loc, 482, ECombatTextType::Critical);
		UMythCombatText::ShowDamage(this, Loc, 55, ECombatTextType::DamageTaken);
		UMythCombatText::ShowDamage(this, Loc, 210, ECombatTextType::Heal);
		UMythCombatText::ShowMiss(this, Loc);
	}
}

void AMythPlayerController::MythTestHud()
{
	APawn* P = GetPawn();
	UMythStatsComponent* Stats = P ? P->FindComponentByClass<UMythStatsComponent>() : nullptr;
	UAutoBattleComponent* Auto = P ? P->FindComponentByClass<UAutoBattleComponent>() : nullptr;
	if (Stats)
	{
		Stats->SetIdentity(TEXT("Thor"), 12);
		Stats->SetVitals(312, 480, 41, 95);
		Stats->SetExp(4370, 9800);
	}
	if (Auto)
	{
		// alterna entre peso normal e sobrepeso a cada chamada
		Auto->UpdateInventoryWeight(Auto->IsOverweight() ? 40.f : 92.f);
	}
}

// --------------------------------------------------------------------- Targeting

AMythMonster* AMythPlayerController::GetFocusedTarget() const
{
	return FocusedTarget.Get();
}

void AMythPlayerController::SetFocusedTarget(AMythMonster* NewTarget)
{
	if (FocusedTarget.Get() == NewTarget && !(NewTarget && NewTarget->IsNetUnit()))
	{
		return; // re-clicar no mesmo alvo nao reinicia a recarga do ataque (com servidor, repete o pedido)
	}
	if (NewTarget && NewTarget->IsNetUnit())
	{
		// Com servidor: NPC abre a conversa (CZ_CONTACTNPC); monstro recebe o ataque continuo (CZ_REQUEST_ACT tipo 7).
		// O rAthena faz o personagem andar ate o alvo e bater; o dano volta pelo ZC_NOTIFY_ACT.
		if (URagnarokNetworkSubsystem* Net = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>())
		{
			if (NewTarget->IsNpc())
			{
				Net->ContactNpc(NewTarget->NetID);
				return;
			}
			if (!NewTarget->IsAttackable())
			{
				return;
			}
			UE_LOG(LogTemp, Log, TEXT("MythNet: atacando %s (%lld)"), *NewTarget->MonsterName, NewTarget->NetID);
			Net->SendActionRequest(NewTarget->NetID, bContinuousAttack ? 7 : 0);
		}
	}
	FocusedTarget = NewTarget;
	ChaseRepathTimer = 0.f;
}

void AMythPlayerController::TurnToward(const FVector& WorldLocation)
{
	AMythPlayerCharacter* Char = GetMythCharacter();
	if (!Char)
	{
		return;
	}
	if (Char->Sprite)
	{
		Char->Sprite->FaceLocation(WorldLocation);
	}
	// Direcao do rAthena pela diferenca de celulas (Y do grid cresce para o norte): 0 N, 1 NO, 2 O, 3 SO, 4 S, 5 SE, 6 L, 7 NE
	const FIntPoint From = WorldToCell(Char->GetActorLocation());
	const FIntPoint To = WorldToCell(WorldLocation);
	if (From == To)
	{
		return;
	}
	const float Angle = FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(-(To.X - From.X)), static_cast<float>(To.Y - From.Y)));
	const uint8 Dir = static_cast<uint8>((FMath::RoundToInt(Angle / 45.f) + 8) % 8);
	if (URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr)
	{
		Net->SendChangeDirection(0, Dir);
	}
}

void AMythPlayerController::SetupRoCursors()
{
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	FString Text;
	if (!Viewport || !FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("cursors.json"))))
	{
		return;
	}
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
	{
		return;
	}
	// Formas do Slate usadas como "vaga" de cada cursor do RO (acao do cursors.act)
	const TPair<EMouseCursor::Type, int32> Slots[] = {
		{ EMouseCursor::Default, 0 },          // seta
		{ EMouseCursor::GrabHand, 1 },         // falar (NPC)
		{ EMouseCursor::ResizeLeftRight, 4 },  // girar a camera
		{ EMouseCursor::GrabHandClosed, 5 },   // atacar
		{ EMouseCursor::CardinalCross, 7 },    // portal
		{ EMouseCursor::SlashedCircle, 8 },    // proibido (celula bloqueada)
		{ EMouseCursor::Hand, 9 },             // pegar item
		{ EMouseCursor::Crosshairs, 10 },      // mira de skill
	};
	for (const TPair<EMouseCursor::Type, int32>& Slot : Slots)
	{
		const TSharedPtr<FJsonObject>* Entry;
		if (!Root->TryGetObjectField(FString::FromInt(Slot.Value), Entry))
		{
			continue;
		}
		const TArray<TSharedPtr<FJsonValue>>& Hot = (*Entry)->GetArrayField(TEXT("hotspot"));
		const TArray<TSharedPtr<FJsonValue>>& Size = (*Entry)->GetArrayField(TEXT("size"));
		if (Hot.Num() < 2 || Size.Num() < 2)
		{
			continue;
		}
		// Ponto quente normalizado (0..1) na imagem
		const FVector2D HotSpot(Hot[0]->AsNumber() / FMath::Max(1.0, Size[0]->AsNumber()), Hot[1]->AsNumber() / FMath::Max(1.0, Size[1]->AsNumber()));
		Viewport->SetHardwareCursor(Slot.Key, FName(*(*Entry)->GetStringField(TEXT("png"))), HotSpot);
	}
}

bool AMythPlayerController::IsWarpCell(const FIntPoint& Cell) const
{
	for (TActorIterator<AMythMonster> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("MythWarp")))
		{
			const FIntPoint WarpCell = WorldToCell(It->GetActorLocation());
			if (FMath::Max(FMath::Abs(WarpCell.X - Cell.X), FMath::Abs(WarpCell.Y - Cell.Y)) <= 1)
			{
				return true;
			}
		}
	}
	return false;
}

bool AMythPlayerController::IsCursorOverUi() const
{
	// Mouse sobre uma janela da HUD (o ultimo widget sob o mouse nao e o viewport do jogo)
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}
	FSlateApplication& App = FSlateApplication::Get();
	const FWidgetPath Path = App.LocateWindowUnderMouse(App.GetCursorPos(), App.GetInteractiveTopLevelWindows());
	return Path.IsValid() && Path.Widgets.Num() > 0 && Path.Widgets.Last().Widget->GetType() != FName(TEXT("SViewport"));
}

void AMythPlayerController::UpdateRoCursor()
{
	EMouseCursor::Type Want = EMouseCursor::Default;
	if (IsInputKeyDown(EKeys::RightMouseButton))
	{
		Want = EMouseCursor::ResizeLeftRight;
	}
	else if (IsCursorOverUi())
	{
		Want = EMouseCursor::Default;
	}
	else if (PendingSkillID != 0)
	{
		Want = EMouseCursor::Crosshairs;
	}
	else
	{
		FHitResult Hit;
		if (GetHitResultUnderCursor(ECC_Visibility, false, Hit) && Hit.bBlockingHit)
		{
			AActor* A = Hit.GetActor();
			if (AMythMonster* M = Cast<AMythMonster>(A); M && !M->IsDead())
			{
				Want = M->ActorHasTag(TEXT("MythWarp")) ? EMouseCursor::CardinalCross : (M->IsNpc() ? EMouseCursor::GrabHand : EMouseCursor::GrabHandClosed);
			}
			else if (A && A->IsA<AMythGroundItem>())
			{
				Want = EMouseCursor::Hand;
			}
			else if (A && A->IsA<AMythWarpPortal>())
			{
				Want = EMouseCursor::CardinalCross;
			}
			else
			{
				// Portal: o NPC do warp (45 com aneis, 139 escondido nas portas) nao tem sprite para o raio acertar;
				// como o cliente (HasWarpAtAttrCell), vale a celula
				const FIntPoint Cell = WorldToCell(Hit.ImpactPoint);
				if (IsWarpCell(Cell))
				{
					Want = EMouseCursor::CardinalCross;
				}
				if (Want == EMouseCursor::Default && !UMythCellMoverComponent::IsWalkable(GetWorld(), Cell))
				{
					Want = EMouseCursor::SlashedCircle;
				}
			}
		}
		else
		{
			Want = EMouseCursor::SlashedCircle;
		}
	}
	if (Want != CurrentMouseCursor)
	{
		UE_LOG(LogTemp, Verbose, TEXT("MythCursor: %d"), static_cast<int32>(Want));
	}
	CurrentMouseCursor = Want;
}

void AMythPlayerController::MythTurn(int32 X, int32 Y)
{
	if (const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld()))
	{
		UE_LOG(LogTemp, Log, TEXT("MythTurn: virando para (%d, %d)"), X, Y);
		TurnToward(Info->CellToWorld(FIntPoint(X, Y), 0.f));
	}
}

void AMythPlayerController::MythPointAt(int32 X, int32 Y)
{
	// Teste: poe o mouse sobre a celula e diz qual cursor do RO ficou
	const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld());
	const APawn* P = GetPawn();
	FVector2D Screen;
	if (Info && P && ProjectWorldLocationToScreen(Info->CellToWorld(FIntPoint(X, Y), P->GetActorLocation().Z - 60.f), Screen))
	{
		SetMouseLocation(FMath::RoundToInt(Screen.X), FMath::RoundToInt(Screen.Y));
		GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, X, Y]()
		{
			UpdateRoCursor();
			const TCHAR* Name = TEXT("?");
			switch (CurrentMouseCursor.GetValue())
			{
			case EMouseCursor::Default: Name = TEXT("seta"); break;
			case EMouseCursor::GrabHand: Name = TEXT("falar"); break;
			case EMouseCursor::GrabHandClosed: Name = TEXT("atacar"); break;
			case EMouseCursor::Hand: Name = TEXT("pegar item"); break;
			case EMouseCursor::CardinalCross: Name = TEXT("portal"); break;
			case EMouseCursor::SlashedCircle: Name = TEXT("proibido"); break;
			case EMouseCursor::Crosshairs: Name = TEXT("mira de skill"); break;
			case EMouseCursor::ResizeLeftRight: Name = TEXT("girar camera"); break;
			default: break;
			}
			FHitResult Hit;
			const bool bHit = GetHitResultUnderCursor(ECC_Visibility, false, Hit) && Hit.bBlockingHit;
			FString Warps;
			for (TActorIterator<AMythMonster> It(GetWorld()); It; ++It)
			{
				if (It->ActorHasTag(TEXT("MythWarp")))
				{
					const FIntPoint C = WorldToCell(It->GetActorLocation());
					if (FMath::Abs(C.X - X) <= 8 && FMath::Abs(C.Y - Y) <= 8)
					{
						Warps += FString::Printf(TEXT(" (%d,%d)"), C.X, C.Y);
					}
				}
			}
			UE_LOG(LogTemp, Log, TEXT("MythPointAt (%d, %d): cursor %s | raio em %s celula %s | portais perto:%s"), X, Y, Name,
				bHit && Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("nada"),
				bHit ? *WorldToCell(Hit.ImpactPoint).ToString() : TEXT("-"), *Warps);
		}));
	}
}

void AMythPlayerController::ClearFocusedTarget()
{
	FocusedTarget = nullptr;
	ChaseTargetID = 0;
}

void AMythPlayerController::HandleAttackOutOfRange(int64 TargetID, int32 TargetX, int32 TargetY, int32 Range)
{
	// Como o cliente original: o servidor disse que o alvo esta longe; anda ate ele e, no alcance, pede o ataque
	// de novo (o UpdateNetChase cuida disso)
	const AMythMonster* Target = FocusedTarget.Get();
	if (!Target || Target->NetID != TargetID)
	{
		return;
	}
	ChaseTargetID = TargetID;
	ChaseRange = FMath::Max(Range, 1);
	ChaseRepathTimer = 1.2f;
	SendChaseMove(FIntPoint(TargetX, TargetY));
}

void AMythPlayerController::SendChaseMove(const FIntPoint& TargetCell)
{
	// Celula de aproximacao (docs/ref/04 4.2): na linha do alvo ate o jogador, a ChaseRange celulas do alvo
	// (corpo a corpo = a celula vizinha do lado do jogador; arco = o anel do alcance mais perto do jogador)
	const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld());
	const APawn* P = GetPawn();
	if (!Info || !P)
	{
		return;
	}
	const FIntPoint Self = WorldToCell(P->GetActorLocation());
	const FIntPoint D = Self - TargetCell;
	const int32 Dist = FMath::Max(FMath::Abs(D.X), FMath::Abs(D.Y));
	FIntPoint Goal = TargetCell;
	if (Dist > 0)
	{
		const float Scale = static_cast<float>(FMath::Min(ChaseRange, Dist)) / Dist;
		Goal = TargetCell + FIntPoint(FMath::RoundToInt(D.X * Scale), FMath::RoundToInt(D.Y * Scale));
	}
	if (!UMythCellMoverComponent::IsWalkable(GetWorld(), Goal))
	{
		Goal = TargetCell;
	}
	SendCellToServer(Info->CellToWorld(Goal, 0.f), /*bForce=*/true);
}

void AMythPlayerController::UpdateNetChase(float DeltaTime)
{
	AMythMonster* Target = FocusedTarget.Get();
	const APawn* P = GetPawn();
	if (ChaseTargetID == 0 || !Target || !P || Target->NetID != ChaseTargetID || Target->IsDead())
	{
		ChaseTargetID = 0;
		return;
	}
	const FIntPoint Self = WorldToCell(P->GetActorLocation());
	const FIntPoint Goal = WorldToCell(Target->GetActorLocation());
	if (FMath::Max(FMath::Abs(Self.X - Goal.X), FMath::Abs(Self.Y - Goal.Y)) <= ChaseRange)
	{
		// No alcance: retoma o ataque continuo (CZ_REQUEST_ACT 7); o servidor para o personagem e bate
		ChaseTargetID = 0;
		if (URagnarokNetworkSubsystem* Net = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>())
		{
			Net->SendActionRequest(Target->NetID, 7);
		}
		return;
	}
	// O alvo anda: refaz a aproximacao a cada 1,2 s (kAttackChaseRequestIntervalMs do cliente)
	ChaseRepathTimer -= DeltaTime;
	if (ChaseRepathTimer <= 0.f)
	{
		ChaseRepathTimer = 1.2f;
		SendChaseMove(Goal);
	}
}

void AMythPlayerController::UpdatePickup()
{
	AMythGroundItem* Item = PendingPickup.Get();
	const APawn* P = GetPawn();
	if (!Item || !P)
	{
		return;
	}
	// Como o cliente original (docs/ref/04 5): pede ao ficar a ate 1 celula (Chebyshev) e repete a cada 400 ms
	// ate o item sumir do chao (ZC_ITEM_DISAPPEAR destroi o ator e o ponteiro fraco zera)
	const FIntPoint Self = WorldToCell(P->GetActorLocation());
	const FIntPoint At = WorldToCell(Item->GetActorLocation());
	if (FMath::Max(FMath::Abs(Self.X - At.X), FMath::Abs(Self.Y - At.Y)) > 1)
	{
		return;
	}
	const double Now = FPlatformTime::Seconds();
	if (Now - LastPickupTryTime < 0.4)
	{
		return;
	}
	if (LastPickupTryTime > 0.0 && Now - LastPickupTryTime > 3.0)
	{
		LastPickupTryTime = -1000.0; // tentativa velha de outro item
	}
	if (URagnarokNetworkSubsystem* Net = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>())
	{
		Net->PickUpItem(Item->NetID);
	}
	LastPickupTryTime = Now;
	if (++PickupTries >= 3)
	{
		PendingPickup = nullptr; // o servidor recusou (peso, item de outro jogador): desiste
	}
}

void AMythPlayerController::UpdateCombat(float DeltaTime)
{
	AMythPlayerCharacter* Char = GetMythCharacter();
	AMythMonster* Target = FocusedTarget.Get();
	if (!Char || !Target)
	{
		return;
	}
	if (Target->IsDead())
	{
		ClearFocusedTarget();
		return;
	}
	if (Target->IsNetUnit())
	{
		return; // alvo do servidor: aproximacao, golpes e dano sao do rAthena
	}

	const UMythStatsComponent* Stats = Char->Stats;
	const float Range = Stats ? Stats->GetAttackRangeUU() : 80.f;
	const float Delay = Stats ? Stats->GetAttackDelaySeconds() : 1.f;

	// Distancia ate a borda do alvo, no plano.
	const float Dist = FVector::Dist2D(Char->GetActorLocation(), Target->GetActorLocation())
		- Target->GetCapsuleComponent()->GetScaledCapsuleRadius()
		- Char->GetCapsuleComponent()->GetScaledCapsuleRadius();

	if (Dist <= Range)
	{
		StopMovement();
		bHasDirectTarget = false;

		const double Now = GetWorld()->GetTimeSeconds();
		if (Now - LastAttackTime >= Delay)
		{
			LastAttackTime = Now;
			if (Char->Sprite)
			{
				Char->Sprite->FaceLocation(Target->GetActorLocation());
				Char->Sprite->PlayAttack(Delay);
			}

			// 1. Envia o pacote oficial de ataque (CZ_REQUEST_ACT 0x0360) ao servidor rAthena
			if (URagnarokNetworkSubsystem* Net = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>())
			{
				Net->SendActionRequest(Target->GetUniqueID(), 0);
			}

			// 2. Feedback visual clássico imediato (Floating Combat Text nos anos 2000)
			const int32 BaseDmg = FMath::RandRange(38, 55);
			const bool bCrit = (FMath::RandRange(1, 100) <= 15);
			const int32 Damage = bCrit ? BaseDmg * 2 : BaseDmg;
			UMythCombatText::ShowDamage(this, Target->GetActorLocation() + FVector(0.f, 0.f, 50.f), Damage, bCrit ? ECombatTextType::Critical : ECombatTextType::DamageDealt);
			// Som de golpe do cliente original para ataque desarmado (_hit_fist1..4.wav, sorteado).
			UMythRoSound::PlayWavAtLocation(this, FString::Printf(TEXT("_hit_fist%d.wav"), FMath::RandRange(1, 4)), Target->GetActorLocation());

			// 3. Aplica o dano ao monstro (HP, animacao Hit e morte). Dano ainda sorteado no cliente;
			// quando o servidor enviar o dano real, ele deve chamar este mesmo ReceiveMythDamage.
			Target->ReceiveMythDamage(Damage);

			OnAttackPerformed.Broadcast(Target);
		}
	}
	else
	{
		// Persegue: refaz o caminho a cada 0.25 s porque o alvo se move.
		ChaseRepathTimer -= DeltaTime;
		if (ChaseRepathTimer <= 0.f)
		{
			ChaseRepathTimer = 0.25f;
			MoveToLocation(Target->GetActorLocation(), /*bForceSend=*/false);
		}
	}
}

void AMythPlayerController::MythSpawnMonster()
{
	if (APawn* P = GetPawn())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		const FVector Loc = P->GetActorLocation() + FVector(400.f, 0.f, 0.f);
		GetWorld()->SpawnActor<AMythPoring>(AMythPoring::StaticClass(), Loc, FRotator::ZeroRotator, Params);
	}
}

void AMythPlayerController::MythKillTarget()
{
	if (AMythMonster* T = GetFocusedTarget())
	{
		T->Kill();
	}
}

void AMythPlayerController::MythSetHour(float Hour)
{
	for (TActorIterator<AMythDayNightActor> It(GetWorld()); It; ++It)
	{
		It->SetTimeOfDay(Hour);
	}
}

void AMythPlayerController::MythWeather(const FString& Type)
{
	AMythWeatherActor* Weather = AMythWeatherActor::FindOrSpawn(GetWorld());
	if (!Weather)
	{
		return;
	}
	EMythWeather W = EMythWeather::Clear;
	if (Type.Equals(TEXT("Rain"), ESearchCase::IgnoreCase) || Type.Equals(TEXT("Chuva"), ESearchCase::IgnoreCase)) { W = EMythWeather::Rain; }
	else if (Type.Equals(TEXT("Snow"), ESearchCase::IgnoreCase) || Type.Equals(TEXT("Neve"), ESearchCase::IgnoreCase)) { W = EMythWeather::Snow; }
	else if (Type.Equals(TEXT("Fog"), ESearchCase::IgnoreCase) || Type.Equals(TEXT("Neblina"), ESearchCase::IgnoreCase)) { W = EMythWeather::Fog; }
	Weather->SetWeather(W);
}

namespace
{
	// Timers dos comandos de teste ficam no GameInstance: sobrevivem a troca de mapa (portal/warp), que recria o
	// controlador. O comando roda no controlador atual.
	void MythRunLater(UObject* Context, float Delay, TFunction<void(APlayerController*)> Fn)
	{
		UGameInstance* GI = Context ? Context->GetWorld()->GetGameInstance() : nullptr;
		if (!GI)
		{
			return;
		}
		FTimerHandle Handle;
		GI->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(GI, [GI, Fn]()
		{
			if (APlayerController* PC = GI->GetFirstLocalPlayerController())
			{
				Fn(PC);
			}
		}), FMath::Max(Delay, 0.1f), false);
	}

	void MythTakeShot(const FString& Name)
	{
		FScreenshotRequest::RequestScreenshot(Name.IsEmpty() ? FString(TEXT("myth")) : Name, /*bShowUI=*/true, /*bAddFilenameSuffix=*/false);
		UE_LOG(LogTemp, Log, TEXT("MythShot: print '%s' solicitado"), *Name);
	}
}

void AMythPlayerController::MythShot(float DelaySeconds, float Hour, const FString& Name)
{
	MythRunLater(this, DelaySeconds, [Hour, Name](APlayerController* PC)
	{
		if (Hour < 0.f)
		{
			MythTakeShot(Name); // sem troca de horario: print imediato (ex.: numeros de dano em voo)
			return;
		}
		PC->ConsoleCommand(FString::Printf(TEXT("MythSetHour %f"), Hour));
		MythRunLater(PC, 2.f, [Name](APlayerController*) { MythTakeShot(Name); }); // luz assenta
	});
}

void AMythPlayerController::MythDelayed(float DelaySeconds, const FString& Command)
{
	// '+' vira espaco: o -ExecCmds nao preserva aspas (ex.: MythDelayed 30 MythWeather+Rain)
	const FString Cmd = Command.Replace(TEXT("+"), TEXT(" "));
	MythRunLater(this, DelaySeconds, [Cmd](APlayerController* PC) { PC->ConsoleCommand(Cmd); });
}

void AMythPlayerController::MythQuitAfter(float Seconds)
{
	MythRunLater(this, FMath::Max(Seconds, 1.f), [](APlayerController* PC) { PC->ConsoleCommand(TEXT("quit")); });
}

void AMythPlayerController::MythGoto(int32 X, int32 Y)
{
	TeleportToCell(X, Y);
}

void AMythPlayerController::MythLogin(const FString& User, const FString& Password)
{
	if (HudWidget)
	{
		HudWidget->HideEntry(); // login de teste direto: sem a tela de entrada (entra com o primeiro personagem)
	}
	if (AMythNetWorld* NetWorld = AMythNetWorld::FindOrSpawn(GetWorld()))
	{
		NetWorld->Login(User, Password);
	}
}

void AMythPlayerController::MythSay(const FString& Text)
{
	if (URagnarokNetworkSubsystem* Net = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>())
	{
		Net->SendGlobalMessage(Text.Replace(TEXT("+"), TEXT(" ")));
	}
}

void AMythPlayerController::MythTalkNearest()
{
	const APawn* P = GetPawn();
	AMythMonster* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();
	for (TActorIterator<AMythMonster> It(GetWorld()); It && P; ++It)
	{
		const float D = FVector::DistSquared(It->GetActorLocation(), P->GetActorLocation());
		if (It->IsNpc() && D < BestDist)
		{
			BestDist = D;
			Best = *It;
		}
	}
	if (Best)
	{
		UE_LOG(LogTemp, Log, TEXT("MythNet: falando com %s"), *Best->MonsterName);
		SetFocusedTarget(Best);
	}
	else
	{
		int32 Count = 0;
		for (TActorIterator<AMythMonster> It(GetWorld()); It; ++It)
		{
			if (++Count <= 8)
			{
				UE_LOG(LogTemp, Log, TEXT("MythTalkNearest: sem NPC; unidade '%s' tipo %d rede %d"), *It->MonsterName, It->NetObjectType, It->IsNetUnit() ? 1 : 0);
			}
		}
		UE_LOG(LogTemp, Log, TEXT("MythTalkNearest: nenhum NPC entre %d unidades"), Count);
	}
}

void AMythPlayerController::MythPickNearest()
{
	const APawn* P = GetPawn();
	AMythGroundItem* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();
	for (TActorIterator<AMythGroundItem> It(GetWorld()); It && P; ++It)
	{
		const float D = FVector::DistSquared(It->GetActorLocation(), P->GetActorLocation());
		if (D < BestDist)
		{
			BestDist = D;
			Best = *It;
		}
	}
	if (Best)
	{
		UE_LOG(LogTemp, Log, TEXT("MythNet: pegando %s"), *Best->GetLabel());
		PendingPickup = Best;
		PickupTries = 0;
		LastPickupTryTime = -1000.0;
		MoveToLocation(Best->GetActorLocation(), /*bForceSend=*/true);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("MythPickNearest: nenhum item no chao"));
	}
}

void AMythPlayerController::MythWalk(int32 X, int32 Y)
{
	UE_LOG(LogTemp, Log, TEXT("MythWalk: andando ate (%d, %d)"), X, Y);
	if (const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld()))
	{
		const APawn* P = GetPawn();
		MoveToLocation(Info->CellToWorld(FIntPoint(X, Y), P ? P->GetActorLocation().Z : 0.f), true);
	}
}

void AMythPlayerController::MythNpcNext()
{
	if (HudWidget)
	{
		HudWidget->NpcDialogConfirm();
	}
}

void AMythPlayerController::MythAttackNearest()
{
	const APawn* P = GetPawn();
	AMythMonster* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();
	for (TActorIterator<AMythMonster> It(GetWorld()); It && P; ++It)
	{
		if (It->IsNetUnit() && It->IsAttackable() && !It->IsDead())
		{
			const float D = FVector::DistSquared(It->GetActorLocation(), P->GetActorLocation());
			if (D < BestDist)
			{
				BestDist = D;
				Best = *It;
			}
		}
	}
	if (Best)
	{
		SetFocusedTarget(Best);
	}
}

bool AMythPlayerController::TeleportToCell(int32 X, int32 Y)
{
	UE_LOG(LogTemp, Log, TEXT("MythMove: teleporte para (%d, %d)"), X, Y);
	APawn* P = GetPawn();
	const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld());
	if (!P || !Info)
	{
		return false;
	}
	// Celula andavel mais proxima: o chao do GAT so existe nas celulas livres (trace de visibilidade nao ve a malha visual)
	for (int32 R = 0; R <= 12; ++R)
	{
		for (int32 DY = -R; DY <= R; ++DY)
		{
			for (int32 DX = -R; DX <= R; ++DX)
			{
				if (FMath::Max(FMath::Abs(DX), FMath::Abs(DY)) != R)
				{
					continue;
				}
				const FVector At = Info->CellToWorld(FIntPoint(X + DX, Y + DY), 0.f);
				float GroundZ = 0.f;
				if (UMythCellMoverComponent::GroundZ(GetWorld(), At, GroundZ))
				{
					if (UMythCellMoverComponent* Mover = P->FindComponentByClass<UMythCellMoverComponent>())
					{
						Mover->Stop(); // tambem tira a fisica: a posicao vem do servidor, sem gravidade
					}
					// Pes no chao do .gat (sem gravidade para assentar o personagem depois)
					const ACharacter* Char = Cast<ACharacter>(P);
					const float HalfHeight = Char && Char->GetCapsuleComponent() ? Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
					P->SetActorLocation(FVector(At.X, At.Y, GroundZ + HalfHeight), false, nullptr, ETeleportType::TeleportPhysics);
					UE_LOG(LogTemp, Log, TEXT("MythMove: no chao da celula (%d, %d) em %s"), X + DX, Y + DY, *P->GetActorLocation().ToString());
					StopMovement();
					bHasDirectTarget = false;
					return true;
				}
			}
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("MythGoto: nenhuma celula andavel perto de (%d,%d)"), X, Y);
	return false;
}

void AMythPlayerController::MythWindow(const FString& Name)
{
	const UEnum* Enum = StaticEnum<EMythRoWindow>();
	const int64 Value = Enum ? Enum->GetValueByNameString(Name) : INDEX_NONE;
	if (Value != INDEX_NONE)
	{
		ToggleHudWindow(static_cast<EMythRoWindow>(Value));
	}
}

void AMythPlayerController::MythDayLength(float Seconds)
{
	for (TActorIterator<AMythDayNightActor> It(GetWorld()); It; ++It)
	{
		It->DayLengthSeconds = FMath::Max(1.f, Seconds);
	}
}

void AMythPlayerController::MythHurtMonster(int32 Damage)
{
	AMythMonster* Target = GetFocusedTarget();
	if (!Target && GetPawn())
	{
		float Best = TNumericLimits<float>::Max();
		for (TActorIterator<AMythMonster> It(GetWorld()); It; ++It)
		{
			const float D = FVector::DistSquared(It->GetActorLocation(), GetPawn()->GetActorLocation());
			if (!It->IsDead() && D < Best)
			{
				Best = D;
				Target = *It;
			}
		}
	}
	if (Target)
	{
		SetFocusedTarget(Target);
		UMythCombatText::ShowDamage(this, Target->GetActorLocation() + FVector(0.f, 0.f, 50.f), Damage, ECombatTextType::DamageDealt);
		Target->ReceiveMythDamage(Damage);
	}
}

// ------------------------------------------------------------------------------------------------ skills e atalhos

void AMythPlayerController::UseHotkey(int32 Index)
{
	URagnarokNetworkSubsystem* Net = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>();
	if (!Net || !Net->GetHotkeys().IsValidIndex(Index))
	{
		return;
	}
	const FRagnarokHotkey& H = Net->GetHotkeys()[Index];
	if (H.ID == 0)
	{
		return;
	}
	if (H.bIsSkill)
	{
		BeginSkill(H.ID, FMath::Max(1, H.Count));
		return;
	}
	// Item: usa o primeiro do inventario com esse ID (equipamento: veste, como o duplo clique)
	for (const FRagnarokInventoryItem& Item : Net->GetInventoryItems())
	{
		if (Item.ItemID == H.ID && !Item.bIsEquipped && Item.Amount > 0)
		{
			if (Item.Type == 4 || Item.Type == 5)
			{
				Net->EquipItem(Item.Index, Item.EquipLocation);
			}
			else
			{
				Net->UseItem(Item.Index);
			}
			return;
		}
	}
}

void AMythPlayerController::BeginSkill(int32 SkillID, int32 Level)
{
	URagnarokNetworkSubsystem* Net = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>();
	if (!Net)
	{
		return;
	}
	int32 Inf = 0;
	for (const FRagnarokSkillInfo& S : Net->GetSkillList())
	{
		if (S.SkillID == SkillID)
		{
			Inf = S.Inf;
			Level = FMath::Clamp(Level, 1, FMath::Max(1, S.Level));
			break;
		}
	}
	if (Inf == 0)
	{
		return; // passiva
	}
	if (Inf & 0x04)
	{
		ClearFocusedTarget();
		Net->UseSkill(SkillID, Level, Net->GetSelfID()); // em si: sem mira
		return;
	}
	// Mira: cursor em cruz ate o proximo clique (botao direito ou Esc cancela)
	PendingSkillID = SkillID;
	PendingSkillLevel = Level;
	PendingSkillInf = Inf;
	CurrentMouseCursor = EMouseCursor::Crosshairs;
}

void AMythPlayerController::CancelSkillTargeting()
{
	PendingSkillID = 0;
	CurrentMouseCursor = EMouseCursor::Default;
}

bool AMythPlayerController::HandleSkillTargetClick(const FHitResult& Hit)
{
	URagnarokNetworkSubsystem* Net = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>();
	if (!Net || PendingSkillID == 0)
	{
		return false;
	}
	const int32 Skill = PendingSkillID;
	const int32 Level = PendingSkillLevel;
	const int32 Inf = PendingSkillInf;
	CancelSkillTargeting();
	ClearFocusedTarget(); // usar skill interrompe o ataque comum (o cliente nao deve retomar a perseguicao)

	AActor* HitActor = Hit.GetActor();
	if (AMythMonster* Unit = Cast<AMythMonster>(HitActor); Unit && Unit->IsNetUnit() && (Inf & (0x01 | 0x10)))
	{
		Net->UseSkill(Skill, Level, Unit->NetID);
		return true;
	}
	if ((Inf & 0x10) && GetPawn() && (HitActor == GetPawn() || FVector::Dist2D(Hit.ImpactPoint, GetPawn()->GetActorLocation()) < 60.f))
	{
		Net->UseSkill(Skill, Level, Net->GetSelfID()); // suporte em si mesmo
		return true;
	}
	if (Inf & 0x02)
	{
		const FIntPoint Cell = WorldToCell(Hit.ImpactPoint);
		Net->UseSkill(Skill, Level, 0, Cell.X, Cell.Y);
		return true;
	}
	return true; // clique sem alvo valido: so cancela a mira
}

void AMythPlayerController::MythHotkey(int32 Index, int32 SkillID, int32 Level)
{
	if (URagnarokNetworkSubsystem* Net = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>())
	{
		Net->SetHotkey(Index, true, SkillID, Level);
	}
}

void AMythPlayerController::MythPressF(int32 Number)
{
	UseHotkey((HudWidget ? HudWidget->HotbarActiveBase() : 0) + Number - 1);
}

void AMythPlayerController::MythPlayStr(const FString& File)
{
	if (APawn* P = GetPawn())
	{
		AMythStrEffect::Play(this, File, AMythStrEffect::FeetOf(P), P);
	}
}

void AMythPlayerController::MythPlaySkill(int32 SkillID, int32 Stage)
{
	if (APawn* P = GetPawn())
	{
		FMythSkillEffects::Play(this, SkillID, static_cast<EMythSkillEffectStage>(FMath::Clamp(Stage, 0, 3)), P, P->GetActorLocation());
	}
}

void AMythPlayerController::MythBuff(int32 EfstID, float Seconds)
{
	if (AMythHUD* Hud = Cast<AMythHUD>(GetHUD()))
	{
		const int32 Ms = FMath::RoundToInt(FMath::Max(Seconds, 0.f) * 1000.f);
		Hud->SetStatusIcon(EfstID, Seconds >= 0.f, Ms, Ms);
	}
}

void AMythPlayerController::MythLook(int32 Look, int32 Value)
{
	UMythRoSpriteComponent* S = GetPawn() ? GetPawn()->FindComponentByClass<UMythRoSpriteComponent>() : nullptr;
	if (!S)
	{
		return;
	}
	switch (Look)
	{
	case 0: S->JobClass = Value; break;
	case 2: S->Weapon = Value; break;
	case 3: S->HeadBottom = Value; break;
	case 4: S->HeadTop = Value; break;
	case 5: S->HeadMid = Value; break;
	case 8: S->Shield = Value; break;
	case 99: S->PlayAttack(FMath::Max(Value, 1) / 1000.f); return; // so a animacao de ataque (Value = ms)
	default: return;
	}
	UE_LOG(LogTemp, Log, TEXT("MythLook %d = %d"), Look, Value);
	S->ReloadSprites();
	UE_LOG(LogTemp, Log, TEXT("MythLook recarregado"));
}

void AMythPlayerController::MythTargetNearest()
{
	if (PendingSkillID == 0 || !GetPawn())
	{
		return;
	}
	AMythMonster* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();
	for (TActorIterator<AMythMonster> It(GetWorld()); It; ++It)
	{
		const float D = FVector::DistSquared(It->GetActorLocation(), GetPawn()->GetActorLocation());
		if (It->IsNetUnit() && It->IsAttackable() && !It->IsDead() && D < BestDist)
		{
			BestDist = D;
			Best = *It;
		}
	}
	if (Best)
	{
		FHitResult Hit;
		Hit.HitObjectHandle = FActorInstanceHandle(Best);
		Hit.ImpactPoint = Best->GetActorLocation();
		HandleSkillTargetClick(Hit);
	}
}

void AMythPlayerController::MythFireArrow(int32 Count)
{
	if (Count > 1)
	{
		// Rajada: uma flecha a cada 80 ms, para pegar uma no ar nos prints
		for (int32 i = 0; i < FMath::Min(Count, 100); ++i)
		{
			FTimerHandle H;
			GetWorldTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(this, [this]() { MythFireArrow(1); }), 0.08f * i + 0.01f, false);
		}
		return;
	}
	const APawn* P = GetPawn();
	AMythMonster* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();
	for (TActorIterator<AMythMonster> It(GetWorld()); It && P; ++It)
	{
		const float D = FVector::DistSquared(It->GetActorLocation(), P->GetActorLocation());
		if (!It->IsDead() && D < BestDist)
		{
			BestDist = D;
			Best = *It;
		}
	}
	if (Best && P)
	{
		FMythSkillEffects::PlayEffectId(this, 2003, Best, Best->GetActorLocation(), const_cast<APawn*>(P));
	}
}

void AMythPlayerController::MythRestart(int32 Type)
{
	if (URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr)
	{
		Net->SendRestart(static_cast<uint8>(Type));
	}
}

void AMythPlayerController::MythAura(int32 Mode)
{
	AMythAura99::SetMode(Mode);
}

void AMythPlayerController::MythFloatCheck()
{
	// Pes da capsula (centro - meia altura) contra o chao do .gat sob a unidade; diferenca > 5 uu = flutuando/afundado
	int32 Count = 0, Off = 0;
	auto Check = [&](const AActor* A, float HalfHeight, const TCHAR* Kind)
	{
		const float Feet = A->GetActorLocation().Z - HalfHeight;
		float Near = 0.f, Top = 0.f;
		const bool bNear = UMythCellMoverComponent::GroundZ(GetWorld(), A->GetActorLocation(), Near, &Feet);
		const bool bTop = UMythCellMoverComponent::GroundZ(GetWorld(), A->GetActorLocation(), Top);
		++Count;
		if (!bNear || FMath::Abs(Feet - Near) > 5.f)
		{
			++Off;
			UE_LOG(LogTemp, Log, TEXT("MythFloat: %s '%s' em (%.0f,%.0f) pes %.0f | chao perto dos pes %s%.0f | chao mais alto %s%.0f"), Kind, *A->GetName(), A->GetActorLocation().X, A->GetActorLocation().Y, Feet, bNear ? TEXT("") : TEXT("(nenhum) "), Near, bTop ? TEXT("") : TEXT("(nenhum) "), Top);
		}
	};
	if (const APawn* P = GetPawn())
	{
		const UCapsuleComponent* Cap = P->FindComponentByClass<UCapsuleComponent>();
		Check(P, Cap ? Cap->GetScaledCapsuleHalfHeight() : 90.f, TEXT("jogador"));
	}
	for (TActorIterator<AMythMonster> It(GetWorld()); It; ++It)
	{
		Check(*It, It->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), It->IsNpc() ? TEXT("npc") : TEXT("unidade"));
	}
	UE_LOG(LogTemp, Log, TEXT("MythFloat: %d unidades medidas, %d fora do chao"), Count, Off);
	// Quem responde ao canal Visibility na vertical do jogador e do primeiro NPC (para achar o "chao" que o mover usa)
	TArray<const AActor*> Probes;
	if (GetPawn()) { Probes.Add(GetPawn()); }
	for (TActorIterator<AMythMonster> It(GetWorld()); It && Probes.Num() < 3; ++It) { if (It->IsNpc()) { Probes.Add(*It); } }
	for (const AActor* A : Probes)
	{
		TArray<FHitResult> Hits;
		FCollisionQueryParams Q(TEXT("MythFloatProbe"), true); // complexa: o chao do .gat pode so ter colisao complexa
		GetWorld()->LineTraceMultiByChannel(Hits, FVector(A->GetActorLocation().X, A->GetActorLocation().Y, 20000.f), FVector(A->GetActorLocation().X, A->GetActorLocation().Y, -3000.f), ECC_Visibility, Q);
		for (const FHitResult& H : Hits)
		{
			UE_LOG(LogTemp, Log, TEXT("MythFloat: sonda de '%s' (z %.0f): z %.0f ator '%s' comp '%s' mobilidade %d"), *A->GetName(), A->GetActorLocation().Z, H.ImpactPoint.Z, *GetNameSafe(H.GetActor()), *GetNameSafe(H.GetComponent()), H.GetComponent() ? static_cast<int32>(H.GetComponent()->Mobility) : -1);
		}
	}
}

void AMythPlayerController::MythShop(int32 Type) { if (HudWidget) { HudWidget->ShopChooseDeal(Type); } }
void AMythPlayerController::MythShopAdd(int32 Row, int32 Qty) { if (HudWidget) { HudWidget->ShopAddToBasket(Row, Qty); } }
void AMythPlayerController::MythShopConfirm() { if (HudWidget) { HudWidget->ShopConfirm(); } }

void AMythPlayerController::MythDropFirst(int32 Amount)
{
	if (URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr)
	{
		for (const FRagnarokInventoryItem& Item : Net->GetInventoryItems())
		{
			if (!Item.bIsEquipped)
			{
				Net->SendDropItem(Item.Index, FMath::Min(Amount, Item.Amount));
				return;
			}
		}
	}
}

void AMythPlayerController::MythTradeReply(int32 Accept) { if (URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>()) { N->SendTradeReply(Accept != 0); } }
void AMythPlayerController::MythTradeAdd(int32 Index, int32 Amount)
{
	if (Index < 0)
	{
		// -1: o primeiro item da mochila que nao esta equipado (teste)
		if (const URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>())
		{
			for (const FRagnarokInventoryItem& I : N->GetInventoryItems())
			{
				if (!I.bIsEquipped && I.Amount >= Amount)
				{
					Index = I.Index;
					break;
				}
			}
		}
	}
	if (HudWidget)
	{
		HudWidget->TryTradeAddFromInventory(Index, Amount);
	}
}
void AMythPlayerController::MythTradeLock() { if (URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>()) { N->SendTradeLock(); } }
void AMythPlayerController::MythTradeCommit() { if (URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>()) { N->SendTradeCommit(); } }
void AMythPlayerController::MythEntryLogin(const FString& User, const FString& Pass) { if (HudWidget) { HudWidget->EntryFillAndLogin(User, Pass); } }
void AMythPlayerController::MythEntryAction(int32 Action, const FString& Name) { if (HudWidget) { HudWidget->EntryTestAction(Action, Name); } }
void AMythPlayerController::MythRoom(int32 Create, const FString& Title)
{
	URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>();
	if (!N) { return; }
	if (Create) { N->SendCreateChatRoom(Title.Replace(TEXT("+"), TEXT(" ")), 8, true, FString()); return; }
	for (const TPair<int32, FRoChatRoom>& P : N->GetChatRooms()) { N->SendEnterChatRoom(P.Key, FString()); break; }
}
void AMythPlayerController::MythFriendReply(int32 Accept) { if (HudWidget) { HudWidget->AnswerFriendRequest(Accept != 0); } }
void AMythPlayerController::MythOptions(int32 Tab) { if (HudWidget) { HudWidget->ShowOptionsWindow(Tab); } }
void AMythPlayerController::MythPartyAnswer(int32 Accept) { if (HudWidget) { HudWidget->AnswerPartyInvite(Accept != 0); } }
void AMythPlayerController::MythStore(int32 ToStorage, int32 Row, int32 Qty) { if (HudWidget) { HudWidget->StorageMove(ToStorage != 0, Row, Qty); } }

void AMythPlayerController::MythChat(const FString& Text) { if (HudWidget) { HudWidget->SubmitChat(Text.Replace(TEXT("+"), TEXT(" "))); } }

void AMythPlayerController::MythBank(int32 Op, int32 Amount)
{
	if (URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>())
	{
		N->SendBankCommand(Op == 1 ? 0x09a7 : 0x09a9, Amount);
	}
}

void AMythPlayerController::MythSwitch(int32 Op)
{
	URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>();
	if (!N) { return; }
	if (Op == 1) { N->SendEquipSwitchAll(); return; }
	for (const FRagnarokInventoryItem& It : N->GetInventoryItems())
	{
		if ((It.Type == 4 || It.Type == 5) && !It.bIsEquipped && It.EquipLocation != 0)
		{
			N->SendEquipSwitchAdd(It.Index, It.EquipLocation);
			break;
		}
	}
}

void AMythPlayerController::MythMail(int32 Op, const FString& To, const FString& Title, const FString& Body)
{
	URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>();
	if (!N) { return; }
	if (Op == 0) { N->SendMailBeginWrite(To); return; }
	if (Op == 2) { if (N->GetMails().Num() > 0) { N->SendMailRequest(0x09ea, N->GetMails()[0].Id, N->GetMails()[0].Tab); } return; }
	N->SendMailWrite(To, Title.Replace(TEXT("+"), TEXT(" ")), Body.Replace(TEXT("+"), TEXT(" ")));
}

void AMythPlayerController::MythEquip(int32 ItemID)
{
	// Teste: veste o primeiro item com esse ID (municao usa 0x8000)
	if (URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr)
	{
		for (const FRagnarokInventoryItem& Item : Net->GetInventoryItems())
		{
			if (static_cast<int32>(Item.ItemID) == ItemID && !Item.bIsEquipped)
			{
				Net->EquipItem(Item.Index, Item.EquipLocation);
				return;
			}
		}
	}
}

void AMythPlayerController::MythVendBrowse()
{
	if (URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr)
	{
		for (const TPair<int32, FString>& Board : Net->GetVendingBoards())
		{
			UE_LOG(LogTemp, Log, TEXT("MythVend: abrindo a loja '%s' de %d"), *Board.Value, Board.Key);
			Net->SendVendingListReq(Board.Key);
			return;
		}
		UE_LOG(LogTemp, Log, TEXT("MythVend: nenhuma loja por perto"));
	}
}

void AMythPlayerController::MythUseItem(int32 ItemID)
{
	if (URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr)
	{
		for (const FRagnarokInventoryItem& Item : Net->GetInventoryItems())
		{
			if (static_cast<int32>(Item.ItemID) == ItemID)
			{
				if (Item.Type == 6) { Net->SendCardListRequest(Item.Index); } else { Net->UseItem(Item.Index); }
				return;
			}
		}
	}
}

void AMythPlayerController::MythTakeOffAll()
{
	if (URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr)
	{
		Net->SendTakeOffAll(0x0003FFu | 0x8000u); // equipamento da aba Geral e a municao
	}
}
