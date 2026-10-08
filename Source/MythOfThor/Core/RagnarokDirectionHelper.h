#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RagnarokDirectionHelper.generated.h"

/**
 * As 8 direções canônicas de visão e animação do Ragnarok Online.
 * Indexadas exatamente como no cliente clássico:
 * 0 = Sul (Frente), 1 = Sudoeste, 2 = Oeste, 3 = Noroeste,
 * 4 = Norte (Costas), 5 = Nordeste, 6 = Leste, 7 = Sudeste.
 */
UENUM(BlueprintType)
enum class ERagnarokDirection : uint8
{
	South = 0     UMETA(DisplayName = "Sul (Frente)"),
	SouthWest = 1 UMETA(DisplayName = "Sudoeste"),
	West = 2      UMETA(DisplayName = "Oeste"),
	NorthWest = 3 UMETA(DisplayName = "Noroeste"),
	North = 4     UMETA(DisplayName = "Norte (Costas)"),
	NorthEast = 5 UMETA(DisplayName = "Nordeste"),
	East = 6      UMETA(DisplayName = "Leste"),
	SouthEast = 7 UMETA(DisplayName = "Sudeste")
};

UCLASS()
class MYTHOFTHOR_API URagnarokDirectionHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Calcula qual das 8 direções de sprite 2D deve ser reproduzida
	 * baseando-se no vetor de movimento do personagem e no ângulo da câmera.
	 *
	 * @param Velocity Vetor de velocidade/movimento do personagem no mundo.
	 * @param CameraRotation Rotação atual da câmera no mundo.
	 * @param LastDirection Direção anterior (usada se o personagem estiver parado).
	 * @return A direção correta do sprite (0 a 7).
	 */
	UFUNCTION(BlueprintPure, Category = "Ragnarok|Animation")
	static ERagnarokDirection CalculateSpriteDirection(
		const FVector& Velocity,
		const FRotator& CameraRotation,
		ERagnarokDirection LastDirection = ERagnarokDirection::South
	);

	/**
	 * Converte uma direção de enum em um ângulo em graus relativo.
	 */
	UFUNCTION(BlueprintPure, Category = "Ragnarok|Animation")
	static float DirectionToAngle(ERagnarokDirection Direction);
};
