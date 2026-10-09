#include "Player/MythPlayerController.h"

#include "Player/MythPlayerCharacter.h"
#include "Network/RagnarokNetworkSubsystem.h"
#include "UI/MythHUD.h"
#include "World/MythMapInfo.h"
#include "World/MythDayNightActor.h"
#include "EngineUtils.h"
#include "Sprites/MythMonster.h"
#include "Sprites/MythPoring.h"
#include "Sprites/MythRoSpriteComponent.h"
#include "Components/CapsuleComponent.h"
#include "UI/MythClassicHUDWidget.h"
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
		HudWidget = CreateWidget<UMythClassicHUDWidget>(this, UMythClassicHUDWidget::StaticClass());
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
		if (AMythMonster* Monster = Cast<AMythMonster>(Hit.GetActor()))
		{
			if (!Monster->IsDead())
			{
				SetFocusedTarget(Monster);
				return;
			}
		}
		ClearFocusedTarget(); // clicar no chao cancela o alvo
		MoveToLocation(Hit.ImpactPoint);
	}
}

void AMythPlayerController::MoveToLocation(const FVector& Destination, bool bForceSend)
{
	UWorld* World = GetWorld();
	const UNavigationSystemV1* Nav = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;

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
	UpdateCameraReset();

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
			Net->SendMoveRequest(Cell.X, Cell.Y);
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
	FocusedTarget = NewTarget;
	ChaseRepathTimer = 0.f;
}

void AMythPlayerController::ClearFocusedTarget()
{
	FocusedTarget = nullptr;
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
