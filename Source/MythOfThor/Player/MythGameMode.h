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

protected:
	virtual void BeginPlay() override;
};
