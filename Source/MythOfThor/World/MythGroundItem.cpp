#include "World/MythGroundItem.h"

#include "Sprites/MythRoSpriteComponent.h"
#include "Core/MythRoClientTables.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	constexpr float ItemDropDuration = 0.45f; // queda + quique
	constexpr float ItemDropHeight = 60.f;
}

AMythGroundItem::AMythGroundItem()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Sprite = CreateDefaultSubobject<UMythRoSpriteComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(Root);
	Sprite->MonsterSpriteFolder = TEXT("아이템"); // 아이템
	Sprite->bPlayActSounds = false;
	Sprite->bBlobShadow = true;        // o item no chao tem sombra (shadow.spr), como no cliente original (nostalro e goro)
	Sprite->BlobShadowScale = 0.55f;
	Sprite->bCastSpriteShadow = false;

	// Area de clique sobre o sprite (que fica paralelo a tela), como nos monstros
	ClickBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ClickBox"));
	ClickBox->SetupAttachment(Sprite);
	ClickBox->SetBoxExtent(FVector(50.f, 40.f, 40.f)); // generous: the cursor "snaps" to a dropped item (friend feedback: pixel-perfect clicks were needed)
	ClickBox->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
	ClickBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ClickBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	ClickBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// Aura da raridade: brilho colorido no chao e feixe de luz vertical (ligados no BeginPlay conforme a raridade)
	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Root);
	Glow->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	Glow->SetIntensityUnits(ELightUnits::Candelas);
	Glow->SetCastShadows(false);
	Glow->SetAttenuationRadius(220.f);
	Glow->SetVisibility(false);

	Beam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beam"));
	Beam->SetupAttachment(Root);
	Beam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Beam->SetCastShadow(false);
	Beam->SetVisibility(false);
	if (UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")))
	{
		Beam->SetStaticMesh(Plane);
	}
}

void AMythGroundItem::Setup(int64 InNetID, int32 InItemID, int32 InAmount, bool bJustDropped, const FVector& CellOffset, bool bHasCellOffset)
{
	NetID = InNetID;
	ItemID = InItemID;
	Amount = FMath::Max(1, InAmount);
	FString Resource = FMythRoClientTables::ItemResource(ItemID);
	Sprite->MonsterSpriteName = Resource.IsEmpty() ? TEXT("사과") : Resource; // 사과 (maca) se faltar na tabela
	// Itens na mesma celula: posicao fixa e diferente dentro dela (como o subX/subY do cliente original) e profundidade
	// levemente diferente, para os sprites nao brigarem pelo mesmo pixel (serrilhado, piscar e fundo rosa na mistura)
	const uint32 Hash = GetTypeHash(InNetID) * 2654435761u;
	const float JitterX = static_cast<float>(static_cast<int32>((Hash >> 8) % 5) - 2) * 7.f;
	const float JitterY = static_cast<float>(static_cast<int32>((Hash >> 16) % 5) - 2) * 7.f;
	Sprite->CameraDepthOffset += static_cast<float>((Hash >> 24) % 8) * 3.f;
	RestLocation = GetActorLocation() + (bHasCellOffset ? CellOffset : FVector(JitterX, JitterY, 0.f));
	SetActorLocation(RestLocation);
	DropTime = bJustDropped ? 0.f : -1.f;
	Rarity = FMythRoClientTables::ItemRarity(ItemID);
}

FString AMythGroundItem::GetLabel() const
{
	FString Name = FMythRoClientTables::ItemName(ItemID);
	if (Name.IsEmpty())
	{
		Name = FString::Printf(TEXT("Item %d"), ItemID);
	}
	return Amount > 1 ? FString::Printf(TEXT("%s: %d"), *Name, Amount) : Name;
}

void AMythGroundItem::BeginPlay()
{
	Super::BeginPlay();
	const FLinearColor Color = FMythRoClientTables::RarityColor(Rarity);
	if (Rarity >= 2)
	{
		Glow->SetLightColor(Color);
		Glow->SetIntensity(Rarity >= 4 ? 30.f : (Rarity == 3 ? 20.f : 12.f));
		Glow->SetVisibility(true);
	}
	if (Rarity >= 3)
	{
		if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Myth/M_ItemBeam.M_ItemBeam")))
		{
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, this);
			M->SetVectorParameterValue(TEXT("Color"), Color);
			M->SetScalarParameterValue(TEXT("Strength"), Rarity >= 4 ? 9.f : 6.f);
			Beam->SetMaterial(0, M);
			Beam->SetVisibility(true);
		}
	}
}

void AMythGroundItem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Beam->IsVisible())
	{
		// Feixe em pe e virado para a camera (so gira no eixo vertical): 35 cm de largura, 3,5 m de altura
		if (const APlayerController* PC = GetWorld()->GetFirstPlayerController(); PC && PC->PlayerCameraManager)
		{
			const float Yaw = PC->PlayerCameraManager->GetCameraRotation().Yaw;
			Beam->SetWorldRotation(FRotator(0.f, Yaw + 90.f, 90.f));
		}
		Beam->SetWorldLocation(GetActorLocation() + FVector(0.f, 0.f, 175.f));
		Beam->SetWorldScale3D(FVector(0.35f, 3.5f, 1.f));
	}
	if (DropTime < 0.f)
	{
		return;
	}
	// Item recem-caido: sobe e cai com um quique, como o drop do cliente original
	DropTime += DeltaSeconds;
	const float Step = DropTime * 1000.f / 24.f;
	const float FallOffset = -15.f + (-0.6f + 0.083f * Step) * Step; // curva do cliente: sobe um pouco e cai em ~0,42 s, sem quique
	if (FallOffset >= 0.f)
	{
		SetActorLocation(RestLocation);
		DropTime = -1.f;
	}
	else
	{
		SetActorLocation(RestLocation + FVector(0.f, 0.f, -FallOffset * 5.f));
	}
}
