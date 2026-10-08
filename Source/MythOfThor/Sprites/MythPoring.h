#pragma once

#include "CoreMinimal.h"
#include "Sprites/MythMonster.h"
#include "MythPoring.generated.h"

/**
 * Poring (Lv 1, 50 HP): monstro de teste completo. Carrega, se existirem, os assets gerados por
 * tools/ue_setup_poring.py: sprites 8 direcoes (/Game/Sprites/poring/DA_poring) e os efeitos sonoros
 * (/Game/Monsters/Poring/Audio/SFX_poring_*). Sem os assets ele ainda funciona (invisivel e mudo).
 */
UCLASS()
class MYTHOFTHOR_API AMythPoring : public AMythMonster
{
	GENERATED_BODY()

public:
	AMythPoring();
};
