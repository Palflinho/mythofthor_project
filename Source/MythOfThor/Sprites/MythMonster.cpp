#include "Sprites/MythMonster.h"

#include "Sprites/MythSpriteAnimComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/MythPlayerController.h"

AMythMonster::AMythMonster()
{
	GetCapsuleComponent()->InitCapsuleSize(30.f, 40.f);

	Sprite = CreateDefaultSubobject<UMythSpriteAnimComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(RootComponent);
	ClickBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ClickBox"));
	ClickBox->SetupAttachment(RootComponent);
	ClickBox->SetBoxExtent(FVector(40.f, 40.f, 40.f));
	ClickBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ClickBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	ClickBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// Pivo do sprite fica nos pes (bottom-center): desce ate o chao da capsula.
	Sprite->SetRelativeLocation(FVector(0.f, 0.f, -40.f));
}

void AMythMonster::BeginPlay()
{
	Super::BeginPlay();
	HP = MaxHP;
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
	if (HP == 0)
	{
		Kill();
		return true;
	}

	Sprite->PlayHit();
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

	Sprite->Die();
	SetLifeSpan(DeathDestroyDelay); // auto-Destroy apos a animacao de morte
}
