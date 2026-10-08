#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "RagnarokNetworkSubsystem.generated.h"

// Delegates de Notificação de Rede para UI e Jogador
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetworkConnected, const FString&, ServerName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetworkDisconnected, const FString&, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLoginResult, bool, bSuccess, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerMoveConfirmed, int32, TargetX, int32, TargetY);

/**
 * Subsistema de Rede nativo em C++ para comunicação TCP entre o cliente
 * Unreal Engine 5 e os servidores do rAthena (Login, Char e Map servers).
 */
UCLASS()
class MYTHOFTHOR_API URagnarokNetworkSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Constantes de Portas e Protocolo
	static constexpr int32 LOGIN_SERVER_PORT = 6900;
	static constexpr int32 CHAR_SERVER_PORT  = 6121;
	static constexpr int32 MAP_SERVER_PORT   = 5121;
	static constexpr uint32 CLIENT_PACKETVER = 20211103;

	/**
	 * Conecta ao Login-Server do rAthena e envia as credenciais (CA_LOGIN 0x0064).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool ConnectAndLogin(const FString& ServerHost, const FString& Username, const FString& Password);

	/**
	 * Envia requisição de movimento para o Map-Server (CZ_REQUEST_MOVE 0x035F).
	 * Suporta coordenadas do mouse (Point & Click) ou geradas pelo WASD.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool SendMoveRequest(int32 TargetX, int32 TargetY);

	/**
	 * Desconecta todos os sockets ativos de forma limpa.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	void Disconnect();

	// Eventos Dispatchers
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnNetworkConnected OnNetworkConnected;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnNetworkDisconnected OnNetworkDisconnected;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnLoginResult OnLoginResult;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnPlayerMoveConfirmed OnPlayerMoveConfirmed;

private:
	FSocket* CurrentSocket;
	TArray<uint8> ReceiveBuffer;

	bool ConnectToPort(const FString& Host, int32 Port);
	void SendData(const uint8* Data, int32 Size);
	void ProcessIncomingData();
};
