#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MythGameMode.generated.h"

/** Game Mode do Myth of Thor: usa o personagem e os controles hibridos em C++. */
UCLASS()
class MYTHOFTHOR_API AMythGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMythGameMode();

	/** Ciclo dia/noite do mundo (criado no BeginPlay se o nivel nao tiver um). */
	UPROPERTY(BlueprintReadOnly, Category = "Myth")
	TObjectPtr<class AMythDayNightActor> DayNight;

	/** Multiplicador das luzes do mapa (.rsw) nos interiores, que sao sempre noite. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth")
	float IndoorLightBoost = 5.f;

protected:
	virtual void BeginPlay() override;
};
