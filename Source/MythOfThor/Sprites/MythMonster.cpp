#include "Sprites/MythMonster.h"

#include "Sprites/MythSpriteAnimComponent.h"
#include "Components/CapsuleComponent.h"

AMythMonster::AMythMonster()
{
	GetCapsuleComponent()->InitCapsuleSize(30.f, 40.f);

	Sprite = CreateDefaultSubobject<UMythSpriteAnimComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(RootComponent);
	// Pivo do sprite fica nos pes (bottom-center): desce ate o chao da capsula.
	Sprite->SetRelativeLocation(FVector(0.f, 0.f, -40.f));
}
