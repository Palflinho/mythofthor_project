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
	TMap<int64, int32> PendingSpirits; // esferas anunciadas antes de a unidade aparecer
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
	UFUNCTION() void HandleUnitOption(int64 ID, int32 Option);
	UFUNCTION() void HandleUnitRevive(int64 ID);
	UFUNCTION() void HandleShopDealType(int64 NpcID);
	UFUNCTION() void HandleShopBuyList(const TArray<FRagnarokShopItem>& Items);
	UFUNCTION() void HandleShopSellList(const TArray<FRagnarokShopItem>& Items);
	UFUNCTION() void HandleShopResult(bool bSell, int32 Result);
	UFUNCTION() void HandleStorageOpen(const TArray<FRagnarokInventoryItem>& Items);
	UFUNCTION() void HandleSelectList(uint8 ListType, const TArray<int32>& Values, const TArray<FString>& Names);
	UFUNCTION() void HandleVending(uint8 Event);
	UFUNCTION() void HandleGuild(uint8 Event);
	UFUNCTION() void HandlePet(uint8 Event);
	UFUNCTION() void HandleHomun(uint8 Event);
	UFUNCTION() void HandleStorageChanged(const TArray<FRagnarokInventoryItem>& Items);
	UFUNCTION() void HandlePartyChanged();
	UFUNCTION() void HandleCharCreateResult(bool bSuccess, const FString& Message);
	UFUNCTION() void HandleTradeRequest(const FString& FromName);
	UFUNCTION() void HandleTradeChanged();
	UFUNCTION() void HandleFriendsChanged();
	UFUNCTION() void HandleChatRoomChanged();
	UFUNCTION() void HandleQuestsChanged();
	UFUNCTION() void HandleBankChanged();
	UFUNCTION() void HandleMailChanged();
	UFUNCTION() void HandleEquipSwitchChanged();
	UFUNCTION() void HandleSpiritBalls(int64 UnitID, int32 Count);
	UFUNCTION() void HandleAchievementsChanged();
	UFUNCTION() void HandleFriendRequest(int32 AccountId, int32 CharId, const FString& Name);
	UFUNCTION() void HandleSkillGround(int64 SourceID, int32 SkillID, int32 Level, int32 CellX, int32 CellY);
	UFUNCTION() void HandleSkillUnitAppear(int64 UnitID, int64 OwnerID, int32 UnitType, int32 X, int32 Y);
	UFUNCTION() void HandleSkillUnitDisappear(int64 UnitID);
	UFUNCTION() void HandleMonsterHp(int64 UnitID, int32 Hp, int32 MaxHp);
	UFUNCTION() void HandleUnitBodyState(int64 UnitID, int32 BodyState, int32 HealthState);
	UFUNCTION() void HandlePartyInvite(int32 PartyId, const FString& PartyName);
	UFUNCTION() void HandleStorageClose();
	UFUNCTION() void HandleSpeech(int64 UnitID, const FString& Text);
	UFUNCTION() void HandleEmotion(int64 UnitID, int32 EmotionType);
	UFUNCTION() void HandleUnitVanish(int64 ID, uint8 Reason);
	UFUNCTION() void HandleDamage(int64 SourceID, int64 TargetID, int32 Damage, uint8 DamageType, int32 DelayMs);
	/** Numero de dano, sons e reacao do alvo (adiado ate a flecha chegar no caso do arco). */
	void ShowImpact(int64 TargetID, AActor* Source, AActor* Target, int32 Damage, uint8 DamageType);
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
	/** Quando cada (conjurador, skill) comecou a conjurar (fim previsto): o beginCast nao repete ao soltar. */
	TMap<TPair<int64, int32>, double> LastCastBegin;
	/** Aura, alvo travado e circulo magico de quem esta conjurando; somem quando a skill sai (ClearCastFx). */
	TMap<int64, TArray<TWeakObjectPtr<AActor>>> CastRings;
	void ClearCastFx(int64 SourceID);
	/** Dano vindo de skill: o golpe nao dispara a flecha do ataque comum. */
	bool bSkillHitInProgress = false;
	/** Unidades de skill no chao (Muralha de Fogo, Santuario, Portal...): o efeito se repete enquanto a unidade existir. */
	TMap<int64, FTimerHandle> SkillUnitTimers;
	/** Quando cada skill de area foi usada no chao: o efeito do chao ja cobre o alvo, entao o do alvo nao repete. */
	TMap<int32, double> LastGroundSkill;
	UFUNCTION() void HandleEquipChanged(int64 UnitID, uint8 Look, int32 Value);
	UFUNCTION() void HandleEquipResult(int32 Index, int32 WearLocation, bool bSuccess);
	UFUNCTION() void HandleItemUseResult(int32 ItemID, int64 TargetID, bool bSuccess);
	UFUNCTION() void HandleStatusChange(int64 UnitID, int32 EfstID, bool bActive, int32 TotalMs, int32 RemainMs);

	/** Classe, sexo e cabelo do proprio personagem a partir do login/selecao (o sprite padrao e Aprendiz). */
	void ApplySelfAppearance();
	/** Ultimo estado (OPTION_*) do proprio jogador: reaplicado ao entrar em outro mapa, onde o sprite e novo. */
	int32 SelfOption = 0;
	class AMythHUD* GetGameHud() const;

	UPROPERTY() TMap<int64, TObjectPtr<class AMythGroundItem>> GroundItems;
};
