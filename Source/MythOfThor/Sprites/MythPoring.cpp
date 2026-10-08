#include "Sprites/MythPoring.h"

#include "Sprites/MythSpriteAnimComponent.h"
#include "Sprites/MythSpriteSet.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

AMythPoring::AMythPoring()
{
	MonsterName = TEXT("Poring");
	BaseLevel = 1;
	MaxHP = 50;
	HP = 50;

	GetCapsuleComponent()->InitCapsuleSize(40.f, 40.f);
	ClickBox->SetBoxExtent(FVector(45.f, 45.f, 45.f));
	Sprite->SetRelativeLocation(FVector(0.f, 0.f, -40.f));

	// Assets opcionais: se ainda nao foram importados, o Poring simplesmente fica sem sprite/som.
	static ConstructorHelpers::FObjectFinderOptional<UMythSpriteSet> SetFinder(TEXT("/Game/Sprites/poring/DA_poring.DA_poring"));
	if (SetFinder.Succeeded())
	{
		Sprite->SpriteSet = SetFinder.Get();
	}

	static ConstructorHelpers::FObjectFinderOptional<USoundBase> Attack(TEXT("/Game/Monsters/Poring/Audio/SFX_poring_attack.SFX_poring_attack"));
	static ConstructorHelpers::FObjectFinderOptional<USoundBase> Hit(TEXT("/Game/Monsters/Poring/Audio/SFX_poring_damage.SFX_poring_damage"));
	static ConstructorHelpers::FObjectFinderOptional<USoundBase> Die(TEXT("/Game/Monsters/Poring/Audio/SFX_poring_die.SFX_poring_die"));
	static ConstructorHelpers::FObjectFinderOptional<USoundBase> Move(TEXT("/Game/Monsters/Poring/Audio/SFX_poring_move.SFX_poring_move"));
	if (Attack.Succeeded()) { SfxAttack = Attack.Get(); }
	if (Hit.Succeeded())    { SfxHit = Hit.Get(); }
	if (Die.Succeeded())    { SfxDie = Die.Get(); }
	if (Move.Succeeded())   { SfxMove = Move.Get(); }
}
