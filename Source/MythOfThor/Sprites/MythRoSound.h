#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MythRoSound.generated.h"

class USoundAttenuation;

/**
 * Toca os .wav do cliente original direto do disco (data/wav/<nome>), sem importar assets:
 * os sons pedidos pelos eventos dos .act (poring_move.wav, player_clothes.wav...) e os de golpe (_hit_fist1.wav...).
 * Os PCM decodificados ficam em cache por arquivo.
 */
UCLASS()
class MYTHOFTHOR_API UMythRoSound : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Toca <AssetsRoot>/data/wav/<FileName> em 3D na posicao. Retorna falso se o arquivo nao existir ou for invalido. */
	UFUNCTION(BlueprintCallable, Category = "RO Sound", meta = (WorldContext = "WorldContext"))
	static bool PlayWavAtLocation(const UObject* WorldContext, const FString& FileName, const FVector& Location, float Volume = 1.f);

	/** Volumes do jogador (menu Esc > Som e interface), 0..1; salvos em [Myth.Audio] do GameUserSettings.ini. */
	static float GetMusicVolume();
	static float GetEffectsVolume();
	static void SetVolumes(float Music, float Effects);

private:
	static USoundAttenuation* GetAttenuation();
};
