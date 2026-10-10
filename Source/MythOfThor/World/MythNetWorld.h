#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Network/RagnarokNetworkSubsystem.h"
#include "MythNetWorld.generated.h"

class AMythMonster;
class UMythRoHUDWidget;

/**
 * Ponte entre o rAthena (URagnarokNetworkSubsystem) e o mundo da Unreal. O cliente so mostra o que o servidor manda:
 *   - login -> lista de personagens -> escolhe o primeiro -> entra no mapa (posicao inicial do servidor);
 *   - unidades (monstros, NPCs, jogadores, pets) aparecem, andam, apanham, morrem e somem pelos pacotes;
 *   - dano/MISS/critico vem do ZC_NOTIFY_ACT; o chat (inclusive respostas dos comandos @) do servidor.
 * Sem servidor (offline) nao faz nada e o mapa continua com os monstros locais de teste.
 *
 * Login automatico: linha de comando -MythOnline ou console "MythLogin". Credenciais de teste em
 * Config/DefaultGame.ini [Myth] NetHost/NetUser/NetPassword (mesmas de server/test_account.example).
 */
UCLASS()
class MYTHOFTHOR_API AMythNetWorld : public AActor
{
	GENERATED_BODY()

public:
	AMythNetWorld();

	static AMythNetWorld* FindOrSpawn(UWorld* World);

	/** Conecta e faz login com as credenciais do DefaultGame.ini (ou as passadas). */
	UFUNCTION(BlueprintCallable, Category = "Myth|Network")
	void Login(const FString& User = TEXT(""), const FString& Password = TEXT(""));

	/** Ator de uma unidade do servidor (nulo se nao estiver em campo). */
	AActor* FindUnit(int64 ID) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY() TMap<int64, TObjectPtr<AMythMonster>> Units;
	bool bInMap = false;
	FVector LastSelfDestination = FVector(TNumericLimits<float>::Max());

	URagnarokNetworkSubsystem* GetNet() const;
	UMythRoHUDWidget* GetHud() const;
	void Chat(const FString& Text, const FLinearColor& Color) const;
	FVector CellToGround(int32 X, int32 Y, bool* bOutFound = nullptr) const;
	AActor* ResolveActor(int64 ID) const;
	void RemoveOfflineMonsters();
	void FinishMapEnter(const FString& Map, int32 CellX, int32 CellY);

	UFUNCTION() void HandleLoginResult(bool bSuccess, const FString& Message);
	UFUNCTION() void HandleCharacterList(const TArray<FRagnarokCharacterSlotInfo>& Characters);
	UFUNCTION() void HandleMapEnter(const FString& MapName, int32 CellX, int32 CellY, uint8 Direction);
	UFUNCTION() void HandleUnitAppear(const FRagnarokUnitVisualInfo& Info);
	UFUNCTION() void HandleUnitWalk(int64 ID, int32 FromX, int32 FromY, int32 TargetX, int32 TargetY, int32 StartTick);
	UFUNCTION() void HandleUnitStop(int64 ID, int32 X, int32 Y);
	UFUNCTION() void HandleUnitDirection(int64 ID, uint8 HeadDir, uint8 BodyDir);
	UFUNCTION() void HandleUnitVanish(int64 ID, uint8 Reason);
	UFUNCTION() void HandleDamage(int64 SourceID, int64 TargetID, int32 Damage, uint8 DamageType, int32 DelayMs);
	UFUNCTION() void HandleChat(const FString& Sender, const FString& Text, const FString& ColorHex);
	UFUNCTION() void HandleStats(const FRagnarokPlayerStats& Stats);
	UFUNCTION() void HandleDisconnected(const FString& Reason);
	UFUNCTION() void HandleNpcDialog(int64 NpcID, const FString& Text);
	UFUNCTION() void HandleNpcMenu(int64 NpcID, const FString& Title, const TArray<FString>& Options);
	UFUNCTION() void HandleGroundItemAppear(int64 ID, int32 ItemID, int32 Amount, int32 CellX, int32 CellY);
	UFUNCTION() void HandleGroundItemVanish(int64 ID);
	UFUNCTION() void HandleSkillDamage(int64 SourceID, int64 TargetID, int32 SkillID, int32 Damage, int32 Hits, uint8 DamageType);
	UFUNCTION() void HandleSkillEffect(int64 SourceID, int64 TargetID, int32 SkillID, int32 Value);
	UFUNCTION() void HandleSkillCast(int64 SourceID, int64 TargetID, int32 SkillID, int32 CastMs, int32 CellX, int32 CellY);
	UFUNCTION() void HandleSkillFailed(int32 SkillID, int32 Cause);
	UFUNCTION() void HandleSpecialEffect(int64 UnitID, int32 EffectID);
	UFUNCTION() void HandleNotifyEffect(int64 UnitID, int32 Type);
	/** Ultimo efeito "no conjurador" por (unidade, skill): uma skill em area acerta varios alvos mas explode uma vez. */
	TMap<TPair<int64, int32>, double> LastCasterFx;
	UFUNCTION() void HandleEquipChanged(int64 UnitID, uint8 Look, int32 Value);
	UFUNCTION() void HandleStatusChange(int64 UnitID, int32 EfstID, bool bActive, int32 TotalMs, int32 RemainMs);

	/** Classe, sexo e cabelo do proprio personagem a partir do login/selecao (o sprite padrao e Aprendiz). */
	void ApplySelfAppearance();
	class AMythHUD* GetGameHud() const;

	UPROPERTY() TMap<int64, TObjectPtr<class AMythGroundItem>> GroundItems;
};
