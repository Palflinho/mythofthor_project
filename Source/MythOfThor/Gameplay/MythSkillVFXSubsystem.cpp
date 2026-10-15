#include "Gameplay/MythSkillVFXSubsystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Sprites/MythSpriteAnimComponent.h"
#include "Camera/CameraShakeBase.h"

void UMythSkillVFXSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("[SkillVFX] UMythSkillVFXSubsystem inicializado com Object Pool ativo."));
}

void UMythSkillVFXSubsystem::Deinitialize()
{
	NiagaraPool.Empty();
	Super::Deinitialize();
}

void UMythSkillVFXSubsystem::PlayCastVFX(const FVector& CellWorldLocation, ESkillElement Element, float CastDuration)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FLinearColor ElementColor = FSkillVisualData::GetElementPrimaryColor(Element);

	// Ponto de luz dinâmico ascendente na base do sprite (GDD Técnico HD-2D)
	if (ActiveDynamicLightCount < MaxSimultaneousDynamicLights)
	{
		AActor* TempLightActor = World->SpawnActor<AActor>(CellWorldLocation + FVector(0.f, 0.f, 15.f), FRotator::ZeroRotator);
		if (TempLightActor)
		{
			UPointLightComponent* LightComp = NewObject<UPointLightComponent>(TempLightActor);
			if (LightComp)
			{
				LightComp->RegisterComponent();
				LightComp->SetLightColor(ElementColor);
				LightComp->SetIntensity(2500.f);
				LightComp->SetAttenuationRadius(350.f);
				LightComp->SetCastShadows(true);
				TempLightActor->SetRootComponent(LightComp);
				TempLightActor->SetLifeSpan(FMath::Max(0.5f, CastDuration));
				ActiveDynamicLightCount++;

				// Decrementa o contador de luzes ativas após o término
				FTimerHandle Handle;
				World->GetTimerManager().SetTimer(Handle, [this]()
				{
					ActiveDynamicLightCount = FMath::Max(0, ActiveDynamicLightCount - 1);
				}, CastDuration, false);
			}
		}
	}
}

void UMythSkillVFXSubsystem::PlaySkillAction(const FSkillVisualData& SkillData, const FVector& OriginLocation, AActor* TargetActor)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector TargetLoc = TargetActor ? TargetActor->GetActorLocation() : (OriginLocation + FVector(200.f, 0.f, 0.f));

	if (SkillData.ActionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, SkillData.ActionEffect, OriginLocation,
			(TargetLoc - OriginLocation).Rotation());
	}
}

void UMythSkillVFXSubsystem::TriggerSkillImpact(AActor* TargetActor, const FSkillVisualData& SkillData, const FVector& ImpactLocation)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 1. Spawna efeito de impacto Niagara
	if (SkillData.ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, SkillData.ImpactEffect, ImpactLocation);
	}

	// 2. Hitstop e Hit-Flash no alvo
	if (TargetActor)
	{
		if (UMythSpriteAnimComponent* SpriteComp = TargetActor->FindComponentByClass<UMythSpriteAnimComponent>())
		{
			SpriteComp->TriggerHitstop(SkillData.HitstopDuration);
			SpriteComp->TriggerHitFlash(0.05f);
		}
	}

	// 3. Camera Shake no jogador
	if (SkillData.CameraShakeClass)
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PC->ClientStartCameraShake(SkillData.CameraShakeClass);
		}
	}

	// 4. Decalque transitório de impacto no cenário (3s a 8s)
	if (SkillData.bLeavesDecal && SkillData.DecalMaterial)
	{
		UGameplayStatics::SpawnDecalAtLocation(World, SkillData.DecalMaterial, FVector(64.f, 64.f, 64.f),
			ImpactLocation, FRotator(-90.f, 0.f, 0.f), FMath::FRandRange(3.f, 8.f));
	}
}

UNiagaraComponent* UMythSkillVFXSubsystem::AcquireNiagaraFromPool(UNiagaraSystem* SystemTemplate, const FVector& Location, const FRotator& Rotation)
{
	if (!SystemTemplate)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Spawna e configura componente reutilizável
	UNiagaraComponent* Comp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, SystemTemplate, Location, Rotation);
	return Comp;
}

void UMythSkillVFXSubsystem::ReleaseNiagaraToPool(UNiagaraComponent* Component)
{
	if (Component)
	{
		Component->Deactivate();
	}
}
