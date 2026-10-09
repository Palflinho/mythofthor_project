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
#include "Sprites/MythSkillEffects.h"
#include "Sprites/MythStrEffect.h"

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
	MapMove(EKeys::W, true, false);
	MapMove(EKeys::S, true, true);
	MapMove(EKeys::D, false, false);
	MapMove(EKeys::A, false, true);

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
	InputComponent->BindKey(FInputChord(EKeys::Tab, false, true, false, false), IE_Pressed, this, &AMythPlayerController::OnHotkeyMinimap);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AMythPlayerController::OnHotkeyEscape);
	// F1..F9: atalhos da barra (itens e skills guardados no servidor)
	const FKey HotkeyKeys[9] = { EKeys::F1, EKeys::F2, EKeys::F3, EKeys::F4, EKeys::F5, EKeys::F6, EKeys::F7, EKeys::F8, EKeys::F9 };
	for (int32 i = 0; i < 9; ++i)
	{
		FInputKeyBinding Binding(FInputChord(HotkeyKeys[i]), IE_Pressed);
		Binding.KeyDelegate.GetDelegateForManualSet().BindWeakLambda(this, [this, i]() { UseHotkey(i); });
		InputComponent->KeyBindings.Add(Binding);
	}
}

// ---------------------------------------------------------------- Point & Click

void AMythPlayerController::OnClickStarted(const FInputActionValue&)
{
	MoveToCursor();
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
	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit) && Hit.bBlockingHit)
	{
		if (PendingSkillID != 0 && HandleSkillTargetClick(Hit))
		{
			return; // clique de mira de skill
		}
		if (AMythMonster* Monster = Cast<AMythMonster>(Hit.GetActor()))
		{
			if (!Monster->IsDead())
			{
				SetFocusedTarget(Monster);
				return;
			}
		}
		if (AMythGroundItem* Item = Cast<AMythGroundItem>(Hit.GetActor()))
		{
			// Item no chao: anda ate ele e, perto o bastante, pede ao servidor para pegar (CZ_ITEM_PICKUP)
			ClearFocusedTarget();
			PendingPickup = Item;
			MoveToLocation(Item->GetActorLocation());
			return;
		}
		PendingPickup = nullptr;
		ClearFocusedTarget(); // clicar no chao cancela o alvo
		MoveToLocation(Hit.ImpactPoint);
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

	Char->AddMovementInput(Forward, Input.Y);
	Char->AddMovementInput(Right, Input.X);
}

void AMythPlayerController::OnMoveCompleted(const FInputActionValue&)
{
	bWasdActive = false;
	if (AMythPlayerCharacter* Char = GetMythCharacter())
	{
		// Informa ao servidor a celula onde o personagem parou.
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
	if (PendingSkillID != 0 && WasInputKeyJustPressed(EKeys::RightMouseButton))
	{
		CancelSkillTargeting(); // botao direito cancela a mira, como no cliente original
	}
	UpdateCameraReset();
	// Enter: com dialogo de NPC aberto avanca/escolhe/fecha; senao abre o chat (como no cliente original).
	if (HudWidget && WasInputKeyJustPressed(EKeys::Enter) && !HudWidget->IsChatFocused() && !HudWidget->NpcDialogConfirm())
	{
		HudWidget->FocusChat();
	}

	if (bWasdActive)
	{
		// Projeta o destino alguns tiles a frente e envia limitado por intervalo.
		TimeSinceWasdSend += DeltaTime;
		const FVector Velocity = Char->GetVelocity();
		if (TimeSinceWasdSend >= WasdSendInterval && !Velocity.IsNearlyZero())
		{
			TimeSinceWasdSend = 0.f;
			const FVector Ahead = Char->GetActorLocation() + Velocity.GetSafeNormal2D() * (WasdLookaheadCells * CellSizeUU);
			SendCellToServer(Ahead, /*bForce=*/false);
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

void AMythPlayerController::OnHotkeyStatus() { ToggleHudWindow(EMythRoWindow::Status); }
void AMythPlayerController::OnHotkeyItems() { ToggleHudWindow(EMythRoWindow::Items); }
void AMythPlayerController::OnHotkeyEquip() { ToggleHudWindow(EMythRoWindow::Equip); }
void AMythPlayerController::OnHotkeySkills() { ToggleHudWindow(EMythRoWindow::Skills); }
void AMythPlayerController::OnHotkeyBasic() { ToggleHudWindow(EMythRoWindow::Basic); }
void AMythPlayerController::OnHotkeyMinimap() { ToggleHudWindow(EMythRoWindow::Minimap); }

void AMythPlayerController::OnHotkeyEscape()
{
	if (PendingSkillID != 0)
	{
		CancelSkillTargeting(); // Esc cancela a mira antes de fechar janelas
		return;
	}
	if (HudWidget)
	{
		HudWidget->CloseTopWindow();
	}
}

void AMythPlayerController::OnRotateMouse(const FInputActionValue& Value)
{
	// Botao direito + arrastar gira; com Shift, o arrasto inclina (OnPitchMouse), como no cliente original.
	if (IsInputKeyDown(EKeys::RightMouseButton) && !IsShiftDown())
	{
		if (AMythPlayerCharacter* Char = GetMythCharacter())
		{
			Char->RotateCamera(Value.Get<float>() * MouseRotateSensitivity * 10.f);
		}
	}
}

void AMythPlayerController::OnPitchMouse(const FInputActionValue& Value)
{
	if (IsInputKeyDown(EKeys::RightMouseButton) && IsShiftDown())
	{
		if (AMythPlayerCharacter* Char = GetMythCharacter())
		{
			// Arrastar para cima deixa a camera mais de cima (pitch mais negativo).
			Char->PitchCamera(-Value.Get<float>() * MouseRotateSensitivity * 10.f);
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
	if (!bForce && Cell == LastSentCell)
	{
		return;
	}
	LastSentCell = Cell;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (URagnarokNetworkSubsystem* Net = GI->GetSubsystem<URagnarokNetworkSubsystem>())
		{
			// Retorna false se nao houver sessao; o jogo segue jogavel offline para testes.
			const bool bSent = Net->SendMoveRequest(Cell.X, Cell.Y);
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
	if (FocusedTarget.Get() == NewTarget)
	{
		return; // re-clicar no mesmo alvo nao reinicia a recarga do ataque
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
			Net->SendActionRequest(NewTarget->NetID, 7);
		}
	}
	FocusedTarget = NewTarget;
	ChaseRepathTimer = 0.f;
}

void AMythPlayerController::ClearFocusedTarget()
{
	FocusedTarget = nullptr;
}

void AMythPlayerController::UpdatePickup()
{
	AMythGroundItem* Item = PendingPickup.Get();
	const APawn* P = GetPawn();
	if (!Item || !P)
	{
		return;
	}
	// O rAthena aceita pegar a ate 2 celulas (pc_takeitem); pede ao chegar a ~1,5 celula
	if (FVector::Dist2D(P->GetActorLocation(), Item->GetActorLocation()) <= 75.f)
	{
		if (URagnarokNetworkSubsystem* Net = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>())
		{
			Net->PickUpItem(Item->NetID);
		}
		PendingPickup = nullptr;
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
		MoveToLocation(Best->GetActorLocation());
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
				const FVector Top = Info->CellToWorld(FIntPoint(X + DX, Y + DY), 20000.f);
				FHitResult Hit;
				FCollisionQueryParams Params(TEXT("MythGoto"), false, P);
				if (GetWorld()->LineTraceSingleByChannel(Hit, Top, Top - FVector(0.f, 0.f, 40000.f), ECC_Visibility, Params))
				{
					P->SetActorLocation(Hit.ImpactPoint + FVector(0.f, 0.f, 120.f), false, nullptr, ETeleportType::TeleportPhysics);
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
	UseHotkey(Number - 1);
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
		FMythSkillEffects::Play(this, SkillID, static_cast<EMythSkillEffectStage>(FMath::Clamp(Stage, 0, 2)), P, P->GetActorLocation());
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
