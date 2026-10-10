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

	// Tabelas de som do cliente (docs/ref/07 2.2 e 2.3), pelo tipo de arma do rAthena (W_FIST 0, W_DAGGER 1 ... W_2HSTAFF 23)
	/** Balanco da arma no quadro "atk" do .act (attack_*.wav). */
	static FString WeaponSwingWav(int32 WeaponType);
	/** Impacto no alvo pela arma de quem bateu (_hit_*.wav; desarmado sorteia _hit_fist1..4). */
	static FString WeaponHitWav(int32 WeaponType);
	/** Reacao do corpo do jogador atingido: tecido, metal ou couro, pela classe (player_*.wav). */
	static FString JobHitWav(int32 JobClass, bool bMale);

private:
	static USoundAttenuation* GetAttenuation();
};
