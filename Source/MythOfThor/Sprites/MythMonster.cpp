#include "Sprites/MythMonster.h"

#include "Sprites/MythRoSpriteComponent.h"
#include "Components/BoxComponent.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/MythPlayerController.h"

AMythMonster::AMythMonster()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(30.f, 40.f);

	Sprite = CreateDefaultSubobject<UMythRoSpriteComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(RootComponent);
	ClickBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ClickBox"));
	// Presa ao sprite (que fica paralelo a tela): o clique vale onde o monstro aparece desenhado, como no RO.
	ClickBox->SetupAttachment(Sprite);
	ClickBox->SetBoxExtent(FVector(40.f, 40.f, 40.f));
	ClickBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ClickBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	ClickBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// Pivo do sprite fica nos pes (bottom-center): desce ate o chao da capsula.
	Sprite->SetRelativeLocation(FVector(0.f, 0.f, -40.f));
}

void AMythMonster::PlaySfx(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}
}

void AMythMonster::FitClickBoxToSprite()
{
	// Limites do quadro desenhado, no espaco do sprite (X = largura, Z = altura, Y = espessura).
	const FBox& Bounds = Sprite->GetFrameLocalBounds();
	if (!Bounds.IsValid)
	{
		return;
	}
	const FVector Extent = Bounds.GetExtent();
	ClickBox->SetRelativeLocation(Bounds.GetCenter());
	ClickBox->SetBoxExtent(FVector(FMath::Max(Extent.X, 15.f), 15.f, FMath::Max(Extent.Z, 15.f)));
}

void AMythMonster::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	FitClickBoxToSprite();
	if (bIsDead)
	{
		return;
	}
	const bool bMoving = GetVelocity().Size2D() > 10.f;
	if (bMoving && !bWasMoving)
	{
		PlaySfx(SfxMove);
	}
	bWasMoving = bMoving;
}

void AMythMonster::PlayAttackFeedback()
{
	if (!bIsDead)
	{
		Sprite->PlayAttack();
		PlaySfx(SfxAttack);
	}
}

void AMythMonster::BeginPlay()
{
	Super::BeginPlay();
	HP = MaxHP;
}

void AMythMonster::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogTemp, Log, TEXT("[Monster] %s removido (%d)"), *GetDisplayName(), static_cast<int32>(EndPlayReason));
	Super::EndPlay(EndPlayReason);
}

FString AMythMonster::GetDisplayName() const
{
	return FString::Printf(TEXT("%s [Lv %d]"), *MonsterName, BaseLevel);
}

float AMythMonster::GetHealthFraction() const
{
	return MaxHP > 0 ? FMath::Clamp(static_cast<float>(HP) / MaxHP, 0.f, 1.f) : 0.f;
}

bool AMythMonster::ReceiveMythDamage(int32 Amount)
{
	if (bIsDead || Amount <= 0)
	{
		return false;
	}

	HP = FMath::Max(0, HP - Amount);
	UE_LOG(LogTemp, Log, TEXT("[Monster] %s recebeu %d de dano: HP %d/%d"), *GetDisplayName(), Amount, HP, MaxHP);
	if (HP == 0)
	{
		Kill();
		return true;
	}

	Sprite->PlayHit();
	PlaySfx(SfxHit);
	return false;
}

void AMythMonster::Kill()
{
	if (bIsDead)
	{
		return;
	}
	bIsDead = true;
	HP = 0;
	UE_LOG(LogTemp, Log, TEXT("[Monster] %s morreu; Destroy em %.1f s"), *GetDisplayName(), DeathDestroyDelay);

	// Sem colisao nem movimento: deixa de ser clicavel, bloquear passagem ou perseguir.
	ClickBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->DisableMovement();

	// Limpa o alvo focado de quem estiver mirando neste monstro.
	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (AMythPlayerController* PC = Cast<AMythPlayerController>(It->Get()))
			{
				if (PC->GetFocusedTarget() == this)
				{
					PC->ClearFocusedTarget();
				}
			}
		}
	}

	PlaySfx(SfxDie);
	Sprite->Die();
	SetLifeSpan(DeathDestroyDelay); // auto-Destroy apos a animacao de morte
}
