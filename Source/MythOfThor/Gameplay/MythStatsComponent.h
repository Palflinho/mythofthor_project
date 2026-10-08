#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MythStatsComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMythStatsChanged);

/**
 * Atributos exibidos no HUD: identidade, nivel, HP/SP e EXP base.
 * Os valores iniciais sao de teste; o subsistema de rede deve chamar os Set* ao
 * receber os pacotes de status do map-server (ZC_PAR_CHANGE, ZC_LONGPAR_CHANGE...).
 */
UCLASS(ClassGroup = (Myth), meta = (BlueprintSpawnableComponent))
class MYTHOFTHOR_API UMythStatsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Myth|Stats")
	void SetIdentity(const FString& InName, int32 InBaseLevel);

	UFUNCTION(BlueprintCallable, Category = "Myth|Stats")
	void SetVitals(int32 InHP, int32 InMaxHP, int32 InSP, int32 InMaxSP);

	UFUNCTION(BlueprintCallable, Category = "Myth|Stats")
	void SetExp(int64 InBaseExp, int64 InBaseExpToNext);

	UFUNCTION(BlueprintPure, Category = "Myth|Stats") const FString& GetCharName() const { return CharName; }
	UFUNCTION(BlueprintPure, Category = "Myth|Stats") int32 GetBaseLevel() const { return BaseLevel; }
	UFUNCTION(BlueprintPure, Category = "Myth|Stats") int32 GetHP() const { return HP; }
	UFUNCTION(BlueprintPure, Category = "Myth|Stats") int32 GetMaxHP() const { return MaxHP; }
	UFUNCTION(BlueprintPure, Category = "Myth|Stats") int32 GetSP() const { return SP; }
	UFUNCTION(BlueprintPure, Category = "Myth|Stats") int32 GetMaxSP() const { return MaxSP; }

	/** EXP base atual em [0,1]. */
	UFUNCTION(BlueprintPure, Category = "Myth|Stats")
	float GetExpFraction() const { return BaseExpToNext > 0 ? FMath::Clamp(static_cast<double>(BaseExp) / BaseExpToNext, 0.0, 1.0) : 0.f; }

	UPROPERTY(BlueprintAssignable, Category = "Myth|Stats")
	FOnMythStatsChanged OnStatsChanged;

private:
	UPROPERTY(EditAnywhere, Category = "Myth|Stats") FString CharName = TEXT("Aprendiz");
	UPROPERTY(EditAnywhere, Category = "Myth|Stats") int32 BaseLevel = 1;
	UPROPERTY(EditAnywhere, Category = "Myth|Stats") int32 HP = 40;
	UPROPERTY(EditAnywhere, Category = "Myth|Stats") int32 MaxHP = 40;
	UPROPERTY(EditAnywhere, Category = "Myth|Stats") int32 SP = 11;
	UPROPERTY(EditAnywhere, Category = "Myth|Stats") int32 MaxSP = 11;
	UPROPERTY(EditAnywhere, Category = "Myth|Stats") int64 BaseExp = 0;
	UPROPERTY(EditAnywhere, Category = "Myth|Stats") int64 BaseExpToNext = 100;
};
