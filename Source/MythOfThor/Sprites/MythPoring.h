#pragma once

#include "CoreMinimal.h"
#include "Sprites/MythMonster.h"
#include "MythPoring.generated.h"

/**
 * Poring (Lv 1, 50 HP): monstro de teste. O sprite vem do cliente original (몬스터/poring) e os sons,
 * se existirem, de tools/ue_setup_poring.py (/Game/Monsters/Poring/Audio/SFX_poring_*).
 * Sem os sons ele funciona normalmente (mudo).
 */
UCLASS()
class MYTHOFTHOR_API AMythPoring : public AMythMonster
{
	GENERATED_BODY()

public:
	AMythPoring();
};
