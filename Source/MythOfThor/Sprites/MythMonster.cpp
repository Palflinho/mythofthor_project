#include "Sprites/MythMonster.h"

#include "Sprites/MythSpriteAnimComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"

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

void AMythMonster::Kill()
{
	if (bIsDead)
	{
		return;
	}
	bIsDead = true;
	ClickBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Sprite->Die();
}
