#include "RagnarokDirectionHelper.h"

ERagnarokDirection URagnarokDirectionHelper::CalculateSpriteDirection(
	const FVector& Velocity,
	const FRotator& CameraRotation,
	ERagnarokDirection LastDirection)
{
	// Se a velocidade for muito baixa, mantém a direção anterior (Idle)
	if (Velocity.SizeSquared2D() < 100.0f) // ~10 cm/s
	{
		return LastDirection;
	}

	// Calcula o ângulo em que o personagem está se movendo no plano XY (Mundo)
	const float MoveAngle = FMath::RadiansToDegrees(FMath::Atan2(Velocity.Y, Velocity.X));

	// Obtém o ângulo Yaw da câmera
	const float CameraYaw = CameraRotation.Yaw;

	// O ângulo relativo é a diferença entre a direção de movimento e para onde a câmera está olhando
	float RelativeAngle = MoveAngle - CameraYaw;

	// Normaliza o ângulo para o intervalo [-180, 180]
	RelativeAngle = FRotator::NormalizeAxis(RelativeAngle);

	// No Ragnarok:
	// - Câmera olhando para o personagem de frente = Sul (Frente do personagem apontando para a câmera)
	// Ângulos mapeados em fatias de 45 graus (±22.5 graus para cada direção):
	//  0° = Costas para a câmera (Norte)
	//  45° = Nordeste
	//  90° = Leste
	//  135° = Sudeste
	//  180°/-180° = Frente para a câmera (Sul)
	// -135° = Sudoeste
	// -90° = Oeste
	// -45° = Noroeste

	if (RelativeAngle >= -22.5f && RelativeAngle < 22.5f)
	{
		return ERagnarokDirection::North;
	}
	else if (RelativeAngle >= 22.5f && RelativeAngle < 67.5f)
	{
		return ERagnarokDirection::NorthEast;
	}
	else if (RelativeAngle >= 67.5f && RelativeAngle < 112.5f)
	{
		return ERagnarokDirection::East;
	}
	else if (RelativeAngle >= 112.5f && RelativeAngle < 157.5f)
	{
		return ERagnarokDirection::SouthEast;
	}
	else if (RelativeAngle >= 157.5f || RelativeAngle < -157.5f)
	{
		return ERagnarokDirection::South;
	}
	else if (RelativeAngle >= -157.5f && RelativeAngle < -112.5f)
	{
		return ERagnarokDirection::SouthWest;
	}
	else if (RelativeAngle >= -112.5f && RelativeAngle < -67.5f)
	{
		return ERagnarokDirection::West;
	}
	else // RelativeAngle >= -67.5f && RelativeAngle < -22.5f
	{
		return ERagnarokDirection::NorthWest;
	}
}

float URagnarokDirectionHelper::DirectionToAngle(ERagnarokDirection Direction)
{
	switch (Direction)
	{
	case ERagnarokDirection::North:     return 0.0f;
	case ERagnarokDirection::NorthEast: return 45.0f;
	case ERagnarokDirection::East:      return 90.0f;
	case ERagnarokDirection::SouthEast: return 135.0f;
	case ERagnarokDirection::South:     return 180.0f;
	case ERagnarokDirection::SouthWest: return -135.0f;
	case ERagnarokDirection::West:      return -90.0f;
	case ERagnarokDirection::NorthWest: return -45.0f;
	default:                            return 180.0f;
	}
}
